/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_CPU_IF_H
#define NSI_COMMON_SRC_INCL_NSI_CPU_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nsi_cpu_if_internal.h"

/*
 * Any symbol annotated by these macros will be visible outside of the
 * embedded SW library, both by the native simulator runner,
 * and other possible embedded CPU's SW.
 */
#define NATIVE_SIMULATOR_IF_ATTR __attribute__((visibility("default")))

/*
 * NATIVE_SIMULATOR_IF_SECT() takes the linker section name to place the symbol
 * in: an ELF section name, or on Mach-O hosts the section part of a
 * "__TEXT,<sect>" specifier, which is limited to 16 characters.
 */
#if defined(__APPLE__)
#define NATIVE_SIMULATOR_IF_SECT(sect) \
	NATIVE_SIMULATOR_IF_ATTR NSI_SECTION_TEXT(sect)
#define NATIVE_SIMULATOR_IF NATIVE_SIMULATOR_IF_SECT("nsiif")
#define NATIVE_SIMULATOR_IF_DATA NATIVE_SIMULATOR_IF_ATTR NSI_SECTION_DATA("nsiifdat")
#define NATIVE_SIMULATOR_IF_TEXT NATIVE_SIMULATOR_IF_ATTR NSI_SECTION_TEXT("nsiiftxt")
#else
#define NATIVE_SIMULATOR_IF_SECT(sect) __attribute__((visibility("default"))) \
	__attribute__((__section__(sect)))
#define NATIVE_SIMULATOR_IF NATIVE_SIMULATOR_IF_SECT(".native_sim_if")
#define NATIVE_SIMULATOR_IF_DATA NATIVE_SIMULATOR_IF_SECT(".native_sim_if.data")
#define NATIVE_SIMULATOR_IF_TEXT NATIVE_SIMULATOR_IF_SECT(".native_sim_if.text")
#endif

/*
 * Implementation note:
 * The interface between the embedded SW and the native simulator is allocated in its
 * own section so the linker can retain those symbols when garbage collection is enabled.
 * On ELF this is controlled by the linker script; on Mach-O the section attributes
 * mark the symbols as not eligible for dead stripping.
 */


/*
 * Interfaces the Native Simulator _expects_ from the embedded CPUs:
 */

/*
 * Called during the earliest initialization (before command line parsing)
 *
 * The embedded SW library may provide this function to perform any
 * early initialization, including registering its own command line arguments
 * in the runner.
 */
NATIVE_SIMULATOR_IF void nsif_cpu0_pre_cmdline_hooks(void);

/*
 * Called during initialization (before the HW models are initialized)
 *
 * The embedded SW library may provide this function to perform any
 * early initialization, after the command line arguments have been parsed.
 */
NATIVE_SIMULATOR_IF void nsif_cpu0_pre_hw_init_hooks(void);

/*
 * Called by the runner to boot the CPU.
 *
 * The embedded SW library must provide this function.
 * This function is expected to return after the embedded CPU
 * has gone to sleep for the first time.
 *
 * The expectation is that the embedded CPU SW will spawn a
 * new pthread while in this call, and run the embedded SW
 * initialization in that pthread.
 *
 * It is recommended for the embedded SW to use the NCE (CPU start/stop emulation)
 * component to achieve this.
 */
NATIVE_SIMULATOR_IF void nsif_cpu0_boot(void);

/*
 * Called by the runner when the simulation is ending/exiting
 *
 * The embedded SW library may provide this function.
 * to do any cleanup it needs.
 */
NATIVE_SIMULATOR_IF int nsif_cpu0_cleanup(void);

/*
 * Called by the runner each time an interrupt is raised by the HW
 *
 * The embedded SW library must provide this function.
 * This function is expected to return after the embedded CPU
 * has gone back to sleep.
 */
NATIVE_SIMULATOR_IF void nsif_cpu0_irq_raised(void);

/*
 * Called by the runner each time an interrupt is raised in SW context itself.
 * That is, when a embedded SW action in the HW models, causes an immediate
 * interrupt to be raised (while the execution is still in the
 * context of the calling SW thread).
 */
NATIVE_SIMULATOR_IF void nsif_cpu0_irq_raised_from_sw(void);

/*
 * Optional hook which may be used for test functionality.
 * When the runner HW models use them and for what is up to those
 * specific models.
 */
NATIVE_SIMULATOR_IF int nsif_cpu0_test_hook(void *p);

/* Provide prototypes for all n instances of these hooks */
F_TRAMP_LIST(NATIVE_SIMULATOR_IF void nsif_cpu, _pre_cmdline_hooks(void))
F_TRAMP_LIST(NATIVE_SIMULATOR_IF void nsif_cpu, _pre_hw_init_hooks(void))
F_TRAMP_LIST(NATIVE_SIMULATOR_IF void nsif_cpu, _boot(void))
F_TRAMP_LIST(NATIVE_SIMULATOR_IF int nsif_cpu, _cleanup(void))
F_TRAMP_LIST(NATIVE_SIMULATOR_IF void nsif_cpu, _irq_raised(void))
F_TRAMP_LIST(NATIVE_SIMULATOR_IF void nsif_cpu, _irq_raised_from_sw(void))
F_TRAMP_LIST(NATIVE_SIMULATOR_IF int nsif_cpu, _test_hook(void *p))

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_CPU_IF_H */
