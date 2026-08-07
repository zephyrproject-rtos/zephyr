# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# QEMU's own diagnostic log, which is silent by default.
#
# Set QEMU_LOG to a comma separated list of QEMU log items to enable it, for
# example:
#
#   west build -b qemu_cortex_m3 -- -DQEMU_LOG=guest_errors,unimp
#
# "guest_errors" reports the guest doing something invalid, such as accessing an
# unassigned physical address or programming a device with a bad value, and
# "unimp" reports the guest reaching emulation QEMU has not implemented. Both
# are otherwise invisible: the guest simply misbehaves. Run
#
#   qemu-system-<arch> -d help
#
# for the full list of items, which includes verbose tracing options such as
# "exec" and "in_asm".
#
# The log is written to QEMU_LOG_FILE, by default qemu.log in the build
# directory. Set QEMU_LOG_FILE to "-" to send it to stderr instead.

zephyr_get(QEMU_LOG)
zephyr_get(QEMU_LOG_FILE)

if(QEMU_LOG)
  set_ifndef(QEMU_LOG_FILE ${APPLICATION_BINARY_DIR}/qemu.log)

  qemu_append_flags(-d ${QEMU_LOG})

  if(NOT "${QEMU_LOG_FILE}" STREQUAL "-")
    qemu_append_flags(-D ${QEMU_LOG_FILE})
  endif()
endif()
