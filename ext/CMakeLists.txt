add_subdirectory(lib)
add_subdirectory(hal)
add_subdirectory_ifdef(CONFIG_FILE_SYSTEM fs)
add_subdirectory(debug)
