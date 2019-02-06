#!/bin/sh
#
# Copyright (c) 2019, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# FIXME: move (most of) this to doc/scripts/extract_content.py

# requires
# pip3 install --user sphinxcontrib-moderncmakedomain
#
# to test:


set -e
set -x

# Stop if not run from the expected place
test -d ../doc/_build


# Copy CMakeLists.txt and *.cmake source files to the build directory.
#
# it's quite hard to keep track of the directory hierarchy...
# rsync -av --include='*/' --include='*.cmake' --include='CMakeLists.txt' --exclude='*' .. _build/rst/
# mv _build/rst/CMakeLists.txt _build/rst/cmake/
rsync -a --include='*/' --include='*.cmake' --include='CMakeLists.txt' --exclude='*' ../cmake/  _build/rst/cmake/
rsync -a ../CMakeLists.txt _build/rst/cmake/


# Alternative hack to speed up the initial build.
# Main speed-up hack is disabling doxygen see doc/CMakeList.txt
# Unsurprisingly generates many "undefined" warnings
# rm -rf _build/rst/doc/{guides/,releases/,reference/kconfigxx/}
# find _build -name '*.rst' | wc; exit 1


# cmake --graphviz
( cd ../samples/hello_world/
  mkdir -p build && cd build
  ZEPHYR_TOOLCHAIN_VARIANT=host cmake -DBOARD=qemu_x86 --graphviz=cmake.dot ..
)

mkdir -p _build/html # CMake creates this but later
dot -Tsvg ../samples/hello_world/build/cmake.dot.zephyr_interface.dependers \
    > _build/html/zephyr_interface.svg
