compiler = clang++-5.0
warning_flags = -Wall -Wconversion -Wno-missing-braces -Wno-sign-conversion -Wno-writable-strings -Wno-unused-variable -Wno-unused-function -Wno-conversion -Wno-string-conversion -Wno-switch #-Wdouble-promotion

linker_flags = -lGLEW -lGL `sdl2-config --cflags --libs` -lm -lSDL2_ttf


all:
	@$(compiler) $(warning_flags) -o3 -std=c++11 linux/*.cpp *.cpp linear_math/*.cpp -o engine $(linker_flags)
	@./engine

