#zephyr_link_interface_ifdef(CONFIG_HAS_STM32CUBE STM32CUBE)
add_subdirectory(${SOC_SERIES})
add_subdirectory(common)

#zephyr_library_link_libraries_ifdef(CONFIG_HAS_STM32CUBE STM32CUBE)
