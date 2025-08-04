@echo off

rem The d from -MTd links the c runtime library in debug mode, remove the d for shipping
set CommonCompilerFlags=-MTd -Gm- -GR -EHa- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7
rem removed -WX because it treats all warnings as errors and i cant deal with that
rem removed -nologo cuz i wannna see the logo

set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST "..\..\Build" mkdir "..\..\Build"
pushd ..\..\Build

rem del *.pdb > NUL 2> NULL
del *.pdb

rem cl %CommonCompilerFlags% ..\handmade\code\handmade.cpp -Fmhandmade.map -LD /link /PDB:handmade_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
rem the /PDB flag fixes a problem that I don't have

cl %CommonCompilerFlags% ..\handmade\code\handmade.cpp -Fmhandmade.map -LD /link -incremental:no  -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%

popd
