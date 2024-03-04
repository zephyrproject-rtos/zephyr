# SPDX-License-Identifier: Apache-2.0

# It seems llext currently doesn't support some Thumb32 relocation
# instructions generated when building the extensions with default
# flags. As a workaround, we remove these *unrelated* flags from the
# default flags. This allows the extensions to be built without the
# unsupported instructions.
# See issue #72832 for more details.
list(APPEND LLEXT_EDK_REMOVE_FLAGS -mcpu=cortex-r5 -mfloat-abi=hard)

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE src/main.c src/pubsub.c)
zephyr_include_directories(include)
