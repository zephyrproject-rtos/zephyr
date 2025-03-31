cmake_minimum_required(VERSION 3.20.0)

# Get the version from Git
execute_process(
COMMAND git describe --tags --long
WORKING_DIRECTORY                ${CMAKE_CURRENT_SOURCE_DIR}
OUTPUT_VARIABLE                  version
OUTPUT_STRIP_TRAILING_WHITESPACE
ERROR_STRIP_TRAILING_WHITESPACE
ERROR_VARIABLE                   stderr
RESULT_VARIABLE                  return_code
)
if(return_code)
message(FATAL_ERROR "git describe failed: ${stderr}; Need a git tag in the history to compile!")
endif()

string(REGEX MATCH "^v?([0-9]+)\.([0-9]+)\.([0-9]+)-([0-9]+)-(.*)$" _ "${version}")
set(version_major ${CMAKE_MATCH_1})
set(version_minor ${CMAKE_MATCH_2})
set(version_patch ${CMAKE_MATCH_3})
set(version_commit ${CMAKE_MATCH_4})
set(version_hash ${CMAKE_MATCH_5})

file(WRITE VERSION "\
VERSION_MAJOR = ${version_major}\n\
VERSION_MINOR = ${version_minor}\n\
PATCHLEVEL = ${version_patch}\n\
VERSION_TWEAK = ${version_commit}\n\
VERSION_EXTRAVERSION = \"${version_hash}\"\n\
")

message("Dali-UART-Zephyr app version: v${version_major}.${version_minor}.${version_patch} (${version_commit} commits since. hash:${version_hash})")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(dali_app LANGUAGES C)

target_sources(app PRIVATE src/main.c)
