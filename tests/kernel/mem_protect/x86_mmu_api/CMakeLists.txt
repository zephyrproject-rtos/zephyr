include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

file(GLOB source_for_app src/*.c)

target_sources(app PRIVATE ${source_for_app})
