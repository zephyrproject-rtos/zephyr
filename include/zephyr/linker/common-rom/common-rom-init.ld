/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_TOOLCHAIN_SUPPORTS_STATIC_INIT_GNU) || defined(CONFIG_NATIVE_APPLICATION)
	SECTION_PROLOGUE(init_array,,)
	{
		/*
		 * Add all of the GNU-style constructors in priority order. Note
		 * that this doesn't build the ctors in the "usual" fashion with
		 * a length value first and NULL terminator, but we're creating
		 * an init_array style list and leaving the ctors list empty
		 */
#ifdef CONFIG_NATIVE_APPLICATION
		/* Use the native LIBC constructor code so that any native
		 * constructors get run before main is invoked
		 */
		__init_array_start = .;
#else
		__zephyr_init_array_start = .;
#endif
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)
			SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
#ifdef CONFIG_NATIVE_APPLICATION
		__init_array_end = .;
#else
		__zephyr_init_array_end = .;
#endif

#ifdef CONFIG_NATIVE_LIBC
		/*
                 * The __CTOR_LIST__ and __CTOR_END__ symbols are always defined
                 * to result in an empty list.
		 * Instead, Zephyr's start-up code uses the __zephyr_init_array_start__ and
		 * __zephyr_init_array_end__ symbols.
		 * In this way, in native_simulator based targets, the host glibc process
		 * initialization code will not call the constructors before Zephyr loads.
		 */
		__CTOR_LIST__ = .;
#ifdef CONFIG_64BIT
		QUAD(0)
		QUAD(0)
#else
		LONG(0)
		LONG(0)
#endif
		__CTOR_END__ = .;
#ifndef CONFIG_NATIVE_APPLICATION
		/*
		 * Similar to the schenanigans required for the __CTOR_LIST__ and
		 * __CTOR_END__ symbols we define __init_array_start and __init_array_end
		 * to the same address to define an empty list. This prevents the glibc
		 * startup code from calling any global constructors before Zephyr loads.
		 */
		__init_array_start = .;
		__init_array_end = .;
#endif
#endif
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if !defined(CONFIG_STATIC_INIT_GNU) && !defined(CONFIG_NATIVE_APPLICATION)
	ASSERT(__zephyr_init_array_start == __zephyr_init_array_end,
	       "GNU-style constructors required but STATIC_INIT_GNU not enabled")
#endif

#endif
