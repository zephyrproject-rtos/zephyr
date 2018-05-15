# Translate the SoC name and part number into the mcux device and cpu
# name respectively.
string(TOUPPER ${CONFIG_SOC} MCUX_DEVICE)

if("${MCUX_DEVICE}" STREQUAL "LPC54114")
  set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER}_cm4)
elseif("${MCUX_DEVICE}" STREQUAL "LPC54114_M0")
  set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER}_cm0plus)
  set(MCUX_DEVICE LPC54114)
else()
  set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER})
endif()

zephyr_include_directories(devices/${MCUX_DEVICE})

# The mcux uses the cpu name to expose SoC-specific features of a
# given peripheral. For example, the UART peripheral may be
# instantiated with/without a hardware FIFO, and the size of that FIFO
# may be different for each instance in a given SoC. See
# fsl_device_registers.h and ${MCUX_DEVICE}_features.h
zephyr_compile_definitions(${MCUX_CPU})

# Build mcux device-specific objects. Although it is not normal
# practice, drilling down like this avoids the need for repetitive
# build scripts for every mcux device.
zephyr_sources(devices/${MCUX_DEVICE}/fsl_clock.c)

# Build mcux drivers that can be used for multiple SoC's.
add_subdirectory(components)
add_subdirectory(drivers)

add_subdirectory_ifdef(
  CONFIG_IEEE802154_KW41Z
  middleware/wireless/framework_5.3.3
  )

enable_language(C ASM)

zephyr_sources_ifdef(CONFIG_SOC_LPC54114_M4 devices/${MCUX_DEVICE}/gcc/startup_LPC54114_cm4.S)
