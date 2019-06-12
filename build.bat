@echo off

IF NOT EXIST msvc_build mkdir msvc_build
pushd msvc_build

del *.pdb > NUL 2> NUL

cl -Od -nologo -DUSE_DEBUG_PROFILING -Zi -FC /std:c++latest /I ..\ /I ..\include /I ..\dear_imgui ..\win32\*.cpp ..\*.cpp ..\linear_math\*.cpp ..\dear_imgui\*.cpp /Fe: engine ..\lib\glew32.lib opengl32.lib ..\lib\SDL2main.lib ..\lib\SDL2.lib ..\lib\SDL2_ttf.lib shcore.lib /link -incremental:no -opt:ref /SUBSYSTEM:CONSOLE

popd
