/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Run the set of special native tasks corresponding to the given level
 *
 * @param level One of _NATIVE_*_LEVEL as defined in soc.h
 */
#ifdef __APPLE__

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/util.h>

extern const struct mach_header_64 _mh_execute_header;

struct macho_native_task_section {
	void (**start)(void);
	size_t count;
	unsigned int prio;
};

static unsigned int macho_parse_native_task_prio(const char *str)
{
	unsigned int value = 0U;

	while ((*str >= '0') && (*str <= '9')) {
		value = (value * 10U) + (unsigned int)(*str - '0');
		str++;
	}

	return value;
}

static size_t macho_collect_native_task_sections(const char *prefix,
						 struct macho_native_task_section *sections,
						 size_t max_sections)
{
	const struct mach_header_64 *hdr = &_mh_execute_header;
	const struct load_command *lc =
		(const struct load_command *)((const char *)hdr + sizeof(*hdr));
	const intptr_t slide = _dyld_get_image_vmaddr_slide(0);
	const size_t prefix_len = strlen(prefix);
	size_t count = 0U;

	for (uint32_t i = 0U; i < hdr->ncmds; i++) {
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *seg =
				(const struct segment_command_64 *)lc;
			const struct section_64 *sec =
				(const struct section_64 *)(seg + 1);

			if (strncmp(seg->segname, "__DATA", sizeof(seg->segname)) == 0) {
				for (uint32_t j = 0U; (j < seg->nsects) && (count < max_sections);
				     j++, sec++) {
					char sectname[sizeof(sec->sectname) + 1];

					memcpy(sectname, sec->sectname, sizeof(sec->sectname));
					sectname[sizeof(sec->sectname)] = '\0';

					if (strncmp(sectname, prefix, prefix_len) != 0) {
						continue;
					}

					sections[count].start =
						(void (**)(void))(uintptr_t)(sec->addr + slide);
					sections[count].count = sec->size / sizeof(void (*)(void));
					sections[count].prio =
						macho_parse_native_task_prio(sectname + prefix_len);
					count++;
				}
			}
		}

		lc = (const struct load_command *)((const char *)lc + lc->cmdsize);
	}

	return count;
}

static void macho_sort_native_task_sections(struct macho_native_task_section *sections,
					    size_t count)
{
	for (size_t i = 1U; i < count; i++) {
		struct macho_native_task_section key = sections[i];
		size_t j = i;

		while ((j > 0U) && (sections[j - 1U].prio > key.prio)) {
			sections[j] = sections[j - 1U];
			j--;
		}

		sections[j] = key;
	}
}

void run_native_tasks(int level)
{
	static const char *const prefixes[] = {
		"natt0_",
		"natt1_",
		"natt2_",
		"natt3_",
		"natt4_",
	};
	struct macho_native_task_section sections[16];
	size_t count;

	if ((level < 0) || (level >= (int)ARRAY_SIZE(prefixes))) {
		return;
	}

	count = macho_collect_native_task_sections(prefixes[level], sections,
						       ARRAY_SIZE(sections));
	macho_sort_native_task_sections(sections, count);

	for (size_t i = 0U; i < count; i++) {
		for (size_t j = 0U; j < sections[i].count; j++) {
			void (*fn)(void) = sections[i].start[j];

			if (fn != NULL) {
				fn();
			}
		}
	}
}

#else

void run_native_tasks(int level)
{
	extern void (*__native_PRE_BOOT_1_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_2_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_3_tasks_start[])(void);
	extern void (*__native_FIRST_SLEEP_tasks_start[])(void);
	extern void (*__native_ON_EXIT_tasks_start[])(void);
	extern void (*__native_tasks_end[])(void);

	static void (**native_pre_tasks[])(void) = {
		__native_PRE_BOOT_1_tasks_start,
		__native_PRE_BOOT_2_tasks_start,
		__native_PRE_BOOT_3_tasks_start,
		__native_FIRST_SLEEP_tasks_start,
		__native_ON_EXIT_tasks_start,
		__native_tasks_end
	};

	void (**fptr)(void);

	for (fptr = native_pre_tasks[level]; fptr < native_pre_tasks[level+1];
		fptr++) {
		if (*fptr) { /* LCOV_EXCL_BR_LINE */
			(*fptr)();
		}
	}
}

#endif
