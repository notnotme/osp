# OSP - Old School Player
Yet another chiptune player.


To be able to compile the project you need the following libraries installed in your system:

- libsdl2, libsdl2-image
- libgme, libsidplayfp, libsc68 and libdumb
- Glad loader with 3.3 Core capabilities (your video card must support OpenGL 3.3 Core)


Currently this code can be build for Nintendo Switch and Linux but you may need to tweak the Makefile.sdl to fit your Linux needs.
I essentially target the switch and the other build help me for debug purpose.


This source code is bundled with a version of ImGui (1.76WIP tables branch).
- [ImGui](https://github.com/ocornut/imgui)


And some font:
- [Atari ST 8x16](https://www.dafont.com/fr/atari-st-8x16-system-font.font)
- [Material Design Icons](https://materialdesignicons.com/)


Some ideas:
- Implement the ability to browse modland ftp
- Create some custom controls using the ImGui framework
- When the worspace is not visible, add options to show something (minigames, song information, shiny shaders...)
- Battery save mode (black screen, stop refreshing or do it at 15fps ?)
