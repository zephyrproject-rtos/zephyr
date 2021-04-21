add_subdirectory(common)
add_subdirectory(${SOC_SERIES})
zephyr_compile_options(-march=rv32i)
