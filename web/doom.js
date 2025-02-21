// check which game and variant we should run
const parameters = new URLSearchParams(window.location.search);
var game = parameters.get('game') ?? 'doom';
var variant = parameters.get('variant') ?? '0';
var extra = parameters.get('extra');
if (!/^[a-z]+$/.test(game)) game = 'doom';
if (!/^[0-9]$/.test(variant)) variant = '0';
if (extra && !/^[A-Za-z0-9_]+$/.test(extra)) extra = null;
gameWasm = game + '.wasm';
if (extra) extraWadName = extra + '.wad';

// memory shared between WASM and JavaScript (1024*64K = 64M)
var wasmMemory = new WebAssembly.Memory({
  'initial': 1024,
  'maximum': 1024
});

// ImageData requires a Uint8ClampedArray
HEAPC8 = new Uint8ClampedArray(wasmMemory.buffer);
HEAPU8 = new Uint8Array(wasmMemory.buffer);
HEAP32 = new Int32Array(wasmMemory.buffer);

var doomAlloc, doomInit, doomStep, doomKey;
var last_ts = 0;
var db;
var files = [];

// retrieve the context to draw on the HTML canvas
const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
ctx.font = '18px monospace';

// add keydown/keyup event listeners to canvas' parent div
const display = document.getElementById('display');
display.addEventListener('keydown', function(e) {
  doomKey(1, e.keyCode);
  return false;
});
display.addEventListener('keyup', function(e) {
  doomKey(0, e.keyCode);
  return false;
});

// create an ImageData to hold the image
const image = new ImageData(canvas.width, canvas.height);
const len = canvas.width * canvas.height * 4;

function getStore(type) {
  var tx = db.transaction('files', type);
  return tx.objectStore('files');
}

function loadDB(action) {
  var req = indexedDB.open('vfs', 1);
  req.onupgradeneeded = function (evt) {
    console.log('loadDB upgradeneeded');
    evt.currentTarget.result.createObjectStore('files', { keyPath: 'path' });
  };
  req.onsuccess = function (evt) {
    console.log('loadDB success');
    db = this.result;

    var store = getStore('readonly');
    var cursorReq = store.openCursor();
    cursorReq.onsuccess = function (evt) {
      if (evt.target.result) {
        console.log('found file ' + evt.target.result.value.path);
        files[evt.target.result.value.path] = evt.target.result.value.data;
        evt.target.result.continue();
      } else {
        console.log('no more files');
        action();
      }
    };
  };
  req.onerror = function (evt) {
    console.error('loadDB error', evt.target.errorCode);
  };
}

function saveFile(obj) {
  console.log('saving ' + obj.path);
  var store = getStore('readwrite');
  var delReq = store.delete(obj.path);
  delReq.onsuccess = function (evt) {
    var addReq = store.add(obj);
    addReq.onsuccess = function (evt) {
      console.log('saveFile ' + obj.path + ' sucess');
    };
    addReq.onerror = function (evt) {
      console.error('saveFile ' + obj.path + ' error', this.error);
    };
  };
  delReq.onerror = function (evt) {
    console.error('saveFile ' + obj.path + ' error', this.error);
  };
}

function saveDB() {
  console.log('saveDB');

  for (var path in files) {
    var array = files[path];
    var obj = { path: path, data: array };
    saveFile(obj);
  }
}

function draw(ts) {
  // reduce the frame rate a bit
  if ((ts - last_ts) > 100.0) {
    // call WASM doomStep() function
    const addr = doomStep();

    // abort if doomStep returns NULL
    if (addr == 0) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      saveDB();
      return;
    }

    // the returned value is the image buffer address
    // put the buffer inside an array slice
    const buffer = HEAPC8.slice(addr, addr + len);

    // update the image data with the buffer contents
    image.data.set(buffer);

    // draw the image on the canvas
    ctx.putImageData(image, 0, 0);

    last_ts = ts;
  }

  // request the next frame
  requestAnimationFrame(draw);
}

function runGame() {
  // call WASM DoomInit() function once
  doomInit();

  // request to call JavaScript draw() function on the next frame
  requestAnimationFrame(draw);
}

// functions the Emscripten declares as imports, although
// the code does not use them
function _fd_write() {}
function _fd_close() {}
function _fd_seek() {}
function _clock_time_get() {}
function _proc_exit() {}

// returns current time
function _get_now(tp) {
  var now = performance.now();
  HEAP32[tp >> 2] = now / 1e3 | 0;
  HEAP32[tp + 4 >> 2] = now % 1e3 * 1e3 * 1e3 | 0;
}

