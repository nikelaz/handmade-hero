@echo off

mkdir .\build
pushd .\build
cl -Zi ..\src\win32_handmade.cpp user32.lib gdi32.lib xinput.lib xaudio2.lib 
popd
