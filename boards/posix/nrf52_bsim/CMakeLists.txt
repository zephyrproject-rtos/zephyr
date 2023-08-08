# Copyright (c) 2023 Nordic Semiconductor ASA
# Copyright (c) 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

find_package(BabbleSim)

zephyr_library()

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
	soc/nrfx_coredep.c
	common/bstests_entry.c
	common/cmsis/cmsis.c
)

target_sources(native_simulator INTERFACE
	common/bsim_args_runner.c
	common/phy_sync_ctrl.c
	common/runner_hooks.c
	common/posix_arch_if.c
	common/trace_hook.c
)

zephyr_include_directories(
  soc
  common
  common/cmsis
  ${NSI_DIR}/common/src/include
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
target_link_options(native_simulator INTERFACE
	${libpath}/libUtilv1.32.a
	${libpath}/libPhyComv1.32.a
	${libpath}/lib2G4PhyComv1.32.a
	${libpath}/libRandv2.32.a
)

set(zephyr_build_path ${CMAKE_BINARY_DIR}/zephyr)

set(nsi_config_content
  "NSI_BUILD_OPTIONS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_COMPILE_OPTIONS>,\ >"
  "NSI_BUILD_PATH:=${zephyr_build_path}/NSI"
  "NSI_CC:=${CMAKE_C_COMPILER}"
  "NSI_EMBEDDED_CPU_SW:=${zephyr_build_path}/${KERNEL_ELF_NAME}"
  "NSI_EXE:=${zephyr_build_path}/${KERNEL_EXE_NAME}"
  "NSI_EXTRA_SRCS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_SOURCES>,\ >"
  "NSI_LINK_OPTIONS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_LINK_OPTIONS>,\ >"
  "NSI_PATH:=${NSI_DIR}/"
)

string(REPLACE ";" "\n" nsi_config_content "${nsi_config_content}")

file(GENERATE OUTPUT "${zephyr_build_path}/NSI/nsi_config"
  CONTENT "${nsi_config_content}"
)
