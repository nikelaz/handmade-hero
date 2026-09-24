@echo off

mkdir .\build
pushd .\build
cl ..\src\win32_handmade.cpp
popd
