# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Drive the guest clock from the instruction counter instead of the host clock,
# which makes emulated time deterministic.

if(CONFIG_QEMU_ICOUNT)
  if(CONFIG_QEMU_ICOUNT_SLEEP)
    list(APPEND QEMU_FLAGS
      -icount shift=${CONFIG_QEMU_ICOUNT_SHIFT},align=off,sleep=on
      -rtc clock=vm
    )
  else()
    list(APPEND QEMU_FLAGS
      -icount shift=${CONFIG_QEMU_ICOUNT_SHIFT},align=off,sleep=off
      -rtc clock=vm
    )
  endif()
endif()
