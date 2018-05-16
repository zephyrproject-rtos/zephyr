/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_INSTANCE_H
#define LOG_INSTANCE_H

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Constant data related with source of log messages. */
struct log_module_rom_data {
	const char *p_name;
	u8_t level;
};

/** @brief Constant data related with source of log messages. */
struct log_module_ram_data {
	u32_t filters;
};

extern struct log_module_rom_data __log_rom_start;
extern void *__log_rom_end;

extern struct log_module_ram_data __log_ram_start;
extern void *__log_ram_end;

/***********************************************************/
/****************** ROM section macros *********************/
/***********************************************************/

/** @brief Creates name of variable and section for constant log data.
 *
 *  @param _name Name.
 */
#define LOG_ITEM_ROM_DATA(_name) UTIL_CAT(log_rom_, _name)

/** @brief Pointer to start of the section with constant log data. */
#define LOG_ROM_SECTION_START        &__log_rom_start

/** @brief Macro for calculating offset of the address relative to the start
 *         of the section.
 *
 *  @param _addr Address.
 */
#define LOG_ROM_SECTION_OFFSET(_addr) \
	((size_t)(_addr) - (size_t)&__log_rom_start)

/** @brief Macro for getting index of the element in the section.
 *
 *  @param Address of the element.
 */
#define LOG_ROM_ELEMENT_IDX(_addr) \
	(LOG_ROM_SECTION_OFFSET(_addr) / sizeof(struct log_module_rom_data))

/** @brief Macro for getting number of elements in the section. */
#define LOG_ROM_SECTION_ITEM_COUNT() \
	LOG_ROM_ELEMENT_IDX(&__log_rom_end)

/** @brief Macro for getting address of the element.
 *
 *  @param _idx Element index.
 */
#define LOG_ROM_SECTION_ITEM_GET(_idx) \
	(LOG_ROM_SECTION_START + (_idx))

#define LOG_SOURCE_NAME_GET(_idx) \
	LOG_ROM_SECTION_ITEM_GET(_idx)->p_name

#define LOG_SOURCE_STATIC_LEVEL_GET(_idx) \
	LOG_ROM_SECTION_ITEM_GET(_idx)->level

/** @brief Macro for getting ID of the element of the section.
 *
 *  @param _p_rom Address of the element.
 */
#if CONFIG_LOG
#define LOG_ROM_ID_GET(_p_rom) LOG_ROM_ELEMENT_IDX(_p_rom)
#else
#define LOG_ROM_ID_GET(_p_rom) 0
#endif

/** @brief Macro for getting ID of current module. */
#define LOG_CURRENT_MODULE_ID()	\
	LOG_ROM_ELEMENT_IDX(&LOG_ITEM_ROM_DATA(LOG_MODULE_NAME))
/***********************************************************/
/****************** RAM section macros *********************/
/***********************************************************/

/** @brief Creates name of variable and section for runtime log data.
 *
 *  @param _name Name.
 */
#define LOG_ITEM_RAM_DATA(_name) UTIL_CAT(log_ram_, _name)

/** @brief Pointer to start of the section with dynamic log data. */
#define LOG_RAM_SECTION_START   &__log_ram_start

/** @brief Macro for calculating offset of the address relative to the start
 *         of the section.
 *
 *  @param _addr Address.
 */
#define LOG_RAM_SECTION_OFFSET(_addr) \
	((size_t)(_addr) - (size_t)&__log_ram_start)

/** @brief Macro for getting index of the element in the section.
 *
 *  @param Address of the element.
 */
#define LOG_RAM_ELEMENT_IDX(_addr) \
	(LOG_RAM_SECTION_OFFSET(_addr) / sizeof(struct log_module_ram_data))

/** @brief Macro for getting number of elements in the section. */
#define LOG_RAM_SECTION_ITEM_COUNT \
	LOG_RAM_ELEMENT_IDX(&__log_ram_end)

/** @brief Macro for getting address of the element.
 *
 *  @param _idx Element index.
 */
#define LOG_RAM_SECTION_ITEM_GET(_idx) \
	(LOG_RAM_SECTION_START + (_idx))


/** @brief Macro for getting ID of the element of the section.
 *
 *  @param _p_rom Address of the element.
 */
#if CONFIG_LOG
#define LOG_RAM_ID_GET(_p_ram) LOG_RAM_ELEMENT_IDX(_p_ram)
#else
#define LOG_RAM_ID_GET(_p_ram) 0
#endif

/******************************************************************************/
/****************** Filtering macros ******************************************/
/******************************************************************************/

/** @brief Filter slot size. */
#define LOG_FILTER_SLOT_SIZE LOG_LEVEL_BITS

/** @brief Number of slots in one word. */
#define LOG_FILTERS_NUM_OF_SLOTS (32 / LOG_FILTER_SLOT_SIZE)

/** @brief Slot mask. */
#define LOG_FILTER_SLOT_MASK ((1 << LOG_FILTER_SLOT_SIZE) - 1)

