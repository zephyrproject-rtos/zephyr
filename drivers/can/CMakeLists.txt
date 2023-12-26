# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/can.h)

zephyr_library()
zephyr_sources_ifdef(CONFIG_CAN_MCUX_MCAN    can_mcux_mcan.c)

zephyr_library_sources_ifdef(CONFIG_CAN              can_common.c)
zephyr_library_sources_ifdef(CONFIG_CAN_FAKE         can_fake.c)
zephyr_library_sources_ifdef(CONFIG_CAN_LOOPBACK     can_loopback.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCAN         can_mcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCP2515      can_mcp2515.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCP251XFD    can_mcp251xfd.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCUX_FLEXCAN can_mcux_flexcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SAM          can_sam.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SAM0         can_sam0.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32_BXCAN  can_stm32_bxcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32_FDCAN  can_stm32_fdcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32H7_FDCAN can_stm32h7_fdcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_TCAN4X5X     can_tcan4x5x.c)
zephyr_library_sources_ifdef(CONFIG_CAN_RCAR         can_rcar.c)
zephyr_library_sources_ifdef(CONFIG_CAN_NUMAKER      can_numaker.c)
zephyr_library_sources_ifdef(CONFIG_CAN_XMC4XXX      can_xmc4xxx.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SJA1000      can_sja1000.c)
zephyr_library_sources_ifdef(CONFIG_CAN_ESP32_TWAI   can_esp32_twai.c)
zephyr_library_sources_ifdef(CONFIG_CAN_KVASER_PCI   can_kvaser_pci.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE        can_handlers.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SHELL        can_shell.c)
zephyr_library_sources_ifdef(CONFIG_CAN_NXP_S32_CANXL    can_nxp_s32_canxl.c)

if(CONFIG_CAN_NATIVE_LINUX)
  if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)
    zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/l2)
    zephyr_library_sources(can_native_linux.c)

    if (CONFIG_NATIVE_APPLICATION)
      set_source_files_properties(can_native_linux_adapt.c
        PROPERTIES COMPILE_DEFINITIONS
        "NO_POSIX_CHEATS;_BSD_SOURCE;_DEFAULT_SOURCE")
      zephyr_library_sources(can_native_linux_adapt.c)
    else()
      target_sources(native_simulator INTERFACE can_native_linux_adapt.c)
    endif()
  else()
    message(FATAL_ERROR "CONFIG_CAN_NATIVE_LINUX is only available on Linux")
  endif()
endif()

add_subdirectory(transceiver)
