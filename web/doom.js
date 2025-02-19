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

// retrieve the context to draw on the HTML canvas
const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
ctx.font = "18px monospace";

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

function draw(ts) {
  // reduce the frame rate a bit
  if ((ts - last_ts) > 100.0) {
    // call WASM doomStep() function
    const addr = doomStep();

    // abort if doomStep returns NULL
    if (addr == 0) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
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

// draws an error messages on the canvas
function downloadError(name) {
  const msg = 'Error: could not load ' + name;
  console.log(msg);
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
    downloadError(filename);
  });
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

  // download main WAD
  download(wadName, function() {
    if (typeof extraWadName !== 'undefined') {
      // download extra WAD (if specified)
      download(extraWadName, runGame);
    } else {
      runGame();
    }
  });
}).catch(error => {
  downloadError(gameWasm);
});
