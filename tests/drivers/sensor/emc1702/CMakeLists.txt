cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(test_simulated_emc1702)

target_sources(app
  PRIVATE
  src/emc1702_test.c
  src/emul_emc1702.c
)

# Include private header for unit tests
zephyr_include_directories(${ZEPHYR_BASE}/drivers/sensor/microchip/emc1702/)
zephyr_include_directories(./src)
