cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(test_simulated_emc1702)

target_sources(app
  PRIVATE
  src/emc1702_test.c
  src/emul_emc1702.c
  src/emul_emc1702.h
)

# Include private header for unit tests
zephyr_include_directories(${ZEPHYR_BASE}/drivers/sensor/microchip/emc1702/)
target_sources_ifdef(CONFIG_EMC1702_TRIGGER app PRIVATE src/emc1702_test_trigger.c)
