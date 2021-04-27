cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tensorflow_magic_wand)

# Required for TensorFlow to compile properly
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")

target_sources(app PRIVATE
  src/main_functions.h
  src/constants.h
  src/magic_wand_model_data.h
  src/gesture_predictor.h
  src/output_handler.h
  src/assert.cc
  src/accelerometer_handler.cc
  src/accelerometer_handler.h
  src/main.cc
  src/main_functions.cc
  src/magic_wand_model_data.cc
  src/gesture_predictor.cc
  src/output_handler.cc
  boards/litex_vexriscv.overlay
)
