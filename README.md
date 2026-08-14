# Traffic Master

Small FreeGLUT + legacy OpenGL prototype that simulates a 3D four-way intersection and basic traffic behavior.

Build (from openglportable):

    g++ -Wall -fexceptions -g main.cpp -I ..\freeglut\include -L ..\freeglut\lib\x64 -lfreeglut -lopengl32 -lglu32 -o bin\Debug\openglportable.exe

Controls (runtime):
- R: restart simulation
- L: toggle manual light control
- T: toggle current lights
- Esc: quit

Project layout:
- openglportable/main.cpp — single-file prototype
- freeglut/ — bundled libraries and headers

License: MIT

Keep it simple; open an issue if you want features or refactors.
