# DoomWasm

This is an proof of concept port of Chocolate Doom (https://www.chocolate-doom.org/) to WebAssembly.
It is not a complete port, as some features are missing (sound, save files).
It allows you to play single-player variants of these games: Doom, Heretic, Hexen and Strife.

Supported Doom variants:  
0: DOOM1.WAD (Doom shareware version)  
1: DOOM.WAD (Doom registered version)  
2: DOOM2.WAD (DOOM II)  
3: PLUTONIA.WAD (The Plutonia Experiment)  
4: TNT.WAD (TNT Evilution)  

Supported Heretic variants:  
0: HERETIC1.WAD: Heretic (shareware version)
1: HERETIC.WAD: Heretic (registered version)

Supported Hexen variant:  
0: HEXEN.WAD  

Supported Strife variant:  
0: STRIFE1.WAD  

You must install an Emscripten SDK before building. To build, simply type make. This will create these files:  
web/doom.wasm  
web/heretic.wasm  
web/hexen.wasm  
web/strife.wasm  

You must also put the WAD file(s) in the web diretory.

If you have Python 3, you can start a web server with this command:
python3 -m http.server -d ./web  
Then point your browser to:
http://localhost:8000/doom.html

There is an optional 'game' parameter to speficy the game (the default being doom if no parameter):  
http://localhost:8000/doom.html?game=doom  
http://localhost:8000/doom.html?game=heretic  
http://localhost:8000/doom.html?game=hexen  
http://localhost:8000/doom.html?game=strife  

There is an optional 'variant' parameter to speficy the game variant (the default being 0). Example:  
http://localhost:8000/doom.html?game=doom&variant=2  

Game controls are arrow keys for movement, CTRL to fire, and ENTER and ESC as in standard Doom.
You must click in the game canvas once before using keyboard controls.
Like I said, this is a proof of concept. The script doom.js has minimal error checking, so things may break easily.
There is some debug output to the browser console, if you are interested.
