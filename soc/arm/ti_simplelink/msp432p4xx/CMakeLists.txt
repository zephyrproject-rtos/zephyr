zephyr_compile_definitions(-D__MSP432P401R__)
target_sources(soc_arm PRIVATE ${CMAKE_CURRENT_LIST_DIR}/soc.c)
