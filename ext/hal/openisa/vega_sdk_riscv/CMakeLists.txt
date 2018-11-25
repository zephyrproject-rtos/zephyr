# This CMakeLists.txt is being kept deliberately simple for now, since
# the HAL is currently only needed to support one SoC.

zephyr_include_directories(RISCV)
zephyr_include_directories(devices/RV32M1)

# The HAL uses a CPU name to expose core-specific features.
# See fsl_device_registers.h and $CPU_RV32M1_{ri5cy,zero_riscy}_features.h
# for details.
if(CONFIG_SOC_OPENISA_RV32M1_RI5CY)
  zephyr_compile_definitions(CPU_RV32M1_ri5cy)
else()
  zephyr_compile_definitions(CPU_RV32M1_zero_riscy)
endif()

add_subdirectory(devices/RV32M1/drivers)
