include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE
  src/main.c
)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
