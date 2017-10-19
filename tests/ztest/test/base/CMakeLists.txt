list(APPEND INCLUDE subsys)

list(APPEND SOURCES src/main.c)

include($ENV{ZEPHYR_BASE}/tests/unit/unittest.cmake)
project(none)