// string decoder
var decoder = new TextDecoder('utf-8');

// logs a string to the console
function _console_log(addr) {
  for (i = 0; HEAPU8[addr + i] != 0; i++);
  const buffer = HEAPU8.slice(addr, addr + i);
  console.log(decoder.decode(buffer));
}

function _file_size(name_addr) {
  for (i = 0; HEAPU8[name_addr + i] != 0; i++);
  const name = decoder.decode(HEAPU8.slice(name_addr, name_addr + i));
  const array = files[name];
  return array ? array.byteLength : -1;
}

function _file_read(name_addr, buffer_addr) {
  for (i = 0; HEAPU8[name_addr + i] != 0; i++);
  const name = decoder.decode(HEAPU8.slice(name_addr, name_addr + i));
  console.log('reading file ' + name);
  const array = files[name];
  if (array) {
    console.log('got ' + array.byteLength + ' bytes');
    for (i = 0; i < array.byteLength; i++) {
      HEAPU8[buffer_addr + i] = array[i];
    }
  }
}

function _file_write(name_addr, buffer_addr, n) {
  for (i = 0; HEAPU8[name_addr + i] != 0; i++);
  const name = decoder.decode(HEAPU8.slice(name_addr, name_addr + i));
  const buffer = HEAPU8.slice(buffer_addr, buffer_addr + n);
  console.log('saving file ' + name);
  files[name] = buffer;
}

function _file_remove(name_addr) {
  for (i = 0; HEAPU8[name_addr + i] != 0; i++);
  const name = decoder.decode(HEAPU8.slice(name_addr, name_addr + i));
  console.log('removing file ' + name);
  delete files[name];
}

function _file_rename(oldname_addr, newname_addr) {
  for (i = 0; HEAPU8[oldname_addr + i] != 0; i++);
  const oldname = decoder.decode(HEAPU8.slice(oldname_addr, oldname_addr + i));
  for (i = 0; HEAPU8[newname_addr + i] != 0; i++);
  const newname = decoder.decode(HEAPU8.slice(newname_addr, newname_addr + i));
  console.log('renaming file ' + oldname + ' to ' + newname);
  files[newname] = files[oldname];
  delete files[oldname];
}

// draws an error messages on the canvas
function downloadError(name, error) {
  const msg = 'Error: could not load ' + name;
  console.log(msg);
  console.error(error);
  ctx.fillText(msg, 0, 18);
}

function download(filename, action) {
  // download the file
  fetch(filename).then(res => {
    if (res.ok) return res.arrayBuffer();
    // throw exception if download failed
    throw new Error();

  }).then(data => {
    const array = new Uint8Array(data);

    // copy the file bytes to WASM memory
    const addr = doomAlloc(array.byteLength);
    for (i = 0; i < array.byteLength; i++) {
      HEAPU8[addr + i] = array[i];
    }

    action();

  }).catch(error => {
    downloadError(filename, error);
  });
}

function _segfault() {
  console.log('segfault');
}

function _alignfault() {
  console.log('alignfault');
}

// the environment for the WASM runtime
var env = {
  'memory': wasmMemory,
  'fd_write': _fd_write,
  'fd_close': _fd_close,
  'fd_seek': _fd_seek,
  'clock_time_get': _clock_time_get,
  'proc_exit': _proc_exit,
  'get_now': _get_now,
  'console_log': _console_log,
  'file_size': _file_size,
  'file_read': _file_read,
  'file_write': _file_write,
  'file_remove': _file_remove,
  'file_rename': _file_rename,
  'segfault': _segfault,
  'alignfault': _alignfault,
}

// download and run the WASM code for the selected game
WebAssembly.instantiateStreaming(
  fetch(gameWasm),
  { env: env, wasi_snapshot_preview1: env }

).then(obj => {
  // save WASM functions to be called later
  doomAlloc = obj.instance.exports.DoomWadAlloc;
  doomInit = obj.instance.exports.DoomInit;
  doomStep = obj.instance.exports.DoomStep;
  doomKey = obj.instance.exports.DoomKey;

  // gets the WAD file name from the game and variant
  const addr = obj.instance.exports.DoomWadName(variant);
  for (i = 0; HEAPU8[addr + i] != 0; i++);
  const buffer = HEAPU8.slice(addr, addr + i);
  const wadName = decoder.decode(buffer);

  loadDB(function() {
    // download main WAD
    download(wadName, function() {
      if (typeof extraWadName !== 'undefined') {
        // download extra WAD (if specified)
        download(extraWadName, runGame);
      } else {
        runGame();
      }
    });
  });
}).catch(error => {
  downloadError(gameWasm, error);
});
