# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gen_inc_file)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Write the generated file into the include/generated directory, which
# is already in the system path
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(source_file src/file.bin)

generate_inc_file_for_target(app ${source_file} ${gen_dir}/file.bin.inc)
generate_inc_file_for_target(app ${source_file} ${gen_dir}/file.bin.partial.inc
  --offset=100 --length=42)
generate_inc_file_for_target(app ${source_file} ${gen_dir}/file.bin.gz.inc --gzip)
generate_inc_file_for_target(app ${source_file} ${gen_dir}/file.bin.mtime.gz.inc
  --gzip --gzip-mtime=42)
generate_inc_file_for_target(app ${source_file} ${gen_dir}/file.bin.partial.gz.inc
  --gzip --offset=100 --length=42)
