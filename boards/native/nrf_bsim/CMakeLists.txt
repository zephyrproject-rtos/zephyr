# Copyright (c) 2023 Nordic Semiconductor ASA
# Copyright (c) 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

find_package(BabbleSim)

zephyr_library()

if(CONFIG_BOARD_NRF54L15BSIM_NRF54L15_CPUFLPR)
  message(FATAL_ERROR "Targeting the nrf54l15bsim/nrf54l15/cpuflpr core is not yet supported")
endif()

# Due to the BLE controller assumption about enum size
zephyr_compile_options(
  -fshort-enums
)
# Structures layouts needs to match in the interface between the runner and the embedded SW
# The nrfx HAL uses enums in its definitions,so they need to have the same size in both,
# as both the HW models and embedded SW use them.
target_compile_options(native_simulator INTERFACE -fshort-enums)

zephyr_library_sources(
  irq_handler.c
  cpu_wait.c
  argparse.c
  nsi_if.c
  native_remap.c
  gpiote_nrfx_bsim.c
  soc/nrfx_coredep.c
  common/bstests_entry.c
  common/cmsis/cmsis.c
  common/trace_hook.c
)

# Include GPIOTE nrfx from real SOC code
zephyr_library_sources(${ZEPHYR_BASE}/soc/nordic/common/gpiote_nrfx.c)

# Include gppi_init from real SOC code if enabled
if(CONFIG_NRFX_GPPI)
  zephyr_library_sources(${ZEPHYR_BASE}/soc/nordic/common/gppi_init.c)
endif()

# Include sync_rtc from real SOC code if enabled
zephyr_library_sources_ifdef(CONFIG_NRF53_SYNC_RTC
  ${ZEPHYR_BASE}/soc/nordic/nrf53/sync_rtc.c
  )

target_sources(native_simulator INTERFACE
  common/bsim_args_runner.c
  common/bsim_extra_cpu_if_stubs.c
  common/phy_sync_ctrl.c
  common/runner_hooks.c
  common/posix_arch_if.c
  common/trace_hook.c
)

if(CONFIG_IPC_SERVICE AND CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP)
  zephyr_library_sources(
    ipc_backend.c
  )
endif()

zephyr_include_directories(
  soc
  common
  common/cmsis
  ${NSI_DIR}/common/src/include
  ${ZEPHYR_BASE}/soc/nordic/common
)

zephyr_library_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  ${BSIM_COMPONENTS_PATH}/libRandv2/src/
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/posix/include
  common/
)

set(libpath ${BSIM_OUT_PATH}/lib)
set_property(TARGET native_simulator APPEND PROPERTY RUNNER_LINK_LIBRARIES
  ${libpath}/libUtilv1.32.a
  ${libpath}/libPhyComv1.32.a
  ${libpath}/lib2G4PhyComv1.32.a
  ${libpath}/libRandv2.32.a
)

target_compile_options(native_simulator INTERFACE
                       "-DNSI_PRIMARY_MCU_N=${CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX}")

add_subdirectory(${ZEPHYR_BASE}/boards/native/common/extra_args/
  ${CMAKE_CURRENT_BINARY_DIR}/extra_args
)

if(CONFIG_DT_HAS_ZEPHYR_BSIM_2G4_RADIO_ENABLED)
  add_subdirectory("${ZEPHYR_BASE}/boards/native/common/bsim_2g4_radio"
    "${CMAKE_CURRENT_BINARY_DIR}/bsim_2g4_radio"
  )
  target_sources(native_simulator INTERFACE
    "common/bs_2g4_radio_nrf_bsim.c"
  )

  # Tests set the test cheats of the nRF RADIO model, which then have to
  # reach the radio model in use.
  target_link_options(native_simulator INTERFACE
    -Wl,--wrap=hw_radio_testcheat_set_tx_power_gain
    -Wl,--wrap=hw_radio_testcheat_set_rx_power_gain
    -Wl,--wrap=hw_radio_testcheat_disable_tx
    -Wl,--wrap=hw_radio_testcheat_disable_rx
  )

  # The radio model may be given the interrupt line of RTC0, which the driver
  # of an enabled RTC0 would take too.
  dt_comp_path(bsr_path COMPATIBLE "zephyr,bsim-2g4-radio")
  dt_prop(bsr_irqs PATH "${bsr_path}" PROPERTY "interrupts")
  dt_nodelabel(rtc0_path NODELABEL "rtc0")
  if(DEFINED rtc0_path)
    dt_node_has_status(rtc0_okay PATH "${rtc0_path}" STATUS okay)
    dt_prop(rtc0_irq PATH "${rtc0_path}" PROPERTY "interrupts" INDEX 0)
  endif()
  if(rtc0_okay)
    list(LENGTH bsr_irqs bsr_irqs_len)
    math(EXPR bsr_irqs_last "${bsr_irqs_len} - 1")
    # The interrupts property holds <irq priority> pairs
    foreach(i RANGE 0 ${bsr_irqs_last} 2)
      list(GET bsr_irqs ${i} irq)
      if(irq EQUAL rtc0_irq)
        message(FATAL_ERROR "zephyr,bsim-2g4-radio uses interrupt ${irq} of the enabled RTC0")
      endif()
    endforeach()
  endif()
endif()

include(../common/natsim_config.cmake)
