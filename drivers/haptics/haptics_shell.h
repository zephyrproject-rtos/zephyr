/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_HAPTICS_HAPTICS_SHELL_H_
#define ZEPHYR_DRIVERS_HAPTICS_HAPTICS_SHELL_H_

#include <zephyr/drivers/haptics.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Haptics registry information for the haptics shell
 *
 * Device drivers can optionally place metadata in the iterable section, which can then be used by
 * the haptics shell for auto-completion. See @ref HAPTICS_SHELL_REGISTER() for details.
 */
struct haptics_shell_registry {
	const struct device *dev;    /**< Pointer to the device structure for the haptic device */
	const uint32_t num_triggers; /**< Number of available triggers */
	const union shell_cmd_entry *set_trigger_subcmd; /**< Device-specific set trigger */
	const union shell_cmd_entry *trigger_subcmd;     /**< Device-specific trigger */
};

/**
 * @brief Device-specific dynamic command for @ref haptics_set_trigger()
 *
 * In combination with @ref haptics_shell_registry, this function enables dynamic auto-completion
 * in the haptics shell to show the number of available triggers.
 *
 * @param[in] dev Pointer to the device structure for the haptic device
 * @param[in] idx The device number starting from zero
 * @param[in] entry Pointer to array of static commands
 */
void haptics_shell_set_trigger_get(const struct device *dev, size_t idx,
				   struct shell_static_entry *entry);

/**
 * @brief Device-specific dynamic command for @ref haptics_trigger()
 *
 * In combination with @ref haptics_shell_registry, this function enables dynamic auto-completion
 * in the haptics shell to show the number of available triggers.
 *
 * @param[in] dev Pointer to the device structure for the haptic device
 * @param[in] idx The device number starting from zero
 * @param[in] entry Pointer to array of static commands
 */
void haptics_shell_trigger_get(const struct device *dev, size_t idx,
			       struct shell_static_entry *entry);

/**
 * @brief Extend @ref UTIL_CAT() to concatenate three strings
 *
 * @param[in] a String to concatenate
 * @param[in] b String to concatenate
 * @param[in] c String to concatenate
 */
#define HAPTICS_UTIL_CAT_3(a, b, c) UTIL_CAT(UTIL_CAT(UTIL_CAT(a, _), b), UTIL_CAT(_, c))

/**
 * @brief Helper macro for @ref HAPTICS_SHELL_REGISTER()
 *
 * Generates a unique dynamic command string with the `dsub_` prefix.
 *
 * @param[in] inst Devicetree instance number
 * @param[in] cmd Function name corresponding to a haptics shell command
 */
#define HAPTICS_SHELL_REGISTER_GET_SUBCMD(inst, cmd)                                               \
	HAPTICS_UTIL_CAT_3(UTIL_CAT(dsub_, cmd), DT_DRV_COMPAT, inst)

/**
 * @brief Helper macro for @ref HAPTICS_SHELL_REGISTER()
 *
 * Generates the dynamic command corresponding to @p cmd.
 *
 * @param[in] inst Devicetree instance number
 * @param[in] cmd Function name corresponding to a haptics shell command
 */
#define HAPTICS_SHELL_REGISTER_SUBCMD(inst, cmd)                                                   \
	static void HAPTICS_UTIL_CAT_3(cmd, DT_DRV_COMPAT, inst)(size_t idx,                       \
								 struct shell_static_entry *entry) \
	{                                                                                          \
		cmd(DEVICE_DT_INST_GET(inst), idx, entry);                                         \
	}                                                                                          \
	SHELL_DYNAMIC_CMD_CREATE(HAPTICS_SHELL_REGISTER_GET_SUBCMD(inst, cmd),                     \
				 HAPTICS_UTIL_CAT_3(cmd, DT_DRV_COMPAT, inst))

/**
 * @brief Haptics registry macro for device drivers
 *
 * If @kconfig{CONFIG_HAPTICS_SHELL} is enabled, any device driver that implements this macro will
 * register a copy of @ref haptics_registry in the iterable section with the following data:
 *	- @p _num_triggers stores the number of available triggers for @ref haptics_set_trigger()
 *	  and @ref haptics_trigger().
 *
 * @note If @kconfig{CONFIG_HAPTICS_SHELL} is disabled, then this macro becomes a no-op to prevent
 * unused data from being added to the iterable section.
 *
 * @param[in] inst Devicetree instance number
 * @param[in] _num_triggers Number of available triggers
 */
#define HAPTICS_SHELL_REGISTER(inst, _num_triggers)                                                \
	COND_CODE_1(IS_ENABLED(CONFIG_HAPTICS_SHELL),						   \
		    (HAPTICS_SHELL_REGISTER_SUBCMD(inst, haptics_shell_set_trigger_get);           \
		     HAPTICS_SHELL_REGISTER_SUBCMD(inst, haptics_shell_trigger_get);               \
		     static const STRUCT_SECTION_ITERABLE(haptics_shell_registry,		   \
				HAPTICS_UTIL_CAT_3(haptics_shell_registry, DT_DRV_COMPAT, inst)) = \
			{.dev = DEVICE_DT_INST_GET(inst),                                          \
			 .num_triggers = (_num_triggers),                                          \
			 .set_trigger_subcmd = &HAPTICS_SHELL_REGISTER_GET_SUBCMD(inst,            \
								haptics_shell_set_trigger_get),    \
			 .trigger_subcmd = &HAPTICS_SHELL_REGISTER_GET_SUBCMD(inst,                \
								haptics_shell_trigger_get)};),     \
		    (EMPTY))

/** @endcond */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_HAPTICS_HAPTICS_SHELL_H_ */
