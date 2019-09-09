@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl /W2 -O2 -FC -Zi ..\code\win32_handmade.cpp user32.lib gdi32.lib
popd
