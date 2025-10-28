CMAKE_MINIMUM_REQUIRED (VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(audio_example)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dmic_gain_speaker.c)
