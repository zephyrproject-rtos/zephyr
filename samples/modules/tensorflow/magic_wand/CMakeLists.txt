cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tensorflow_magic_wand)

# These samples use local static initialization. Since Zephyr doesn't support the
# C++ ABI for thread-safe initialization of local statics and the constructors don't
# appear to require thread safety, we turn it off in the C++ compiler.
set(NO_THREADSAFE_STATICS $<TARGET_PROPERTY:compiler-cpp,no_threadsafe_statics>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${NO_THREADSAFE_STATICS}>)

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
