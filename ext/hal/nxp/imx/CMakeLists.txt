# Translate the SoC name and part number into the imx device and cpu
# name respectively.
string(TOUPPER ${CONFIG_SOC} IMX_DEVICE)

zephyr_include_directories(devices/${IMX_DEVICE})

# Build imx drivers and utilities that can be used for multiple SoC's.
add_subdirectory(drivers)
add_subdirectory(devices/${IMX_DEVICE})
