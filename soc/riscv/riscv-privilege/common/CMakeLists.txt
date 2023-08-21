# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

#commented out common idle.c file because of need to use the telink idle file
#the telink B92 CPU contains a WFI bug which causes unexpected behavior on peripheral
zephyr_sources(
  #idle.c
  soc_irq.S
  soc_common_irq.c
  vector.S
  )

if(!CONFIG_SOC_RISCV_TELINK_B92)
zephyr_sources(
	idle.c
)
endif()
