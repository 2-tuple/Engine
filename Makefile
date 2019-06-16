compiler = clang++-5.0
warning_flags = -Wall -Wconversion -Wno-missing-braces -Wno-sign-conversion -Wno-writable-strings -Wno-unused-variable -Wno-unused-function -Wno-conversion -Wno-string-conversion -Wno-switch -Wno-format-security #-Wdouble-promotion

linker_flags = -lGLEW -lGL `sdl2-config --cflags --libs` -lm -lSDL2_ttf


all:
	@$(compiler) $(warning_flags) -g -mavx -D USE_DEBUG_PROFILING -std=c++11 -I./include -I./dear_imgui linux/*.cpp *.cpp linear_math/*.cpp dear_imgui/*.cpp -o engine $(linker_flags)
	@./engine

plot:
	@python3 plot_measurements.py

models:
	./_build.sh
	./builder/builder ./data/models_actors/conference.obj ./data/built/conference --model --scale 0.0035

animations:
	./clean_anims_actors.sh
	make mixamo
	make cmu

raw_mocap:
	./build_raw_mocap.sh

mixamo:
	./build_mixamo.sh

cmu:
	./build_cmu.sh
