/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_PRINTK_CUSTOM_FORMAT_H_
#define ZEPHYR_INCLUDE_SYS_PRINTK_CUSTOM_FORMAT_H_

#include <sys/memobj.h>
#include <sys/printk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@defgroup PRINTK_CUSTOM_FORMAT_FLAGS Flags
 * @{ */

/** @brief Flag indicating if memory object should be freed after string is
 *	   formatted.
 */
#define PRINTK_CUST_FORMAT_FREE BIT(1)

/** @brief Flag indicating if pointer to data is stored in the object. */
#define PRINTK_CUST_FORMAT_PTR BIT(2)

/**@} */

/** @brief Prototype of custom formatting function.
 *
 * @param memobj	Memory object with data to format.
 * @param flags		Flags. See @ref PRINTK_CUSTOM_FORMAT_FLAGS.
 * @param out_func	Output function.
 * @param ctx		Context passed to the output function.
 */
typedef void (*printk_custom_formatter_t)(memobj_t *memobj, u8_t flags,
					  printk_out_func_t out_func,
					  void *ctx);

#define Z_PRINTK_CUST_FORMAT_OVERHEAD \
	(sizeof(printk_custom_formatter_t) + sizeof(u8_t))

#define Z_PRINTK_CUST_FORMAT_SIZE(len) \
	(Z_PRINTK_CUST_FORMAT_OVERHEAD + MEMOBJ1_TOTAL_SIZE(len))

struct z_printk_cust_format_ptr_obj {
	char buf[Z_PRINTK_CUST_FORMAT_SIZE(sizeof(void *))];
};

/** @brief Macro for custom formatting with object allocated on the stack.
 *
 * @param ptr	Data for custom formatting.
 * @param func	User function for creating memory object with the data.
 */
#define PRINTK_CUST_FORMAT_DUP(ptr, func) \
	func((u8_t *)&(struct z_printk_cust_format_ptr_obj){}, (void *)ptr)

/** @brief Prepare memory object which holds data to be formatted and function
 *	   that will be used for the formatting.
 *
 * @param buf	Buffer. If NULL buffer will be allocated from the logger buffer
 *		pool.
 * @param data	Data to be formatted.
 * @param size	Data size.
 * @param func	Formatting function.
 *
 * @return Pointer to the memory object.
 */
void *printk_cust_format_dup(u8_t *buf, void *data, u32_t size,
			     printk_custom_formatter_t func);

/** @brief Get data from the object.
 *
 * @param[in]  memobj	Memory object.
 * @param[out] data	Buffer for data.
 * @param[in]  len	Amount of data to read.
 * @param[in]  offset	Read offset.
 */
u32_t printk_custom_format_data_get(memobj_t *memobj, u8_t *data,
				  u32_t len, u32_t offset);

/** @brief Format provided object.
 *
 * Function is processing provided object by reading the data and formatting
 * function and providing data to the function.
 *
 * @param memobj	Memory object.
 * @param out		Output function.
 * @param ctx		Context passed to the output function.
 */
void printk_custom_format_handle(memobj_t *memobj, printk_out_func_t out,
				 void *ctx);

/** @brief Duplicate stirng to a memory object.
 *
 * @param buf	Buffer for memory object, if NULL buffer is allocated from the
 *		logger buffer pool.
 * @param str	String.
 */
void *z_printk_custom_format_string_dup(u8_t *buf, char *str);

/** @brief Macro for duplicating string in synchronous context.
 *
 * Macro is allocating object on the stack and duplicates string. Usage example:
 * printk("%pZ", string_dup(str))
 */
#define string_dup(str) \
	PRINTK_CUST_FORMAT_DUP(str, z_printk_custom_format_string_dup)


/** @brief Macro to be used in log messages to log string using custom printk
 *	   formatting.
 *
 * Example: LOG_INF("%pZ", log_string_dup(addr));
 */
#define log_string_dup(str) \
	(IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? \
		string_dup(str) : \
		z_printk_custom_format_string_dup(NULL, (u8_t *)str))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_PRINTK_CUSTOM_FORMAT_H_ */
