@echo off

set CommonCompilerFlags=-MT -Gm- -GR -EHa- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7
rem removed -WX because it treats all warnings as errors and i cant deal with that
rem removed -nologo cuz i wannna see the logo

set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST "..\..\Build" mkdir "..\..\Build"
pushd ..\..\Build

cl %CommonCompilerFlags% ..\handmade\code\handmade.cpp -Fmhandmade.map /LD /link /EXPORT:GameGetSoundSamples /EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%

popd
