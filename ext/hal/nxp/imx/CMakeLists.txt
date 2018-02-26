# Translate the SoC name and part number into the imx device and cpu
# name respectively.
string(TOUPPER ${CONFIG_SOC} IMX_DEVICE)

set(IMX_CPU CPU_${CONFIG_SOC_PART_NUMBER_IMX7_M4})

zephyr_include_directories(devices/${IMX_DEVICE})

# The mcux uses the cpu name to expose SoC-specific features of a
# given peripheral. For example, the UART peripheral may be
# instantiated with/without a hardware FIFO, and the size of that FIFO
# may be different for each instance in a given SoC. See
# fsl_device_registers.h and ${MCUX_DEVICE}_features.h
zephyr_compile_definitions(${IMX_CPU})

# Build mcux drivers that can be used for multiple SoC's.
add_subdirectory(drivers)
