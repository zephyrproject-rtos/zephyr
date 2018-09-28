cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Write the generated file into the include/generated directory, which
# is already in the system path
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(source_file src/file.bin)

generate_inc_file_for_target(app ${source_file} ${gen_dir}/file.bin.inc)
generate_inc_file_for_target(app ${source_file} ${gen_dir}/file.bin.gz.inc --gzip)
