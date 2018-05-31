set(BOARD_FLASH_RUNNER intel_s1000)
set(BOARD_DEBUG_RUNNER intel_s1000)

board_finalize_runner_args(intel_s1000
  "--xt-ocd-dir=/opt/Tensilica/xocd-12.0.4/xt-ocd"
  "--ocd-topology=topology_dsp0_flyswatter2.xml"
  "--ocd-jtag-instr=dsp0_gdb.txt"
  "--gdb-flash-file=load_elf.txt"
)
