@echo off

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -Zi /Febreakout ..\src\win\win_main.c user32.lib gdi32.lib winmm.lib opengl32.lib
popd