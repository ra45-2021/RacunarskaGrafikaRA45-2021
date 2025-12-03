#!/bin/bash

clang++ -std=c++17 \
  Source/Main.cpp \
  Source/Shader.cpp \
  Source/RenderUtils.cpp \
  Source/Globals.cpp \
  Source/Util.cpp \
  -IHeader \
  -I/opt/homebrew/include \
  -L/opt/homebrew/lib \
  -lglfw -lGLEW -framework OpenGL \
  -o app

./app
