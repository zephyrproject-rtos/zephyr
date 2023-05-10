# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_sources_ifdef(CONFIG_CAN_MCUX_MCAN    can_mcux_mcan.c)

zephyr_library_sources_ifdef(CONFIG_CAN              can_common.c)
zephyr_library_sources_ifdef(CONFIG_CAN_FAKE         can_fake.c)
zephyr_library_sources_ifdef(CONFIG_CAN_LOOPBACK     can_loopback.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCAN         can_mcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCP2515      can_mcp2515.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCUX_FLEXCAN can_mcux_flexcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SAM          can_sam.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32        can_stm32.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32FD      can_stm32fd.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32H7      can_stm32h7.c)
zephyr_library_sources_ifdef(CONFIG_CAN_RCAR         can_rcar.c)

if(CONFIG_CAN_NATIVE_POSIX_LINUX)
  if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)
    zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/l2)
    zephyr_library_compile_definitions(NO_POSIX_CHEATS)
    zephyr_library_compile_definitions(_BSD_SOURCE)
    zephyr_library_compile_definitions(_DEFAULT_SOURCE)
    zephyr_library_sources(
      can_native_posix_linux.c
      can_native_posix_linux_socketcan.c
    )
  else()
    message(FATAL_ERROR "CONFIG_CAN_NATIVE_POSIX_LINUX only available on Linux")
  endif()
endif()

zephyr_library_sources_ifdef(CONFIG_CAN_SJA1000      can_sja1000.c)
zephyr_library_sources_ifdef(CONFIG_CAN_ESP32_TWAI   can_esp32_twai.c)
zephyr_library_sources_ifdef(CONFIG_CAN_KVASER_PCI   can_kvaser_pci.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE        can_handlers.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SHELL        can_shell.c)
zephyr_library_sources_ifdef(CONFIG_CAN_NXP_S32_CANXL    can_nxp_s32_canxl.c)

add_subdirectory(transceiver)
