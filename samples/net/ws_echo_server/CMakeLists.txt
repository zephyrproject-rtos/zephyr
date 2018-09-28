cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

# List of files that are used to generate .h file that can be included
# into .c file.
foreach(inc_file
	echo-apps-cert.der
	echo-apps-key.der
	index.html
    )
  generate_inc_file_for_target(
    app
    src/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()

generate_inc_file_for_target(app src/ws.js ${gen_dir}/ws.js.gz.inc --gzip)
