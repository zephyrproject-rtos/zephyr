# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS armfvp)
set(ARMFVP_BIN_NAME FVP_Base_RevC-2xAEMvA)

set(ARMFVP_FLAGS
  -C bp.secure_memory=0
  -C cluster0.NUM_CORES=${CONFIG_MP_MAX_NUM_CPUS}
  -C bp.refcounter.non_arch_start_at_default=1
  # UART0 config
  -C bp.pl011_uart0.out_file=-
  -C bp.pl011_uart0.unbuffered_output=1
  -C bp.terminal_0.start_telnet=0
  # UART1 config
  -C bp.pl011_uart1.out_file=-
  -C bp.pl011_uart1.unbuffered_output=1
  -C bp.terminal_1.start_telnet=0
  # UART2 config
  -C bp.pl011_uart2.out_file=-
  -C bp.pl011_uart2.unbuffered_output=1
  -C bp.terminal_2.start_telnet=0
  # UART3 config
  -C bp.pl011_uart3.out_file=-
  -C bp.pl011_uart3.unbuffered_output=1
  -C bp.terminal_3.start_telnet=0

  -C bp.vis.disable_visualisation=1
  -C bp.vis.rate_limit-enable=0
  -C gic_distributor.ARE-fixed-to-one=1
  -C gic_distributor.ITS-device-bits=16
  -C cache_state_modelled=0
  )

# Add ARMv9-A specific configuration flags for all ARMv9-A variants
if(CONFIG_ARMV9_A)
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    # Enable ARMv9.0 extension (includes all ARMv8.x features)
    -C cluster0.has_arm_v9-0=1
    # Enable SVE and SVE2 support (mandatory for ARMv9.0 compliance)
    -C cluster0.has_sve=1
    -C cluster0.sve.has_sve2=1
    -C cluster0.sve.sve2_version=2
    -C cluster0.sve.enable_at_reset=1
    # Enable enhanced PAC and BTI support (ARMv9.0 features)
    -C cluster0.enhanced_pac2_level=3
    -C cluster0.has_enhanced_pac=1
    )
  # Set minimum FVP version known to work with ARMv9.0 features (SVE2, enhanced PAC/BTI)
  set(ARMFVP_MIN_VERSION 11.29.27)
endif()

# Add Cortex-A320 specific configuration flags
if(CONFIG_BOARD_FVP_BASE_REVC_2XAEM_A320)
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    # Cortex-A320 specific CPU identification
    -C cluster0.MIDR=0x410FD8F0
    -C cluster0.AMIIDR=0xD8F0043B
    -C cluster0.AMPIDR=0x4000BBD8F
    -C cluster0.ERRIIDR=0xD8F0043B
    -C cluster0.ERRPIDR=0x4000BBD8F
    -C cluster0.PMUPIDR=0x4000BBD80
    -C cluster0.CTIPIDR=0x4003BBD8F
    -C cluster0.DBGPIDR=0x4003BBD8F

    # ARMv9.2-A support level
    -C cluster0.has_arm_v9-2=1

    # Advanced SIMD and crypto support
    -C cluster0.advsimd_bf16_support_level=1
    -C cluster0.advsimd_i8mm_support_level=1
    -C cluster0.cpu0.crypto_sha3=1
    -C cluster0.cpu0.crypto_sha512=1
    -C cluster0.cpu0.crypto_sm3=1
    -C cluster0.cpu0.crypto_sm4=1
    -C cluster0.cpu0.enable_crc32=1

    # Memory tagging support
    -C cluster0.memory_tagging_support_level=3

    # Enhanced security features
    -C cluster0.has_qarma3_pac=1
    -C cluster0.has_const_pac=2

    # SVE configuration for Cortex-A320
    -C cluster0.sve.veclen=2

    # Performance monitoring
    -C cluster0.pmu-num_counters=6
    -C cluster0.configure_pmu_events_with_json='{"pmu_events":["SVE_INST_RETIRED","BR_INDNR_TAKEN_RETIRED","BR_IND_RETIRED","EXC_IRQ","EXC_FIQ","EXC_RETURN","EXC_TAKEN","L1D_CACHE_RD","L2D_CACHE_RD","BUS_ACCESS_RD","BUS_ACCESS_WR","MEM_ACCESS_RD","MEM_ACCESS_WR","BR_PRED_RETIRED","BR_IMMED_MIS_PRED_RETIRED","BR_IND_MIS_PRED_RETIRED","BR_RETURN_PRED_RETIRED","BR_RETURN_MIS_PRED_RETIRED","BR_INDNR_PRED_RETIRED","BR_INDNR_MIS_PRED_RETIRED","BR_IMMED_PRED_RETIRED"]}'

    # Cache configuration
    -C cluster0.dcache-ways=4
    -C cluster0.icache-ways=4
    -C cluster0.l2cache-ways=8
    -C cluster0.icache-log2linelen=6
    -C cluster0.l2cache-read_bus_width_in_bytes=16
    -C cluster0.l2cache-write_bus_width_in_bytes=32

    # Debug and trace configuration
    -C cluster0.has_ets=1
    -C cluster0.has_trbe=1
    -C cluster0.has_self_hosted_trace_extension=2

    # Advanced architectural features
    -C cluster0.has_ccidx=1
    -C cluster0.has_16k_granule=1
    -C cluster0.ecv_support_level=2
    -C cluster0.has_cvadp_support=1
    -C cluster0.has_lrcpc=1
    -C cluster0.has_dot_product=2
    -C cluster0.has_wfet_and_wfit=2
    -C cluster0.has_xs=2
    -C cluster0.has_v8_5_debug_over_power_down=2
    -C cluster0.has_no_os_double_lock=1
    -C cluster0.has_v8_7_fp_enhancements=2
    -C cluster0.has_amu=1
    -C cluster0.has_mpmm=1
    -C cluster0.has_mpam=2
    -C cluster0.has_ras=2

    # Memory system configuration
    -C cluster0.stage12_tlb_size=1024
    -C cluster0.restriction_on_speculative_execution=2
    -C cluster0.restriction_on_speculative_execution_aarch32=2

    # Error handling
    -C cluster0.number_of_error_records=3
    -C cluster0.ERXMISC0_mask=0xC003FFC3
    )
  # Set minimum FVP version for Cortex-A320 features
  set(ARMFVP_MIN_VERSION 11.29.27)
endif()

if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "fvp")

  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(FVP_SECURE_FLASH_FILE ${TFA_BINARY_DIR}/fvp/debug/bl1.bin)
    set(FVP_FLASH_FILE ${TFA_BINARY_DIR}/fvp/debug/fip.bin)
  else()
    set(FVP_SECURE_FLASH_FILE ${TFA_BINARY_DIR}/fvp/release/bl1.bin)
    set(FVP_FLASH_FILE ${TFA_BINARY_DIR}/fvp/release/fip.bin)
  endif()

elseif(CONFIG_PM_CPU_OPS_FVP)
  # Configure RVBAR_EL3 for bare metal SMP when using FVP PM CPU ops driver
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    # Set RVBAR_EL3 for secondary CPUs to Zephyr image base address
    -C cluster0.cpu1.RVBAR=${CONFIG_KERNEL_VM_BASE}
    -C cluster0.cpu2.RVBAR=${CONFIG_KERNEL_VM_BASE}
    -C cluster0.cpu3.RVBAR=${CONFIG_KERNEL_VM_BASE}
    )
endif()
