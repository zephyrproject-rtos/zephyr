# Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS armfvp)
set(ARMFVP_BIN_NAME FVP_BaseR_AEMv8R)
set(ARMFVP_MIN_VERSION 11.16.16)

set(ARMFVP_FLAGS
  -C cluster0.has_aarch64=1
  -C cluster0.VMSA_supported=0
  -C cluster0.NUM_CORES=${CONFIG_MP_MAX_NUM_CPUS}
  -C cluster0.gicv3.cpuintf-mmap-access-level=2
  -C cluster0.gicv3.SRE-enable-action-on-mmap=2
  -C cluster0.gicv3.SRE-EL2-enable-RAO=1
  -C cluster0.gicv3.extended-interrupt-range-support=1
  -C gic_distributor.GICD_CTLR-DS-1-means-secure-only=1
  -C gic_distributor.has-two-security-states=0
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
  -C cache_state_modelled=1
  )
