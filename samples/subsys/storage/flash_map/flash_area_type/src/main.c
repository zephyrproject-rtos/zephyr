/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Sample explains usage functions:
 *  flash_area_active
 *  flash_area_enum_type
 * and macros:
 *  FLASH_AREA_FOR_EACH_COMPATIBLE
 *
 * for obtaining information on active partitions and generating code for user selected type of
 * partitions.
 */

#include <zephyr.h>
#include <device.h>
#include <storage/flash_map.h>

#define MAKE_LIST_ITEM(type, compatible) { type, #type, compatible }

const static struct type_meta {
	int type;
	const char *stype;
	const char *compatible;
} types[] = {
	MAKE_LIST_ITEM(FLASH_AREA_TYPE_UNSPECIFIED, "None"),
	MAKE_LIST_ITEM(FLASH_AREA_TYPE_MCUBOOT, "zephyr,mcuboot,image"),
	MAKE_LIST_ITEM(FLASH_AREA_TYPE_MCUBOOT_SCRATCH, "zephyr,mcyboot, scratch"),
	MAKE_LIST_ITEM(FLASH_AREA_TYPE_ZEPHYR_APPLICATION, "zephyr,application,image"),
	MAKE_LIST_ITEM(FLASH_AREA_TYPE_UNKNOWN, "Unknown")
};

/*
 * This function will only present list of known types with C definitions for them and "compatible"
 * string that they correspond to; function has been provided for convenience.
 */
static void list_known_types(void)
{
	size_t i = 0;

	printk("Known types:\n");
	while (i < ARRAY_SIZE(types)) {
		printk("\t%s = 0x%x, compatible = %s\n", types[i].stype, types[i].type,
		       types[i].compatible);
		++i;
	}
}


/*
 * The function will enumerate, at run-time, partitions of all known types. Some list may be
 * generated empty, when the DTS or overlay does not specify the partition or it has been
 * disabled within DTS.
 */
static void enum_all_of_type(const struct type_meta *t)
{
	const struct flash_area *other = NULL;

	printk("\nList of all %s areas\n", t->stype);
	printk(" Offset | ID\n");
	printk("========+====\n");
	while (1) {
		other = flash_area_enum_type(other, t->type);
		if (other == NULL) {
			break;
		}

		printk("%8lx|%4d\n", (unsigned long)other->fa_off, other->fa_id);
	}
	printk("--------+----\n");
}


/*
 * Macros given as function parameter to FLASH_AREA_FOR_EACH_COMPATIBLE take two parameters; first
 * parameter is the instance index of compatible the second is the compatible string, same one
 * that is passed as a first parameter to FLASH_AREA_FOR_EACH_COMPATIBLE.
 *
 * The exemplary macro below will expand each par of idx (index) and compat (compatible) to
 * printk printing the partition ID and string representing the DT node that represents the
 * partition.
 */
#define PRINT_ID_AND_NODE(idx, compat)				\
	printk(" ID %d -> %s\n",				\
	       DT_FIXED_PARTITION_ID(DT_INST(idx, compat)),	\
	       STRINGIFY(DT_INST(idx, compat))			\
	);							\

/*
 * Function, together with PRINT_ID_AND_NODE macro, provides example on how compile time expansion
 * works; the FLASH_AREA_FOR_EACH_COMPATIBLE can be also used to generate structure at compile time.
 */
static void compile_time_expanded(void)
{
	printk("\nExamples of compile time expansions by 'compatible'\n");
	printk("===================================================\n");
	printk("List of zephyr,mcuboot,image:\n");
	FLASH_AREA_FOR_EACH_COMPATIBLE(zephyr_mcuboot_image, PRINT_ID_AND_NODE);
	printk("List of zephyr,mcuboot,scratch:\n");
	FLASH_AREA_FOR_EACH_COMPATIBLE(zephyr_mcuboot_scratch, PRINT_ID_AND_NODE);
	printk("List of zephyr,application,image:\n");
	FLASH_AREA_FOR_EACH_COMPATIBLE(zephyr_application_image, PRINT_ID_AND_NODE);
	printk("List of fake, not exisitng partition (empty):\n");
	FLASH_AREA_FOR_EACH_COMPATIBLE(not_existing_junk, PRINT_ID_AND_NODE);
}

void main(void)
{
	/* raw disk i/o */
	const struct flash_area *active = flash_area_active();

	/* Just list know types for reference  */
	list_known_types();

	if (active == NULL) {
		printk("\nActive flash area not found in flash map\n");
	} else {
		printk("\nActive flash area is %p, ID is %d, type is 0x%x\n", active, active->fa_id,
		       active->fa_type);
	}

	/* Go through the list of types and print lists of flash areas that have certain type */
	for (int i = 0; i < ARRAY_SIZE(types); ++i) {
		enum_all_of_type(&types[i]);
	}

	compile_time_expanded();
}
