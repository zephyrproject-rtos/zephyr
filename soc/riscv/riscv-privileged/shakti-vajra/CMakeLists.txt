
if(CONFIG_SOC_RISCV64_SHAKTI)
    if (CONFIG_RISCV_GENERIC_TOOLCHAIN)
        zephyr_compile_options(-g -march=rv64imafdc -mabi=lp64d -O2 -mcmodel=medany)
    else()
        zephyr_compile_options(-g -march=rv64imac -mabi=lp64 -O)
    endif()
endif()

zephyr_sources()
