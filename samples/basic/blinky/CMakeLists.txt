include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
