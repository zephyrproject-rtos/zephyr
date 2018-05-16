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

/** @brief Constant data associated with the source of log messages. */
struct log_source_const_data {
	const char *name;
	u8_t level;
};

/** @brief Dynamic data associated with the source of log messages. */
struct log_source_dynamic_data {
	u32_t filters;
};

extern struct log_source_const_data __log_const_start[0];
extern struct log_source_const_data __log_const_end[0];


/** @brief Creates name of variable and section for constant log data.
 *
 *  @param _name Name.
 */
#define LOG_ITEM_CONST_DATA(_name) UTIL_CAT(log_const_, _name)

/** @brief Get name of the log source.
 *
 * @param source_id Source ID.
 * @return Name.
 */
static inline const char *log_name_get(u32_t source_id)
{
	return __log_const_start[source_id].name;
}

/** @brief Get compiled level of the log source.
 *
 * @param source_id Source ID.
 * @return Level.
 */
static inline u8_t log_compiled_level_get(u32_t source_id)
{
	return __log_const_start[source_id].level;
}

/** @brief Get index of the log source based on the address of the constant data
 *         associated with the source.
 *
 * @param data Address of the constant data.
 *
 * @return Source ID.
 */
static inline u32_t log_const_source_id(
				const struct log_source_const_data *data)
{
	return ((void *)data - (void *)__log_const_start)/
			sizeof(struct log_source_const_data);
}

/** @brief Get number of registered sources. */
static inline u32_t log_sources_count(void)
{
	return log_const_source_id(__log_const_end);
}

extern struct log_source_dynamic_data __log_dynamic_start[0];
extern struct log_source_dynamic_data __log_dynamic_end[0];

/** @brief Creates name of variable and section for runtime log data.
 *
 *  @param _name Name.
 */
#define LOG_ITEM_DYNAMIC_DATA(_name) UTIL_CAT(log_dynamic_, _name)

#define LOG_INSTANCE_DYNAMIC_DATA(_module_name, _inst) \
	LOG_ITEM_DYNAMIC_DATA(LOG_INSTANCE_FULL_NAME(_module_name, _inst))

/** @brief Get pointer to the filter set of the log source.
 *
 * @param source_id Source ID.
 *
 * @return Pointer to the filter set.
 */
static inline u32_t *log_dynamic_filters_get(u32_t source_id)
{
	return &__log_dynamic_start[source_id].filters;
}

/** @brief Get index of the log source based on the address of the dynamic data
 *         associated with the source.
 *
 * @param data Address of the dynamic data.
 *
 * @return Source ID.
 */
static inline u32_t log_dynamic_source_id(struct log_source_dynamic_data *data)
{
	return ((void *)data - (void *)__log_dynamic_start)/
			sizeof(struct log_source_dynamic_data);
}

/**
 *  @def LOG_CONST_ID_GET
 *  @brief Macro for getting ID of the element of the section.
 *
 *  @param _addr Address of the element.
 */
/**
 * @def LOG_CURRENT_MODULE_ID
 * @brief Macro for getting ID of current module.
 */
#ifdef CONFIG_LOG
#define LOG_CONST_ID_GET(_addr) \
	log_const_source_id((const struct log_source_const_data *)_addr)
#define LOG_CURRENT_MODULE_ID()	\
	log_const_source_id(&LOG_ITEM_CONST_DATA(LOG_MODULE_NAME))
#else
#define LOG_CONST_ID_GET(_addr) 0
#define LOG_CURRENT_MODULE_ID() 0
#endif

/** @brief Macro for getting ID of the element of the section.
 *
 *  @param _addr Address of the element.
 */
#if CONFIG_LOG
#define LOG_DYNAMIC_ID_GET(_addr) \
	log_dynamic_source_id((struct log_source_dynamic_data *)_addr)
#else
#define LOG_DYNAMIC_ID_GET(_addr) 0
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

#define LOG_FILTER_SLOT_GET(_filters, _id) \
	((*(_filters) >> LOG_FILTER_SLOT_SHIFT(_id)) & LOG_FILTER_SLOT_MASK)

#define LOG_FILTER_SLOT_SET(_filters, _id, _filter)		     \
	do {							     \
		*(_filters) &= ~(LOG_FILTER_SLOT_MASK <<	     \
				 LOG_FILTER_SLOT_SHIFT(_id));	     \
		*(_filters) |= ((_filter) & LOG_FILTER_SLOT_MASK) << \
			       LOG_FILTER_SLOT_SHIFT(_id);	     \
	} while (0)

