/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INIT_H_
#define ZEPHYR_INCLUDE_INIT_H_

#include <stdint.h>
#include <stddef.h>

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys_init System Initialization
 * @since 1.0
 * @version 1.0.0
 * @ingroup os_services
 *
 * Zephyr offers an infrastructure to call initialization code before `main`.
 * Such initialization calls can be registered using SYS_INIT() or
 * SYS_INIT_NAMED() macros. By using a combination of initialization levels and
 * priorities init sequence can be adjusted as needed. The available
 * initialization levels are described, in order, below:
 *
 * - `EARLY`: Used very early in the boot process, right after entering the C
 *   domain (``z_cstart()``). This can be used in architectures and SoCs that
 *   extend or implement architecture code and use drivers or system services
 *   that have to be initialized before the Kernel calls any architecture
 *   specific initialization code.
 * - `PRE_KERNEL`: Executed in Kernel's initialization context, which uses
 *   the interrupt stack. At this point Kernel services are not yet available.
 * - `PRE_KERNEL_1`: @deprecated Alias of `PRE_KERNEL`. Entries registered at
 *   this level share the `PRE_KERNEL` level and are ordered together with its
 *   entries, by priority. Use `PRE_KERNEL` instead.
 * - `PRE_KERNEL_2`: @deprecated Same execution context as `PRE_KERNEL`; all
 *   its entries run after every `PRE_KERNEL` (and `PRE_KERNEL_1`) entry.
 *   Migrate to `PRE_KERNEL` with a priority that orders the entry after its
 *   dependencies.
 * - `POST_KERNEL`: Executed after Kernel is alive. From this point on, Kernel
 *   primitives can be used.
 * - `APPLICATION`: Executed after the Kernel is alive, before the static
 *   threads are started and, on SMP systems, before the secondary CPUs are
 *   brought up.
 * - `PRE_MAIN`: Executed on the boot thread as the last step before the
 *   application `main()` is entered. At this point the system is fully up:
 *   the static threads have been created and, if @kconfig{CONFIG_SMP} is
 *   enabled, every secondary CPU started at boot is online (bringing up a
 *   secondary CPU can be deferred to run time, in which case it is not).
 * - `SMP`: @deprecated Alias of `PRE_MAIN`. Entries registered at this level
 *   share the `PRE_MAIN` level and are ordered together with its entries, by
 *   priority. Unlike the old `SMP` level, `PRE_MAIN` is not conditioned on
 *   @kconfig{CONFIG_SMP}. Use `PRE_MAIN` instead.
 *
 * Initialization priority can take a value in the range of 0 to 999.
 *
 * Besides a hand-picked numeric priority, an init entry can be ordered
 * automatically, at build time, by one of the following means:
 *
 * - **Anchors**: a service registered with SYS_INIT_ANCHORED() leaves a named
 *   anchor other services can order themselves after (see SYS_ANCHOR() and
 *   SYS_ANCHOR_AFTER()). Ordering is resolved entirely at link time; there is
 *   no runtime dependency tracking and no footprint beyond the init entry
 *   itself.
 * - **Consuming a device**: SYS_INIT_DEPENDS() orders an init function right
 *   after the device of a given devicetree node.
 * - **Automatic device ordering**: DEVICE_DT_DEFINE_AUTO() orders a device by
 *   its devicetree dependency ordinal instead of a numeric priority.
 *
 * Within one level, all numeric-priority entries run first, then all
 * automatically ordered entries (devicetree-ordinal ones first, anchored
 * services last).
 *
 * @note The same infrastructure is used by devices.
 * @{
 */

struct device;

/**
 * @brief Structure to store initialization entry information.
 *
 * @internal
 * Init entries need to be defined following these rules:
 *
 * - Their name must be set using Z_INIT_ENTRY_NAME().
 * - They must be placed in a special init section, given by
 *   Z_INIT_ENTRY_SECTION().
 * - They must be aligned, e.g. using Z_DECL_ALIGN().
 *
 * See SYS_INIT_NAMED() for an example.
 * @endinternal
 */
struct init_entry {
	/**
	 * If the init function belongs to a SYS_INIT, this field stored the
	 * initialization function, otherwise it is set to NULL.
	 */
	int (*init_fn)(void);
	/**
	 * If the init entry belongs to a device, this fields stores a
	 * reference to it, otherwise it is set to NULL.
	 */
	const struct device *dev;
};

/** @cond INTERNAL_HIDDEN */

/* Helper definitions to evaluate level equality */
#define Z_INIT_EARLY_EARLY		 1
#define Z_INIT_PRE_KERNEL_PRE_KERNEL	 1
#define Z_INIT_PRE_KERNEL_1_PRE_KERNEL_1 1
#define Z_INIT_PRE_KERNEL_2_PRE_KERNEL_2 1
#define Z_INIT_POST_KERNEL_POST_KERNEL	 1
#define Z_INIT_APPLICATION_APPLICATION	 1
#define Z_INIT_PRE_MAIN_PRE_MAIN	 1
#define Z_INIT_SMP_SMP			 1

/* Init level ordinals. A deprecated alias shares the ordinal of the level it
 * aliases: PRE_KERNEL_1 with PRE_KERNEL, and SMP with PRE_MAIN.
 */
#define Z_INIT_ORD_EARLY	0
#define Z_INIT_ORD_PRE_KERNEL	1
#define Z_INIT_ORD_PRE_KERNEL_1 1
#define Z_INIT_ORD_PRE_KERNEL_2 2
#define Z_INIT_ORD_POST_KERNEL	3
#define Z_INIT_ORD_APPLICATION	4
#define Z_INIT_ORD_PRE_MAIN	5
#define Z_INIT_ORD_SMP		5

/* Map an init level token to the linker section its entries are placed in.
 * PRE_KERNEL_1 is a deprecated alias of PRE_KERNEL: both share one section
 * and their entries are ordered together, by priority. PRE_KERNEL_2 keeps a
 * section of its own, which the linker places right after the PRE_KERNEL
 * one, preserving the legacy "all PRE_KERNEL_1 entries run before any
 * PRE_KERNEL_2 entry" guarantee while PRE_KERNEL_2 is being phased out.
 * SMP is a deprecated alias of PRE_MAIN and shares its section.
 */
#define Z_INIT_SECTION_LEVEL_EARLY	  EARLY
#define Z_INIT_SECTION_LEVEL_PRE_KERNEL	  PRE_KERNEL
#define Z_INIT_SECTION_LEVEL_PRE_KERNEL_1 PRE_KERNEL
#define Z_INIT_SECTION_LEVEL_PRE_KERNEL_2 PRE_KERNEL_2
#define Z_INIT_SECTION_LEVEL_POST_KERNEL  POST_KERNEL
#define Z_INIT_SECTION_LEVEL_APPLICATION  APPLICATION
#define Z_INIT_SECTION_LEVEL_PRE_MAIN	  PRE_MAIN
#define Z_INIT_SECTION_LEVEL_SMP	  PRE_MAIN

/**
 * @brief Obtain init entry name.
 *
 * @param init_id Init entry unique identifier.
 */
#define Z_INIT_ENTRY_NAME(init_id) _CONCAT(__init_, init_id)

/**
 * @brief Init entry section.
 *
 * Each init entry is placed in a section with a name crafted so that it allows
 * linker scripts to sort them according to the specified
 * level/priority/sub-priority.
 */
#define Z_INIT_ENTRY_SECTION(level, prio, sub_prio)                                                \
	__attribute__((__section__(                                                                \
		".z_init_" STRINGIFY(_CONCAT(Z_INIT_SECTION_LEVEL_, level))                        \
		"_P_" STRINGIFY(prio) "_SUB_" STRINGIFY(sub_prio)"_")))

/**
 * @brief Init entry section for anchored entries.
 *
 * Anchored entries carry a string sort key instead of a numeric priority.
 * They are placed in the level's automatic-ordering band (`P_AUTO`, which the
 * linker script sorts after every numeric priority) with the key as
 * sub-priority, so the linker's lexical sort yields the dependency order:
 * a service's key is its dependency's key extended with `"~" "<name>"`, and
 * `~` (0x7e) compares greater than any C identifier character, making every
 * extension of a key sort after the key itself.
 */
#define Z_INIT_ENTRY_SECTION_KEYED(level, key)                                                     \
	__attribute__((__section__(                                                                \
		".z_init_" STRINGIFY(_CONCAT(Z_INIT_SECTION_LEVEL_, level))                        \
		"_P_AUTO_SUB_" key "_")))

/**
 * @brief Emit a validation record for an anchored init entry.
 *
 * Records the entry's level and full anchor key as a string in a
 * non-allocated section (like debug info: present in the ELF for build-time
 * tooling, never loaded, absent from binary outputs). The records are
 * consumed by scripts/build/check_init_priorities.py, which verifies that
 * every dependency named in a key is linked in and does not run at a later
 * level than its dependent.
 */
#define Z_SYS_INIT_ANCHOR_RECORD(level, key)                                                       \
	__asm__(PUSHSECTION_DIRECTIVE " .zinit_anchor_info,\"\"\n\t"                          \
		".asciz \"" STRINGIFY(_CONCAT(Z_INIT_SECTION_LEVEL_, level)) ":" key "\"\n\t"     \
		POPSECTION_DIRECTIVE)

/** @endcond */

/**
 * @brief Obtain the ordinal for an init level.
 *
 * @param level Init level (EARLY, PRE_KERNEL, POST_KERNEL, APPLICATION,
 * PRE_MAIN, and the deprecated PRE_KERNEL_1, PRE_KERNEL_2 and SMP).
 *
 * @return Init level ordinal.
 */
#define INIT_LEVEL_ORD(level)                                                  \
	COND_CASE_1(Z_INIT_EARLY_##level, (Z_INIT_ORD_EARLY),                  \
		    Z_INIT_PRE_KERNEL_##level, (Z_INIT_ORD_PRE_KERNEL),        \
		    Z_INIT_PRE_KERNEL_1_##level, (Z_INIT_ORD_PRE_KERNEL_1),    \
		    Z_INIT_PRE_KERNEL_2_##level, (Z_INIT_ORD_PRE_KERNEL_2),    \
		    Z_INIT_POST_KERNEL_##level, (Z_INIT_ORD_POST_KERNEL),      \
		    Z_INIT_APPLICATION_##level, (Z_INIT_ORD_APPLICATION),      \
		    Z_INIT_PRE_MAIN_##level, (Z_INIT_ORD_PRE_MAIN),            \
		    Z_INIT_SMP_##level, (Z_INIT_ORD_SMP),                      \
		    (ZERO_OR_COMPILE_ERROR(0)))

/**
 * @brief Register an initialization function.
 *
 * The function will be called during system initialization according to the
 * given level and priority.
 *
 * @note The return value of the initialization function is ignored.
 *
 * @param init_fn Initialization function.
 * @param level Initialization level. Allowed tokens: `EARLY`, `PRE_KERNEL`,
 * `POST_KERNEL`, `APPLICATION` and `PRE_MAIN`. The tokens `PRE_KERNEL_1`
 * (alias of `PRE_KERNEL`), `PRE_KERNEL_2` and `SMP` (alias of `PRE_MAIN`)
 * are deprecated but still accepted for compatibility.
 * @param prio Initialization priority within @p _level. Note that it must be a
 * decimal integer literal without leading zeroes or sign (e.g. `32`), or an
 * equivalent symbolic name (e.g. `#define MY_INIT_PRIO 32`); symbolic
 * expressions are **not** permitted (e.g.
 * `CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 5`).
 */
#define SYS_INIT(init_fn, level, prio)                                         \
	SYS_INIT_NAMED(init_fn, init_fn, level, prio)

/**
 * @brief Register an initialization function (named).
 *
 * @note This macro can be used for cases where the multiple init calls use the
 * same init function.
 *
 * @note The return value of the initialization function is ignored.
 *
 * @param name Unique name for SYS_INIT entry.
 * @param init_fn_ See SYS_INIT().
 * @param level See SYS_INIT().
 * @param prio See SYS_INIT().
 *
 * @see SYS_INIT()
 */
#define SYS_INIT_NAMED(name, init_fn_, level, prio)                                       \
	static const Z_DECL_ALIGN(struct init_entry)                                      \
		Z_INIT_ENTRY_SECTION(level, prio, 0) __used __noasan                      \
		Z_INIT_ENTRY_NAME(name) = {.init_fn = (init_fn_), .dev = NULL}            \

/**
 * @brief Build the anchor key of a service without dependencies.
 *
 * A service registered with SYS_INIT_ANCHORED() leaves a named anchor that
 * other services can order themselves after. The anchor is a build-time sort
 * key published as a macro named `SYS_ANCHOR_<name>`, conventionally in the
 * service's header (next to its API), so that depending on a service means
 * including its header:
 *
 * @code{.c}
 * #define SYS_ANCHOR_settings SYS_ANCHOR(settings)
 * @endcode
 *
 * @param name Service name; must match the @p name later passed to
 * SYS_INIT_ANCHORED().
 */
#define SYS_ANCHOR(name) STRINGIFY(name)

/**
 * @brief Build the anchor key of a service that runs after another service.
 *
 * Extends the dependency's anchor key, ordering this service after it. The
 * dependency's anchor macro must be visible (include the header of the
 * service depended on); a missing or misspelled dependency is a compile
 * error.
 *
 * @code{.c}
 * #define SYS_ANCHOR_shell SYS_ANCHOR_AFTER(SYS_ANCHOR_settings, shell)
 * @endcode
 *
 * @note The dependency is passed as its anchor macro (`SYS_ANCHOR_<dep>`),
 * not its name: the key must expand in macro argument position for
 * arbitrarily deep dependency chains to expand fully.
 *
 * A service with multiple dependencies extends the key of its *dominant*
 * dependency (the one initialized last); ordering against the others must
 * hold by construction of their keys.
 *
 * @param dep_key Anchor key macro of the service this one runs after.
 * @param name Service name; must match the @p name later passed to
 * SYS_INIT_ANCHORED().
 */
#define SYS_ANCHOR_AFTER(dep_key, name) dep_key "~" STRINGIFY(name)

/**
 * @brief Register a service initialization function, ordered by its anchor.
 *
 * Registers @p init_fn_ to run at @p level like SYS_INIT(), but instead of a
 * hand-picked numeric priority the entry is ordered by the anchor key
 * `SYS_ANCHOR_<name>`, which must be defined (see SYS_ANCHOR() and
 * SYS_ANCHOR_AFTER()) before this macro is used.
 *
 * Ordering is resolved entirely at build time by the linker: an anchored
 * entry is a plain @ref init_entry run by the regular init loop, so this
 * costs no more ROM/RAM than SYS_INIT() and adds no boot-time work.
 *
 * Anchored entries run after every numeric-priority and devicetree-ordered
 * entry of the same level. A dependency must not live in a later level than
 * its dependents.
 *
 * @code{.c}
 * #define SYS_ANCHOR_settings SYS_ANCHOR(settings)
 * SYS_INIT_ANCHORED(settings, settings_init, POST_KERNEL);
 *
 * #define SYS_ANCHOR_shell SYS_ANCHOR_AFTER(SYS_ANCHOR_settings, shell)
 * SYS_INIT_ANCHORED(shell, shell_init, POST_KERNEL);
 * @endcode
 *
 * @param name Service name; `SYS_ANCHOR_<name>` must be its anchor key.
 * @param init_fn_ Initialization function, see SYS_INIT().
 * @param level Initialization level, see SYS_INIT().
 */
#define SYS_INIT_ANCHORED(name, init_fn_, level)                                          \
	Z_SYS_INIT_ANCHOR_RECORD(level, _CONCAT(SYS_ANCHOR_, name));                      \
	static const Z_DECL_ALIGN(struct init_entry)                                      \
		Z_INIT_ENTRY_SECTION_KEYED(level, _CONCAT(SYS_ANCHOR_, name))             \
		__used __noasan Z_INIT_ENTRY_NAME(name) = {                               \
			.init_fn = (init_fn_), .dev = NULL}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_INIT_H_ */
