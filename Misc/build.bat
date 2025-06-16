@echo off

IF NOT EXIST mkdir "..\..\Build" mkdir "..\..\Build"
pushd ..\..\Build
cl -Gm- -GR- -Oi -W4 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 /Zi -Fmwin32_handmade.map ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib winmm.lib
popd
