# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
  # GNU compiler options

  message(FATAL_ERROR "ARC VPX targets can be built with ARC MWDT toolchain only")

else()
  # MWDT compiler options

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_VPX5 -arcv2hs -core4 -uarch_rev=1:4 -Xcode_density
             -HL -Xatomic -Xll64 -Xunaligned -Xdiv_rem=radix4 -Xswap -Xbitscan -Xmpy_option=qmpyh
             -Xshift_assist -Xbarrel_shifter -Xtimer0 -Xtimer1 -Xrtc -dcache=32768,64,2,a
             -Hld_cycles=1 -DDCCM_SYSTEM_BASE_CORE0=0x80000000 -Hccm
             -DICCM0_SYSTEM_BASE_CORE0=0x0000000 -Xstu=4 -Xvdsp4 -Xvec_unit_rev_minor=1
             -Xvec_width=512 -Xvec_mem_size=256k -Xvec_mem_bank_width=16 -Xvec_max_fetch_size=16
             -Xvec_num_slots=3 -Xvec_super_with_scalar -Xvec_regs=40 -Xvec_num_rd_ports=6
             -Xvec_num_acc=8 -Xvec_num_mpy=2 -Xvec_mpy32 -Xvec_num_alu=3 -Xvec_guard_bit_option=2
             -Xvec_stack_check -DVEC_MEM_SYS_BASE_CORE0=0xb4000000)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_VPX5 -Hlib=vpx5_integer_full)

endif()
