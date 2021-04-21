# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(intel_s1000)
board_set_debugger_ifnset(intel_s1000)

if(CONFIG_SMP)
board_finalize_runner_args(intel_s1000
  "--xt-ocd-dir=/opt/tensilica/xocd-12.0.4/xt-ocd"
  "--ocd-topology=topology_all_flyswatter2.xml"
  "--ocd-jtag-instr=all_gdb.txt"
  "--gdb-flash-file=load_elf.txt"
)
else()
board_finalize_runner_args(intel_s1000
  "--xt-ocd-dir=/opt/tensilica/xocd-12.0.4/xt-ocd"
  "--ocd-topology=topology_dsp0_flyswatter2.xml"
  "--ocd-jtag-instr=dsp0_gdb.txt"
  "--gdb-flash-file=load_elf.txt"
)
endif()