#define LOG_FILTER_AGGR_SLOT_IDX 0

#define LOG_FILTER_AGGR_SLOT_GET(_filters) \
	LOG_FILTER_SLOT_GET(_filters, LOG_FILTER_AGGR_SLOT_IDX)

#define LOG_FILTER_FIRST_BACKEND_SLOT_IDX 1

#if CONFIG_LOG_RUNTIME_FILTERING
#define LOG_RUNTIME_FILTER(_filter) \
	LOG_FILTER_SLOT_GET(&(_filter)->filters, LOG_FILTER_AGGR_SLOT_IDX)
#else
#define LOG_RUNTIME_FILTER(_filter) LOG_LEVEL_DBG
#endif

#define _LOG_CONST_ITEM_REGISTER(_name, _str_name, _level)		     \
	const struct log_source_const_data LOG_ITEM_CONST_DATA(_name)	     \
	__attribute__ ((section("." STRINGIFY(LOG_ITEM_CONST_DATA(_name))))) \
	__attribute__((used)) = {					     \
		.name = _str_name,					     \
		.level  = (_level),					     \
	}

#if CONFIG_LOG_RUNTIME_FILTERING
#define _LOG_DYNAMIC_ITEM_REGISTER(_name)				       \
	struct log_source_dynamic_data LOG_ITEM_DYNAMIC_DATA(_name)	       \
	__attribute__ ((section("." STRINGIFY(LOG_ITEM_DYNAMIC_DATA(_name))))) \
	__attribute__((used))
#else
#define _LOG_DYNAMIC_ITEM_REGISTER(_name) /* empty */
#endif

/** @def LOG_INSTANCE_PTR_DECLARE
 * @brief Macro for declaring a logger instance pointer in the module structure.
 */

/** @def LOG_INSTANCE_REGISTER
 * @brief Macro for registering instance for logging with independent filtering.
 *
 * Module instance provides filtering of logs on instance level instead of
 * module level.
 */

/** @def LOG_INSTANCE_PTR_INIT
 * @brief Macro for initializing a pointer to the logger instance.
 */

/** @} */

#ifdef CONFIG_LOG

#define LOG_INSTANCE_FULL_NAME(_module_name, _inst_name) \
	UTIL_CAT(_module_name, UTIL_CAT(_, _inst_name))

#if CONFIG_LOG_RUNTIME_FILTERING
#define LOG_INSTANCE_PTR_DECLARE(_name)	\
	struct log_source_dynamic_data *_name

#define LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level)		   \
	_LOG_CONST_ITEM_REGISTER(					   \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name),	   \
		STRINGIFY(_module_name._inst_name),			   \
		_level);						   \
	struct log_source_dynamic_data LOG_INSTANCE_DYNAMIC_DATA(	   \
						_module_name, _inst_name)  \
		__attribute__ ((section("." STRINGIFY(			   \
				LOG_INSTANCE_DYNAMIC_DATA(_module_name,	   \
						       _inst_name)	   \
				)					   \
		))) __attribute__((used))

#define LOG_INSTANCE_PTR_INIT(_name, _module_name, _inst_name)	   \
	._name = &LOG_ITEM_DYNAMIC_DATA(			   \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name)),

#else /* CONFIG_LOG_RUNTIME_FILTERING */
#define LOG_INSTANCE_PTR_DECLARE(_name)	\
	const struct log_source_const_data *_name

#define LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level)	  \
	_LOG_CONST_ITEM_REGISTER(				  \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name), \
		STRINGIFY(_module_name._inst_name),		  \
		_level)


#define LOG_INSTANCE_PTR_INIT(_name, _module_name, _inst_name) \
	._name = &LOG_ITEM_CONST_DATA(			       \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name)),

#endif /* CONFIG_LOG_RUNTIME_FILTERING */
#else /* CONFIG_LOG */
#define LOG_INSTANCE_PTR_DECLARE(_name) /* empty */
#define LOG_INSTANCE_REGISTER(_module_name, _inst_name,  _level)    /* empty */
#define LOG_INSTANCE_PTR_INIT(_name, _module_name, _inst_name)      /* empty */
#endif

#ifdef __cplusplus
}
#endif

#endif /* LOG_INSTANCE_H */
