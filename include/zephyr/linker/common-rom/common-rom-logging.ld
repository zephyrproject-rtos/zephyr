/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_LOG_FMT_SECTION_STRIP) && defined(DEVNULL_REGION)
	SECTION_PROLOGUE(log_strings,(COPY),SUBALIGN(4))
	{
		Z_LINK_ITERABLE(log_strings);
	} GROUP_ROM_LINK_IN(DEVNULL_REGION, DEVNULL_REGION)
#else
	ITERABLE_SECTION_ROM(log_strings, 4)
#endif

	ITERABLE_SECTION_ROM(log_const, 4)

	ITERABLE_SECTION_ROM(log_backend, 4)

	ITERABLE_SECTION_ROM(log_link, 4)
