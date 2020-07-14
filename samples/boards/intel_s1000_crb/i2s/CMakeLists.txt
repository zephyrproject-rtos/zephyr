# SPDX-License-Identifier: Apache-2.0

set(BOARD intel_s1000_crb)
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2s)

target_sources(app PRIVATE src/i2s_sample.c)

if (AUDIO_PLAY_FROM_HOST)
add_compile_definitions(AUDIO_PLAY_FROM_HOST)
else ()
target_link_libraries(app PUBLIC -lm)
target_link_libraries(app PUBLIC -lc)
target_link_libraries(app PUBLIC -Wl,-u,__extendsfdf2)
target_link_libraries(app PUBLIC -Wl,-u,__truncdfsf2)
target_link_libraries(app PUBLIC -Wl,-u,__divsf3)
target_link_libraries(app PUBLIC -Wl,-u,__divdf3)
target_link_libraries(app PUBLIC -Wl,-u,__adddf3)
target_link_libraries(app PUBLIC -Wl,-u,__subdf3)
target_link_libraries(app PUBLIC -Wl,-u,__ieee754_sqrtf)
target_link_libraries(app PUBLIC -Wl,-u,__ieee754_remainderf)
target_link_libraries(app PUBLIC -Wl,-u,__nedf2)
endif ()
