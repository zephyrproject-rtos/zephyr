
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/v8m-test/src/ext)
target_link_libraries(app $ENV{ZEPHYR_BASE}/samples/v8m-test/src/ext/s_veneers.o)
