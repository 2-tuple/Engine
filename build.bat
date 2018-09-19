@echo off

mkdir msvc_build
pushd msvc_build
cl -Zi -FC /std:c++latest /I ..\ /I ..\include ..\win32\*.cpp ..\*.cpp ..\linear_math\*.cpp /Fe: engine ..\lib\glew32.lib opengl32.lib ..\lib\SDL2main.lib ..\lib\SDL2.lib ..\lib\SDL2_image.lib ..\lib\SDL2_ttf.lib shcore.lib /link /SUBSYSTEM:CONSOLE
popd
