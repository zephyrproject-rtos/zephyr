include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_link_libraries(app mbedTLS)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
