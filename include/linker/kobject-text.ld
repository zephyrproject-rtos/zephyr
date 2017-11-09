#ifndef KOBJECT_TEXT_AREA
#if ! defined(CONFIG_SIZE_OPTIMIZATIONS) || defined(CONFIG_STACK_CANARIES)
#define KOBJECT_TEXT_AREA	256
#else
#define KOBJECT_TEXT_AREA	128
#endif
#endif

#ifdef CONFIG_USERSPACE
	/* We need to reserve room for the gperf generated hash functions.
	 * Fortunately, unlike the data tables, the size of the code is
	 * reasonably predictable.
	 *
	 * The linker will error out complaining that the location pointer
	 * is moving backwards if the reserved room isn't large enough.
	 */
	_kobject_text_area_start = .;
	*(".kobject_data.text*")
	_kobject_text_area_end = .;
#ifndef LINKER_PASS2
#ifdef CONFIG_DYNAMIC_OBJECTS
	PROVIDE(_k_object_gperf_find = .);
	PROVIDE(_k_object_gperf_wordlist_foreach = .);
#else
	PROVIDE(_k_object_find = .);
	PROVIDE(_k_object_wordlist_foreach = .);
#endif
#endif
	. += KOBJECT_TEXT_AREA - (_kobject_text_area_end - _kobject_text_area_start);
#endif /* CONFIG_USERSPACE */

