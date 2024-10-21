# Copyright (c) 2023 Nordic Semiconductor ASA
# Copyright (c) 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

find_package(BabbleSim)

zephyr_library()

if (CONFIG_BOARD_NRF54L15BSIM_NRF54L15_CPUFLPR)
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
	soc/nrfx_coredep.c
	common/bstests_entry.c
	common/cmsis/cmsis.c
	common/trace_hook.c
)

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

if (CONFIG_IPC_SERVICE AND CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP)
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

include(../common/natsim_config.cmake)
