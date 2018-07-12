target_sources(soc_posix PRIVATE ${CMAKE_CURRENT_LIST_DIR}/soc.c)
target_compile_definitions(soc_posix PRIVATE NO_POSIX_CHEATS)
