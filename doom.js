const parameters = new URLSearchParams(window.location.search);
const game = parameters.get('game');

// memory shared between WASM and JavaScript
var wasmMemory = new WebAssembly.Memory({
  "initial": 768,
  "maximum": 768
});

// ImageData requires a Uint8ClampedArray
HEAP = new Uint8ClampedArray(wasmMemory.buffer);
HEAPU8 = new Uint8Array(wasmMemory.buffer);
HEAP32 = new Int32Array(wasmMemory.buffer);

var doomStep, doomKey;
var last_ts = 0;

// retrieve the context to draw on the HTML canvas
const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

const display = document.getElementById("display");
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

    // the returned value is the image buffer address
    // put the buffer inside an array slice
    const buffer = HEAP.slice(addr, addr + len);

    // update the image data with the buffer contents
    image.data.set(buffer);

    // draw the image on the canvas
    ctx.putImageData(image, 0, 0);

    last_ts = ts;
  }

  // request the next frame
  requestAnimationFrame(draw);
}

function _fd_write() {}
function _fd_close() {}
function _fd_seek() {}
function _clock_time_get() {}
function _proc_exit() {}

function _get_now(tp) {
  var now = performance.now();
  HEAP32[tp >> 2] = now / 1e3 | 0;
  HEAP32[tp + 4 >> 2] = now % 1e3 * 1e3 * 1e3 | 0;
}

var decoder = new TextDecoder("utf-8");

function _console_log(addr) {
  for (i = 0; HEAPU8[addr + i] != 0; i++);
  const buffer = HEAPU8.slice(addr, addr + i);
  console.log(decoder.decode(buffer));
}

// the environment for the WASM runtime
var env = {
  "memory": wasmMemory,
  "fd_write": _fd_write,
  "fd_close": _fd_close,
  "fd_seek": _fd_seek,
  "clock_time_get": _clock_time_get,
  "proc_exit": _proc_exit,
  "get_now": _get_now,
  "console_log": _console_log,
}

// load and run the WASM code in "doom.wasm"
WebAssembly.instantiateStreaming(
    fetch(game + '.wasm'),
    { env: env, wasi_snapshot_preview1: env }
  ).then(obj => {

    const addr = obj.instance.exports.DoomWadName();
    for (i = 0; HEAPU8[addr + i] != 0; i++);
    const buffer = HEAPU8.slice(addr, addr + i);
    const wadName = decoder.decode(buffer);

    // fetch the WAD file
    fetch(wadName)
      .then(res  => res.arrayBuffer())
      .then(data => {
        // create Uint8 data view
        const wad = new Uint8Array(data);
        // copy the WAD to WASM memory
        const addr = obj.instance.exports.DoomWadAlloc(wad.byteLength);
        for (i = 0; i < wad.byteLength; i++) {
          HEAPU8[addr + i] = wad[i];
        }

        // call WASM init() function once
        obj.instance.exports.DoomInit();

        // save WASM functions to be called later
        doomStep = obj.instance.exports.DoomStep;
        doomKey = obj.instance.exports.DoomKey;

        // request to call JavaScript draw() function on the next frame
        requestAnimationFrame(draw);
      })
  }
);
