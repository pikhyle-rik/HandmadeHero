@echo off

mkdir ..\build
pushd ..\build
cl /W2 -FC -Zi ..\code\win32_handmade.cpp user32.lib gdi32.lib
popd
