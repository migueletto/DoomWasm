This is an proof of concept port of Chocolate Doom (https://www.chocolate-doom.org/) to WebAssembly.
It is not a complete port, as some features are missing (sound, save files).
It allows you to play single-player versions of these games: Doom, Heretic, Hexen and Strife.

Currently supported WADs:

WAD file      Size      Comment
------------  --------  ----------------------------
DOOM1.WAD      4196020  DOOM (shareware version)
HERETIC1.WAD   5120300  Heretic (shareware version)
HEXEN.WAD     20083672  Hexen
strife1.wad   28372168  Strife

You must install an Emscripten SDK before building. To build, simply type make.
This will create these files:
web/doom.wasm
web/heretic.wasm
web/hexen.wasm
web/strife.wasm

You must also put the WAD files in the web diretory.

If you have Python 3, you can start a web server with this command:
python3 -m http.server -d ./web

Then point your browser to:
http://localhost:8000/doom.html

There is an optional 'game' parameter to speficy the game (the default being doom if no parameter):
http://localhost:8000/doom.html?game=doom
http://localhost:8000/doom.html?game=heretic
http://localhost:8000/doom.html?game=hexen
http://localhost:8000/doom.html?game=strife

Controls are arrow keys for movement, Control to fire, and ENTER and ESC like stamdard Doom.
Like I said, this is a proof of concept. The script doom.js has minimal error checking, so things may break easily.
There are some debug output to the browser console, if you are interested.
