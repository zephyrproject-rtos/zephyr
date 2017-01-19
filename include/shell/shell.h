/* shell.h - Shell header */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif


struct device;

/** @brief Callback called when command is entered.
 *
 *  @param argc Number of parameters passed.
 *  @param argv Array of option strings. First option is always command name.
 *
 * @return 0 in case of success or negative value in case of error.
 */
typedef int (*shell_cmd_function_t)(int argc, char *argv[]);

struct shell_cmd {
	const char *cmd_name;
	shell_cmd_function_t cb;
	const char *help;
};


struct shell_module {
	const char *module_name;
	const struct shell_cmd *commands;
};


/**
 * @brief Kernel Shell API
 * @defgroup _shell_api_functions Shell API Functions
 * @{
 */

/**
 * @def SHELL_REGISTER
 *
 * @brief Create shell_module object and set it up for boot time initialization.
 *
 * @details This macro defines a shell_module object that is automatically
 * configured by the kernel during system initialization.
 *
 * @param shell_name Module name to be entered in shell console.
 *
 * @param shell_commands Array of commands to register.
 * Shell array entries must be packed to calculate array size correctly.
 */
#ifdef CONFIG_CONSOLE_SHELL
#define SHELL_REGISTER(shell_name, shell_commands) \
	\
	static struct shell_module (__shell_shell_name) __used \
	__attribute__((__section__(".shell_"))) = { \
		  .module_name = shell_name, \
		  .commands = shell_commands \
	}

#else
#define SHELL_REGISTER(shell_name, shell_commands)
#endif

/** @brief Initialize shell with optional prompt, NULL in case no prompt is
 *         needed.
 *
 *  @param prompt Prompt to be printed on serial console.
 */
void shell_init(const char *prompt);

/** @brief Optionally register an app default cmd handler.
 *
 *  @param handler To be called if no cmd found in cmds registered with
 *  shell_init.
 */
void shell_register_app_cmd_handler(shell_cmd_function_t handler);

/** @brief Callback to get the current prompt.
 *
 *  @returns Current prompt string.
 */
typedef const char *(*shell_prompt_function_t)(void);

/** @brief Optionally register a custom prompt callback.
 *
 *  @param handler To be called to get the current prompt.
 */
void shell_register_prompt_handler(shell_prompt_function_t handler);

/** @brief Optionally register a default module, to eliminate typing it in
 *  shell console or for backwards compatibility.
 *
 *  @param name Module name.
 */
void shell_register_default_module(const char *name);

/**
* @}
*/


#ifdef CONFIG_CONSOLE_SHELL
int shell_run(struct device *dev);
#else
static inline int shell_run(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}
#endif


#ifdef __cplusplus
}
#endif

