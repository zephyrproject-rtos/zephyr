# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# By default QEMU restarts the guest when the machine resets. An application
# built without reboot support has no reason to reset, so a reset means it
# misbehaved; restarting it hides that behind a boot loop. Stop the emulator
# instead, which surfaces the reset and lets an automated run finish.

if(NOT CONFIG_REBOOT)
  qemu_append_flags(-no-reboot)
endif()
