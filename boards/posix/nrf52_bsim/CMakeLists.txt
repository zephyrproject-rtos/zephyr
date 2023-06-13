# SPDX-License-Identifier: Apache-2.0

find_package(BabbleSim)

zephyr_library()
zephyr_library_compile_definitions(NO_POSIX_CHEATS)

#Due to the BLE controller assumption about enum size
zephyr_compile_options(
  -fshort-enums
)

zephyr_library_sources(
	irq_handler.c
	cpu_wait.c
	bstests_entry.c
	argparse.c
	main.c
	time_machine.c
	trace_hook.c
	cmsis/cmsis.c
	soc/nrfx_coredep.c
)

zephyr_include_directories(soc)
zephyr_include_directories(cmsis)

zephyr_library_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  ${BSIM_COMPONENTS_PATH}/libRandv2/src/
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/posix/include
)

zephyr_ld_options(
  -lm
)

set(libpath ${BSIM_OUT_PATH}/lib)
zephyr_library_import(bsim_libUtilv1      ${libpath}/libUtilv1.32.a)
zephyr_library_import(bsim_libPhyComv1    ${libpath}/libPhyComv1.32.a)
zephyr_library_import(bsim_lib2G4PhyComv1 ${libpath}/lib2G4PhyComv1.32.a)
zephyr_library_import(bsim_libRandv2      ${libpath}/libRandv2.32.a)
