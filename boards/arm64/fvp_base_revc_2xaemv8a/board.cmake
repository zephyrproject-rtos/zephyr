# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS armfvp)
set(ARMFVP_BIN_NAME FVP_Base_RevC-2xAEMvA)

set(ARMFVP_FLAGS
  -C bp.secure_memory=0
  -C cluster0.NUM_CORES=${CONFIG_MP_MAX_NUM_CPUS}
  -C bp.refcounter.non_arch_start_at_default=1
  # UART0 config
  -C bp.pl011_uart0.out_file=-
  -C bp.pl011_uart0.unbuffered_output=1
  -C bp.terminal_0.start_telnet=0
  # UART1 config
  -C bp.pl011_uart1.out_file=-
  -C bp.pl011_uart1.unbuffered_output=1
  -C bp.terminal_1.start_telnet=0
  # UART2 config
  -C bp.pl011_uart2.out_file=-
  -C bp.pl011_uart2.unbuffered_output=1
  -C bp.terminal_2.start_telnet=0
  # UART3 config
  -C bp.pl011_uart3.out_file=-
  -C bp.pl011_uart3.unbuffered_output=1
  -C bp.terminal_3.start_telnet=0

  -C bp.vis.disable_visualisation=1
  -C bp.vis.rate_limit-enable=0
  -C gic_distributor.ARE-fixed-to-one=1
  -C gic_distributor.ITS-device-bits=16
  -C cache_state_modelled=0
  )

if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "fvp")

  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(FVP_SECURE_FLASH_FILE ${TFA_BINARY_DIR}/fvp/debug/bl1.bin)
    set(FVP_FLASH_FILE ${TFA_BINARY_DIR}/fvp/debug/fip.bin)
  else()
    set(FVP_SECURE_FLASH_FILE ${TFA_BINARY_DIR}/fvp/release/bl1.bin)
    set(FVP_FLASH_FILE ${TFA_BINARY_DIR}/fvp/release/fip.bin)
  endif()
endif()

if(CONFIG_BOARD_FVP_LOAD_AHCI_IMAGE)
  set(CONFIG_BOARD_FVP_LOAD_AHCI_IMAGE ${PROJECT_SOURCE_DIR}/${CONFIG_BOARD_FVP_LOAD_AHCI_IMAGE})
  if(EXISTS ${CONFIG_BOARD_FVP_LOAD_AHCI_IMAGE})
    set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
      -C pci.ahci_pci.ahci.image_path=${CONFIG_BOARD_FVP_LOAD_AHCI_IMAGE}
    )
  else()
    message(WARNING "File not found ${CONFIG_BOARD_FVP_LOAD_AHCI_IMAGE}")
  endif()
endif()
