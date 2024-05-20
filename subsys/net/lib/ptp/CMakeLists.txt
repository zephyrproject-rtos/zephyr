# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  clock.c
  msg.c
  port.c
  ptp.c
  state_machine.c
)
