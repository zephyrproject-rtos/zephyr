/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

SECTION_DATA_PROLOGUE(virtual_device_instances_sections,,)
{
	__virtual_device_instances_start = .;
	KEEP(*("._virtual_device_instance.*"));
	__virtual_device_instances_end = .;
} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
