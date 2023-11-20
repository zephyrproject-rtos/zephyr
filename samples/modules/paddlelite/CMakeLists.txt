# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(image_classification)
target_sources(app PRIVATE src/image_classification.cc)
zephyr_compile_options(-fexceptions)
set_property(TARGET compiler-cpp PROPERTY no_rtti "")
