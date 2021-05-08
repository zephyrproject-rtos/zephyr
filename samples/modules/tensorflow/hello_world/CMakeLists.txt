cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tensorflow_hello_world)

# Required for TensorFlow to compile properly
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")

target_sources(app PRIVATE
  src/assert.cc
  src/main.c
  src/main_functions.cc
  src/constants.c
  src/output_handler.cc
  src/model.cc
  src/model.h
  src/output_handler.h
  src/constants.h
  src/main_functions.h
)
