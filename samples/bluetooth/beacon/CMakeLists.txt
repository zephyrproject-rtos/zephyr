include(ENV${ZEPHYR_BASE}/cmake/app/boilerplate.cmake)

target_sources(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
