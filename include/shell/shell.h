/* shell.h - Shell header */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHELL_SHELL_H_
#define ZEPHYR_INCLUDE_SHELL_SHELL_H_

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
	const char *desc;
};

/** @brief Callback to get the current prompt.
 *
 *  @returns Current prompt string.
 */
typedef const char *(*shell_prompt_function_t)(void);

/** @brief Callback for custom line2argv parsing
 *
 * If this callback is set, command parsing for specified module works only
 * if it is selected as default module. This is to not break other modules.
 *
 *  @returns Current prompt string.
 */
typedef size_t (*shell_line2argv_function_t)(char *str, char *argv[],
								size_t size);

struct shell_module {
	const char *module_name;
	const struct shell_cmd *commands;
	shell_prompt_function_t prompt;
	shell_line2argv_function_t line2argv;
};

/** @typedef shell_mcumgr_function_t
 * @brief Callback that is executed when an mcumgr packet is received over the
 *        shell.
 *
 * The packet argument must be exactly what was received over the console,
 * except the terminating newline must be replaced with '\0'.
 *
 * @param line The received mcumgr packet.
 * @param arg An optional argument.
 *
 * @return  on success; negative error code on failure.
 */
typedef int (*shell_mcumgr_function_t)(const char *line, void *arg);

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
 * @param _name Module name to be entered in shell console.
 * @param _commands Array of commands to register.
 * Shell array entries must be packed to calculate array size correctly.
 */

/**
 * @def SHELL_REGISTER_WITH_PROMPT
 *
 * @brief Create shell_module object and set it up for boot time initialization.
 *
 * @details This macro defines a shell_module object that is automatically
 * configured by the kernel during system initialization, in addition to that
 * this also enables setting a custom prompt handler when the module is
 * selected.
 *
 * @param _name Module name to be entered in shell console.
 * @param _commands Array of commands to register.
 * Shell array entries must be packed to calculate array size correctly.
 * @param _prompt Optional prompt handler to be set when module is selected.
 */


/**
 * @def SHELL_REGISTER_COMMAND
 *
 * @brief Create a standalone command and set it up for boot time
 * initialization.
 *
 * @details This macro defines a shell_cmd object that is automatically
 * configured by the kernel during system initialization.
 *
 * The command will be available in the default module, so it will be available
 * immediately.
 *
 */
#ifdef CONFIG_CONSOLE_SHELL
#define SHELL_REGISTER(_name, _commands) \
	SHELL_REGISTER_WITH_PROMPT_AND_LINE2ARGV(_name, _commands, NULL, NULL)

#define SHELL_REGISTER_WITH_PROMPT(_name, _commands, _prompt) \
	SHELL_REGISTER_WITH_PROMPT_AND_LINE2ARGV(_name, _commands, _prompt, NULL)

#define SHELL_REGISTER_WITH_LINE2ARGV(_name, _commands, _line2argv) \
	SHELL_REGISTER_WITH_PROMPT_AND_LINE2ARGV(_name, _commands, NULL, \
						 _line2argv)

#define SHELL_REGISTER_WITH_PROMPT_AND_LINE2ARGV(_name, _commands, _prompt, \
						 _line2argv) \
	\
	static struct shell_module (__shell__name) __used \
	__attribute__((__section__(".shell_module_"))) = { \
		  .module_name = _name, \
		  .commands = _commands, \
		  .prompt = _prompt, \
		  .line2argv = _line2argv, \
	}

#define SHELL_REGISTER_COMMAND(name, callback, _help) \
	\
	const struct shell_cmd (__shell__##name) __used \
	__attribute__((__section__(".shell_cmd_"))) = { \
		  .cmd_name = name, \
		  .cb = callback, \
		  .help = _help \
	}
#else
#define SHELL_REGISTER(_name, _commands)
#define SHELL_REGISTER_WITH_PROMPT(_name, _commands, _prompt)
#define SHELL_REGISTER_COMMAND(_name, _callback, _help)
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

/** @brief Configures a callback for received mcumgr packets.
 *
 *  @param handler The callback to execute when an mcumgr packet is received.
 *  @param arg An optional argument to pass to the callback.
 */
void shell_register_mcumgr_handler(shell_mcumgr_function_t handler, void *arg);

/** @brief Execute command line.
 *
 *  Pass command line to shell to execute. The line cannot be a C string literal
 *  since it will be modified in place, instead a variable can be used:
 *
 *    char cmd[] = "command";
 *    shell_exec(cmd);
 *
 *  Note: This by no means makes any of the commands a stable interface, so
 *  this function should only be used for debugging/diagnostic.
 *
 *  @param line Command line to be executed
 *  @returns Result of the execution
 */
int shell_exec(char *line);

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

#endif /* ZEPHYR_INCLUDE_SHELL_SHELL_H_ */
