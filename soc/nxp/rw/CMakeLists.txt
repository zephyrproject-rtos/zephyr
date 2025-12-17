# Copyright 2022-2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  flexspi_clock_setup.c
  )

zephyr_sources_ifdef(CONFIG_NXP_NBU
  ../common/nxp_nbu.c
  )

zephyr_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_linker_sources_ifdef(CONFIG_NXP_RW6XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

if(CONFIG_NXP_RW6XX_BOOT_HEADER)
  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ".flash_conf"
    OFFSET ${CONFIG_FLASH_CONFIG_OFFSET}
    KEEP
    PRIO 10
  )

  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ".boot_hdr.ivt"
    OFFSET ${CONFIG_IMAGE_VECTOR_TABLE_OFFSET}
    KEEP
    PRIO 11
  )
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_include_directories(.)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)

zephyr_linker_sources(RODATA firmwares.ld)

# Configure WiFi firmware section
if(CONFIG_NXP_MONOLITHIC_WIFI)
  zephyr_linker_section_configure(
    SECTION rodata
    INPUT ".fw_cpu1"
    ALIGN 4
    KEEP
  )
  # For RW612, fw_cpu1 is used for wifi firmware.
  # Place the wifi and ble firmwares in rodata section. If both binaries
  # are placed next to each other, the firmware loader will detect the
  # second binary as part of the first one, leading to initialization
  # issue, so add a padding of one word
  zephyr_linker_section_configure(
    SECTION rodata
    NOINPUT
    OFFSET 4
  )
endif()

# Configure NBU (BLE) firmware section
if(CONFIG_NXP_MONOLITHIC_NBU)
  zephyr_linker_section_configure(
    SECTION rodata
    INPUT ".fw_cpu2"
    ALIGN 4
    KEEP
  )
endif()

zephyr_linker_sources(RAM_SECTIONS sections.ld)

set(TXQ23_SIZE 0x1080)
dt_nodelabel(smu1_data_node NODELABEL "smu1_data")
dt_reg_size(SMU1_SIZE PATH ${smu1_data_node})
dt_nodelabel(smu2_data_node NODELABEL "smu2_data")
dt_reg_size(SMU2_SIZE PATH ${smu2_data_node})

zephyr_linker_section(
  NAME .smu1
  VMA SMU1
  TYPE NOLOAD
  # Size is reserved for CPU3/CPU1 operations
  MIN_SIZE ${SMU1_SIZE}
)

zephyr_linker_section_configure(
  SECTION .smu1
  INPUT ".smu_cpu13_mbox" # CPU3 <-> CPU1 mailbox
        ".smu_cpu31_txq"  # CPU3 -> CPU1 TXQ
  ALIGN 4
  KEEP
)

zephyr_linker_section(
  NAME .smu2
  VMA SMU2
  TYPE NOLOAD
  MIN_SIZE ${SMU2_SIZE}
)

zephyr_linker_section_configure(
  SECTION .smu2
  INPUT ".smu_cpu23_mbox" # CPU3 <-> CPU2 mailbox
  ALIGN 4
  KEEP
)

# Reserve space for CPU1 -> CPU3 TXQ (allocated by the CPU2)
zephyr_linker_section_configure(
  SECTION .smu2
  MIN_SIZE ${TXQ23_SIZE}
  MAX_SIZE ${TXQ23_SIZE}
)

zephyr_linker_section_configure(
  SECTION .smu2
  INPUT ".smu_cpu32_txq" # CPU3 -> CPU2 TXQ
  KEEP
)
