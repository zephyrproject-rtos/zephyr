set(BOARD_FLASH_RUNNER intel_s1000)
set(BOARD_DEBUG_RUNNER intel_s1000)

board_finalize_runner_args(intel_s1000
  "--board-dir=${ZEPHYR_BASE}/boards/xtensa/intel_s1000_crb/"
  "--xt-ocd-dir=/opt/Tensilica/xocd-12.0.4/xt-ocd"
  "--ocd-topology=topology_dsp0_flyswatter2.xml"
  "--ocd-jtag-instr=dsp0_gdb.txt"
  "--gdb-flash-file=load_elf.txt"
  "--gdb=${TOOLCHAIN_HOME}/bin/xt-gdb"
  "--kernel-elf=zephyr/zephyr.elf"
)