/** @brief Bit offset of a slot.
 *
 *  @param _id Slot ID.
 */
#define LOG_FILTER_SLOT_SHIFT(_id) (LOG_FILTER_SLOT_SIZE * (_id))

#define LOG_FILTER_SLOT_GET(_p_filters, _id) \
	((*(_p_filters) >> LOG_FILTER_SLOT_SHIFT(_id)) & LOG_FILTER_SLOT_MASK)

#define LOG_FILTER_SLOT_SET(_p_filters, _id, _filter)					  \
	do {										  \
		*(_p_filters) &=							  \
			~(LOG_FILTER_SLOT_MASK << LOG_FILTER_SLOT_SHIFT(_id));		  \
		*(_p_filters) |=							  \
			((_filter) & LOG_FILTER_SLOT_MASK) << LOG_FILTER_SLOT_SHIFT(_id); \
	} while (0)

#define LOG_FILTER_AGGR_SLOT_IDX 0

#define LOG_FILTER_AGGR_SLOT_GET(_p_filters) \
	LOG_FILTER_SLOT_GET(_p_filters, LOG_FILTER_AGGR_SLOT_IDX)

#define LOG_FILTER_FIRST_BACKEND_SLOT_IDX 1

#if CONFIG_LOG_RUNTIME_FILTERING
#define LOG_RUNTIME_FILTER(_p_filter) \
	LOG_FILTER_SLOT_GET(&(_p_filter)->filters, LOG_FILTER_AGGR_SLOT_IDX)
#else
#define LOG_RUNTIME_FILTER(p_filter) LOG_LEVEL_DBG
#endif

#define LOG_INTERNAL_ROM_ITEM_REGISTER(_name, _str_name, _level)	   \
	const struct log_module_rom_data LOG_ITEM_ROM_DATA(_name)	   \
	__attribute__ ((section("." STRINGIFY(LOG_ITEM_ROM_DATA(_name))))) \
	__attribute__((used)) = {					   \
		.p_name = _str_name,					   \
		.level  = (_level),					   \
	}

#if CONFIG_LOG_RUNTIME_FILTERING
#define LOG_INTERNAL_RAM_ITEM_REGISTER(_name)				   \
	struct log_module_ram_data LOG_ITEM_RAM_DATA(_name)		   \
	__attribute__ ((section("." STRINGIFY(LOG_ITEM_RAM_DATA(_name))))) \
	__attribute__((used))
#else
#define LOG_INTERNAL_RAM_ITEM_REGISTER(_name) /* empty */
#endif

/** @def LOG_INSTANCE_PTR_DECLARE
 * @brief Macro for declaring a logger instance pointer in the module structure.
 */

/** @def LOG_INSTANCE_REGISTER
 * @brief Macro for registering instance for logging with independent filtering.
 *
 * Module instance provides filtering of logs on instance level instead of module level.
 */

/** @def LOG_INSTANCE_PTR_INIT
 * @brief Macro for initializing a pointer to the logger instance.
 */


/** @} */

#ifdef CONFIG_LOG

#define LOG_INSTANCE_FULL_NAME(_module_name, _inst_name) \
	UTIL_CAT(_module_name, UTIL_CAT(_, _inst_name))

#if CONFIG_LOG_RUNTIME_FILTERING
#define LOG_INSTANCE_PTR_DECLARE(_p_name) \
	struct log_module_ram_data *_p_name

#define LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level)	  \
	LOG_INTERNAL_ROM_ITEM_REGISTER(				  \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name), \
		STRINGIFY(_module_name._inst_name),		  \
		_level);					  \
								  \
	LOG_INTERNAL_RAM_ITEM_REGISTER(				  \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name))


#define LOG_INSTANCE_PTR_INIT(_p_name, _module_name, _inst_name) \
	._p_name = &LOG_ITEM_RAM_DATA(				 \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name)),

#else /* CONFIG_LOG_RUNTIME_FILTERING */
#define LOG_INSTANCE_PTR_DECLARE(_p_name) \
	const struct log_module_rom_data *_p_name

#define LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level)	  \
	LOG_INTERNAL_ROM_ITEM_REGISTER(				  \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name), \
		STRINGIFY(_module_name._inst_name),		  \
		_level)


#define LOG_INSTANCE_PTR_INIT(_p_name, _module_name, _inst_name) \
	._p_name = &LOG_ITEM_ROM_DATA(				 \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name)),

#endif /* CONFIG_LOG_RUNTIME_FILTERING */
#else /* CONFIG_LOG */
#define LOG_INSTANCE_PTR_DECLARE(_p_name) /* empty */
#define LOG_INSTANCE_REGISTER(_module_name, _inst_name,  _level)        /* empty */
#define LOG_INSTANCE_PTR_INIT(_p_name, _module_name, _inst_name)        /* empty */
#endif

#ifdef __cplusplus
}
#endif

#endif /* LOG_INSTANCE_H */
