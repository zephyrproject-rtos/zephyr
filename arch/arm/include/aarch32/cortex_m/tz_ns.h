/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TrustZone API for use in nonsecure firmware
 *
 * TrustZone API for Cortex-M CPUs implementing the Security Extension.
 * The following API can be used by the nonsecure firmware to interact with the
 * secure firmware.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_TZ_NS_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_TZ_NS_H_

#ifdef _ASMLANGUAGE

/* nothing */

#else

/**
 * @brief Macro for "sandwiching" a function call (@p name) between two other
 *	  calls
 *
 * This macro should be called via @ref __TZ_WRAP_FUNC.
 *
 * This macro creates the function body of an "outer" function which behaves
 * exactly like the wrapped function (@p name), except that the preface function
 * is called before, and the postface function afterwards.
 *
 * @param preface   The function to call first. Must have no parameters and no
 *                  return value.
 * @param name      The main function, i.e. the function to wrap. This function
 *                  will receive the arguments, and its return value will be
 *                  returned.
 * @param postface  The function to call last. Must have no parameters and no
 *                  return value.
 * @param store_lr  The assembly instruction for storing away the LR value
 *                  before the functions are called. This instruction must leave
 *                  r0-r3 unmodified.
 * @param load_lr   The assembly instruction for restoring the LR value after
 *                  the functions have been called. This instruction must leave
 *                  r0-r3 unmodified.
 */
#define __TZ_WRAP_FUNC_RAW(preface, name, postface, store_lr, load_lr) \
	do { \
		__asm__ volatile( \
			".global "#preface"; .type "#preface", %function"); \
		__asm__ volatile( \
			".global "#name"; .type "#name", %function"); \
		__asm__ volatile( \
			".global "#postface"; .type "#postface", %function"); \
		__asm__ volatile( \
			store_lr "\n\t" \
			"push {r0-r3}\n\t" \
			"bl " #preface "\n\t" \
			"pop {r0-r3}\n\t" \
			"bl " #name " \n\t" \
			"push {r0-r3}\n\t" \
			"bl " #postface "\n\t" \
			"pop {r0-r3}\n\t" \
			load_lr "\n\t" \
			::); \
	} while (0)

/**
 * @brief Macro for "sandwiching" a function call (@p name) in two other calls
 *
 * @pre The wrapped function MUST not pass arguments or return values via
 *      the stack. I.e. the arguments and return values must each fit within 4
 *      words, after accounting for alignment.
 *      Since nothing is passed on the stack, the stack can safely be used to
 *      store LR.
 *
 * Usage example:
 *
 *	int foo(char *arg); // Implemented elsewhere.
 *	int __attribute__((naked)) foo_wrapped(char *arg)
 *	{
 *		__TZ_WRAP_FUNC(bar, foo, baz)
 *	}
 *
 * is equivalent to
 *
 *	int foo(char *arg); // Implemented elsewhere.
 *	int foo_wrapped(char *arg)
 *	{
 *		bar();
 *		int res = foo(arg);
 *		baz();
 *		return res;
 *	}
 *
 * @note __attribute__((naked)) is not mandatory, but without it, GCC gives a
 *       warning for functions with a return value. It also reduces flash use.
 *
 * See @ref __TZ_WRAP_FUNC_RAW for more information.
 */
#define __TZ_WRAP_FUNC(preface, name, postface) \
	__TZ_WRAP_FUNC_RAW(preface, name, postface, "push {r4, lr}", \
			"pop {r4, pc}")


#ifdef CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS
/**
 * @brief Create a thread safe wrapper function for a non-secure entry function
 *
 * This locks the scheduler before calling the function by wrapping the NS entry
 * function in @ref k_sched_lock / @ref k_sched_unlock, using
 * @ref __TZ_WRAP_FUNC.
 *
 * In non-secure code:
 *
 *	int foo(char *arg); // Declaration of entry function.
 *	TZ_THREAD_SAFE_NONSECURE_ENTRY_FUNC(foo_safe, int, foo, char *arg)
 *
 * Usage in non-secure code:
 *
 *	int ret = foo_safe("my arg");
 *
 * If NS entry functions are called without such a wrapper, and a thread switch
 * happens while execution is in the secure binary, the app will possibly crash
 * upon returning to the non-secure binary.
 *
 * @param ret   The return type of the NS entry function.
 * @param name  The desired name of the safe function. This assumes there is a
 *              corresponding NS entry function called nsc_name.
 * @param ...   The rest of the signature of the function. This must be the same
 *              signature as the corresponding NS entry function.
 */
#define TZ_THREAD_SAFE_NONSECURE_ENTRY_FUNC(name, ret, nsc_name, ...) \
	ret __attribute__((naked)) name(__VA_ARGS__) \
	{ \
		__TZ_WRAP_FUNC(k_sched_lock, nsc_name, k_sched_unlock); \
	}

#endif /* CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS */

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_TZ_NS_H_ */
