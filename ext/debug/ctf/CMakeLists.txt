zephyr_include_directories(.)

add_subdirectory_ifdef(CONFIG_CTF_BOTTOM_POSIX bottoms/posix)
