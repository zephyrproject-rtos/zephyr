# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS armfvp)
set(ARMFVP_BIN_NAME FVP_Base_RevC-2xAEMvA)

set(ARMFVP_FLAGS
  -C bp.secure_memory=0
  -C cluster0.NUM_CORES=1
  -C bp.refcounter.non_arch_start_at_default=1
  -C bp.pl011_uart0.out_file=-
  -C bp.pl011_uart0.unbuffered_output=1
  -C bp.terminal_0.start_telnet=0
  -C bp.vis.disable_visualisation=1
  -C bp.vis.rate_limit-enable=0
  -C gic_distributor.ARE-fixed-to-one=1
  -C gic_distributor.ITS-device-bits=16
  )
