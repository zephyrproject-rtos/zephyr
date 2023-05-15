/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

	ITERABLE_SECTION_ROM(log_strings, 4)

	ITERABLE_SECTION_ROM(log_const, 4)

	ITERABLE_SECTION_ROM(log_backend, 4)

	ITERABLE_SECTION_ROM(log_link, 4)
