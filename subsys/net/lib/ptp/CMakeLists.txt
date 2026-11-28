# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

zephyr_library()

zephyr_library_sources(
  btca.c
  clock.c
  msg.c
  port.c
  ptp.c
  state_machine.c
  tlv.c
  transport.c
)
