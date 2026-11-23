# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Drive the guest clock from the instruction counter instead of the host clock,
# which makes emulated time deterministic.
#
# A board that needs a different -icount argument than the one derived from
# CONFIG_QEMU_ICOUNT_SHIFT can set QEMU_ICOUNT_OVERRIDE to the argument it wants,
# for example "auto".
#
# A board must not put its own -icount in QEMU_BOARD_FLAGS. QEMU merges
# repeated -icount options into a single option group in which the last shift=
# wins, and board flags are emitted before these, so the board's value would be
# silently discarded rather than applied. Catch that at configure time.
if("-icount" IN_LIST QEMU_BOARD_FLAGS)
  message(FATAL_ERROR
    "Board sets -icount in QEMU_BOARD_FLAGS, where it would be silently "
    "overridden by the CONFIG_QEMU_ICOUNT_SHIFT derived value. "
    "Set QEMU_ICOUNT_OVERRIDE instead."
  )
endif()

if(CONFIG_QEMU_ICOUNT)
  if(DEFINED QEMU_ICOUNT_OVERRIDE)
    set(icount_argument ${QEMU_ICOUNT_OVERRIDE})
  elseif(CONFIG_QEMU_ICOUNT_SLEEP)
    set(icount_argument shift=${CONFIG_QEMU_ICOUNT_SHIFT},align=off,sleep=on)
  else()
    set(icount_argument shift=${CONFIG_QEMU_ICOUNT_SHIFT},align=off,sleep=off)
  endif()

  qemu_append_flags(
    -icount ${icount_argument}
    -rtc clock=vm
  )
endif()
