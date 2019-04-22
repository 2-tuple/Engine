compiler = clang++-5.0
warning_flags = -Wall -Wconversion -Wno-missing-braces -Wno-sign-conversion -Wno-writable-strings -Wno-unused-variable  #-Wdouble-promotion 

linker_flags = -lGLEW -lGL `sdl2-config --cflags --libs` -lm -lSDL2_image -lSDL2_ttf


all:
	@$(compiler) $(warning_flags) -g -mavx -D USE_DEBUG_PROFILING -std=c++11 linux/*.cpp *.cpp linear_math/*.cpp -o engine $(linker_flags)
	@./engine

models:
	@./_build.sh

fbx_models:
	@./build_fbx.sh

animations:
	@./build_anim.sh

conference:
	@./builder/builder ./data/models_actors/conference.obj ./data/built/conference --model --scale 0.0035

skeleton:
	@./builder/builder ./data/animations/91_01.bvh ./data/built/01_91 --root_bone Hips --actor --scale 0.056444
