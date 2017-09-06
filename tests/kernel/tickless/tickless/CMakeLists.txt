include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/test_tickless.c)
target_sources_ifdef(CONFIG_ARM app PRIVATE src/timestamps.c)
