@echo off
subst w: C:\Users\mason\Documents\Development\HandmadeHero
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

set path=w:\misc;"C:\Program Files (x86)\Vim\vim81\";%path%
w:
cd code
