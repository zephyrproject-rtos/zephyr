# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0
#
# GDB helper that restarts a CM55 debug session on kit_pse84_eval.
#
# The CM55 application is not started by a chip reset: it is released by the
# CM33 `enable_cm55` application calling Cy_SysEnableCM55(). A plain GDB `run`
# performs a blind chip reset, which leaves the CM55 parked in the ROM
# "safe" endless-loop (pc=0x3ff00) instead of at the application reset
# handler, so `run` cannot restart the CM55.
#
# The `restart` command below replays the exact OpenOCD gdb-attach sequence
# that the initial `west debug` connection uses: it reconnects to the CM33
# gdb server (which resets and halts the CM33 at its reset vector), then to
# the CM55 gdb server (whose gdb-attach event runs `reset_halt cm55`, resuming
# the now-halted CM33 so the enable_cm55 application releases the CM55 and the
# CM55 halts at its reset handler).
#
# Use `restart` instead of `run` to restart a CM55 debug session.

define restart
  disconnect
  target extended-remote :3333
  disconnect
  target extended-remote :3334
  maintenance flush register-cache
end

document restart
Restart the CM55 debug session.

Use this instead of `run`, which cannot restart the CM55 on this board.
After `restart` the CM55 is halted at its reset handler; use `continue` to
run to your breakpoints.
end
