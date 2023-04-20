/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_H__
#define SHELL_H__

#include <zephyr/kernel.h>
#include <zephyr/shell/shell_types.h>
#include <zephyr/shell/shell_history.h>
#include <zephyr/shell/shell_fprintf.h>
#include <zephyr/shell/shell_log_backend.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/logging/log_instance.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#if defined CONFIG_SHELL_GETOPT
#include <getopt.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_SHELL_CMD_BUFF_SIZE
#define CONFIG_SHELL_CMD_BUFF_SIZE 0
#endif

#ifndef CONFIG_SHELL_PRINTF_BUFF_SIZE
#define CONFIG_SHELL_PRINTF_BUFF_SIZE 0
#endif

#ifndef CONFIG_SHELL_HISTORY_BUFFER
#define CONFIG_SHELL_HISTORY_BUFFER 0
#endif

#define Z_SHELL_CMD_ROOT_LVL		(0u)

#define SHELL_HEXDUMP_BYTES_IN_LINE	16

/**
 * @brief Flag indicates that optional arguments will be treated as one,
 *	  unformatted argument.
 *
 * By default, shell is parsing all arguments, treats all spaces as argument
 * separators unless they are within quotation marks which are removed in that
 * case. If command rely on unformatted argument then this flag shall be used
 * in place of number of optional arguments in command definition to indicate
 * that only mandatory arguments shall be parsed and remaining command string is
 * passed as a raw string.
 */
#define SHELL_OPT_ARG_RAW	(0xFE)

/**
 * @brief Flag indicating that number of optional arguments is not limited.
 */
#define SHELL_OPT_ARG_CHECK_SKIP (0xFF)

/**
 * @brief Flag indicating maximum number of optional arguments that can be
 *	  validated.
 */
#define SHELL_OPT_ARG_MAX		(0xFD)

/**
 * @brief Shell API
 * @defgroup shell_api Shell API
 * @ingroup shell
 * @{
 */

struct shell_static_entry;

/**
 * @brief Shell dynamic command descriptor.
 *
 * @details Function shall fill the received shell_static_entry structure
 * with requested (idx) dynamic subcommand data. If there is more than
 * one dynamic subcommand available, the function shall ensure that the
 * returned commands: entry->syntax are sorted in alphabetical order.
 * If idx exceeds the available dynamic subcommands, the function must
 * write to entry->syntax NULL value. This will indicate to the shell
 * module that there are no more dynamic commands to read.
 */
typedef void (*shell_dynamic_get)(size_t idx,
				  struct shell_static_entry *entry);

/**
 * @brief Shell command descriptor.
 */
union shell_cmd_entry {
	/*!< Pointer to function returning dynamic commands.*/
	shell_dynamic_get dynamic_get;

	/*!< Pointer to array of static commands. */
	const struct shell_static_entry *entry;
};

struct shell;

struct shell_static_args {
	uint8_t mandatory; /*!< Number of mandatory arguments. */
	uint8_t optional;  /*!< Number of optional arguments. */
};

/**
 * @brief Get by index a device that matches .
 *
 * This can be used, for example, to identify I2C_1 as the second I2C
 * device.
 *
 * Devices that failed to initialize or do not have a non-empty name
 * are excluded from the candidates for a match.
 *
 * @param idx the device number starting from zero.
 *
 * @param prefix optional name prefix used to restrict candidate
 * devices.  Indexing is done relative to devices with names that
 * start with this text.  Pass null if no prefix match is required.
 */
const struct device *shell_device_lookup(size_t idx,
				   const char *prefix);

/**
 * @brief Shell command handler prototype.
 *
 * @param sh Shell instance.
 * @param argc  Arguments count.
 * @param argv  Arguments.
 *
 * @retval 0 Successful command execution.
 * @retval 1 Help printed and command not executed.
 * @retval -EINVAL Argument validation failed.
 * @retval -ENOEXEC Command not executed.
 */
typedef int (*shell_cmd_handler)(const struct shell *sh,
				 size_t argc, char **argv);

/**
 * @brief Shell dictionary command handler prototype.
 *
 * @param sh Shell instance.
 * @param argc  Arguments count.
 * @param argv  Arguments.
 * @param data  Pointer to the user data.
 *
 * @retval 0 Successful command execution.
 * @retval 1 Help printed and command not executed.
 * @retval -EINVAL Argument validation failed.
 * @retval -ENOEXEC Command not executed.
 */
typedef int (*shell_dict_cmd_handler)(const struct shell *sh, size_t argc,
				      char **argv, void *data);

/* When entries are added to the memory section a padding is applied for
 * native_posix_64 and x86_64 targets. Adding padding to allow handle data
 * in the memory section as array.
 */
#if (defined(CONFIG_ARCH_POSIX) && defined(CONFIG_64BIT)) || defined(CONFIG_X86_64)
#define Z_SHELL_STATIC_ENTRY_PADDING 24
#else
#define Z_SHELL_STATIC_ENTRY_PADDING 0
#endif

/*
 * @brief Shell static command descriptor.
 */
struct shell_static_entry {
	const char *syntax;			/*!< Command syntax strings. */
	const char *help;			/*!< Command help string. */
	const union shell_cmd_entry *subcmd;	/*!< Pointer to subcommand. */
	shell_cmd_handler handler;		/*!< Command handler. */
	struct shell_static_args args;		/*!< Command arguments. */
	uint8_t padding[Z_SHELL_STATIC_ENTRY_PADDING];
};

/**
 * @brief Macro for defining and adding a root command (level 0) with required
 * number of arguments.
 *
 * @note Each root command shall have unique syntax. If a command will be called
 * with wrong number of arguments shell will print an error message and command
 * handler will not be called.
 *
 * @param[in] syntax	Command syntax (for example: history).
 * @param[in] subcmd	Pointer to a subcommands array.
 * @param[in] help	Pointer to a command help string.
 * @param[in] handler	Pointer to a function handler.
 * @param[in] mandatory	Number of mandatory arguments including command name.
 * @param[in] optional	Number of optional arguments.
 */
#define SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler,		   \
			       mandatory, optional)			   \
	static const struct shell_static_entry UTIL_CAT(_shell_, syntax) = \
	SHELL_CMD_ARG(syntax, subcmd, help, handler, mandatory, optional); \
	static const TYPE_SECTION_ITERABLE(union shell_cmd_entry,	   \
		UTIL_CAT(shell_cmd_, syntax), shell_root_cmds,		   \
		UTIL_CAT(shell_cmd_, syntax)				   \
	) = {								   \
		.entry = &UTIL_CAT(_shell_, syntax)			   \
	}

/**
 * @brief Macro for defining and adding a conditional root command (level 0)
 * with required number of arguments.
 *
 * @see SHELL_CMD_ARG_REGISTER for details.
 *
 * Macro can be used to create a command which can be conditionally present.
 * It is and alternative to \#ifdefs around command registration and command
 * handler. If command is disabled handler and subcommands are removed from
 * the application.
 *
 * @param[in] flag	Compile time flag. Command is present only if flag
 *			exists and equals 1.
 * @param[in] syntax	Command syntax (for example: history).
 * @param[in] subcmd	Pointer to a subcommands array.
 * @param[in] help	Pointer to a command help string.
 * @param[in] handler	Pointer to a function handler.
 * @param[in] mandatory	Number of mandatory arguments including command name.
 * @param[in] optional	Number of optional arguments.
 */
#define SHELL_COND_CMD_ARG_REGISTER(flag, syntax, subcmd, help, handler, \
					mandatory, optional) \
	COND_CODE_1(\
		flag, \
		(\
		SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, \
					mandatory, optional) \
		), \
		(\
		static shell_cmd_handler dummy_##syntax##_handler __unused = \
								handler;\
		static const union shell_cmd_entry *dummy_subcmd_##syntax \
			__unused = subcmd\
		) \
	)
/**
 * @brief Macro for defining and adding a root command (level 0) with
 * arguments.
 *
 * @note All root commands must have different name.
 *
 * @param[in] syntax	Command syntax (for example: history).
 * @param[in] subcmd	Pointer to a subcommands array.
 * @param[in] help	Pointer to a command help string.
 * @param[in] handler	Pointer to a function handler.
 */
#define SHELL_CMD_REGISTER(syntax, subcmd, help, handler) \
	SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, 0, 0)

/**
 * @brief Macro for defining and adding a conditional root command (level 0)
 * with arguments.
 *
 * @see SHELL_COND_CMD_ARG_REGISTER.
 *
 * @param[in] flag	Compile time flag. Command is present only if flag
 *			exists and equals 1.
 * @param[in] syntax	Command syntax (for example: history).
 * @param[in] subcmd	Pointer to a subcommands array.
 * @param[in] help	Pointer to a command help string.
 * @param[in] handler	Pointer to a function handler.
 */
#define SHELL_COND_CMD_REGISTER(flag, syntax, subcmd, help, handler) \
	SHELL_COND_CMD_ARG_REGISTER(flag, syntax, subcmd, help, handler, 0, 0)

/**
 * @brief Macro for creating a subcommand set. It must be used outside of any
 * function body.
 *
 * Example usage:
 * SHELL_STATIC_SUBCMD_SET_CREATE(
 *	foo,
 *	SHELL_CMD(abc, ...),
 *	SHELL_CMD(def, ...),
 *	SHELL_SUBCMD_SET_END
 * )
 *
 * @param[in] name	Name of the subcommand set.
 * @param[in] ...	List of commands created with @ref SHELL_CMD_ARG or
 *			or @ref SHELL_CMD
 */
#define SHELL_STATIC_SUBCMD_SET_CREATE(name, ...)			\
	static const struct shell_static_entry shell_##name[] = {	\
		__VA_ARGS__						\
	};								\
	static const union shell_cmd_entry name = {			\
		.entry = shell_##name					\
	}

#define Z_SHELL_UNDERSCORE(x) _##x
#define Z_SHELL_SUBCMD_NAME(...) \
	UTIL_CAT(shell_subcmds, MACRO_MAP_CAT(Z_SHELL_UNDERSCORE, __VA_ARGS__))
#define Z_SHELL_SUBCMD_SECTION_TAG(...) MACRO_MAP_CAT(Z_SHELL_UNDERSCORE, __VA_ARGS__)
#define Z_SHELL_SUBCMD_SET_SECTION_TAG(x) \
	Z_SHELL_SUBCMD_SECTION_TAG(NUM_VA_ARGS_LESS_1 x, __DEBRACKET x)
#define Z_SHELL_SUBCMD_ADD_SECTION_TAG(x, y) \
	Z_SHELL_SUBCMD_SECTION_TAG(NUM_VA_ARGS_LESS_1 x, __DEBRACKET x, y)

/** @brief Create set of subcommands.
 *
 * Commands to this set are added using @ref SHELL_SUBCMD_ADD and @ref SHELL_SUBCMD_COND_ADD.
 * Commands can be added from multiple files.
 *
 * @param[in] _name		Name of the set. @p _name is used to refer the set in the parent
 * command.
 *
 * @param[in] _parent	Set of comma separated parent commands in parenthesis, e.g.
 * (foo_cmd) if subcommands are for the root command "foo_cmd".
 */

#define SHELL_SUBCMD_SET_CREATE(_name, _parent) \
	static const TYPE_SECTION_ITERABLE(struct shell_static_entry, _name, shell_subcmds, \
					  Z_SHELL_SUBCMD_SET_SECTION_TAG(_parent))


/** @brief Conditionally add command to the set of subcommands.
 *
 * Add command to the set created with @ref SHELL_SUBCMD_SET_CREATE.
 *
 * @note The name of the section is formed as concatenation of number of parent
 * commands, names of all parent commands and own syntax. Number of parent commands
 * is added to ensure that section prefix is unique. Without it subcommands of
 * (foo) and (foo, cmd1) would mix.
 *
 * @param[in] _flag	 Compile time flag. Command is present only if flag
 *			 exists and equals 1.
 * @param[in] _parent	 Parent command sequence. Comma separated in parenthesis.
 * @param[in] _syntax	 Command syntax (for example: history).
 * @param[in] _subcmd	 Pointer to a subcommands array.
 * @param[in] _help	 Pointer to a command help string.
 * @param[in] _handler	 Pointer to a function handler.
 * @param[in] _mand	 Number of mandatory arguments including command name.
 * @param[in] _opt	 Number of optional arguments.
 */
#define SHELL_SUBCMD_COND_ADD(_flag, _parent, _syntax, _subcmd, _help, _handler, \
			   _mand, _opt) \
	COND_CODE_1(_flag, \
		(static const TYPE_SECTION_ITERABLE(struct shell_static_entry, \
					Z_SHELL_SUBCMD_NAME(__DEBRACKET _parent, _syntax), \
					shell_subcmds, \
					Z_SHELL_SUBCMD_ADD_SECTION_TAG(_parent, _syntax)) = \
			SHELL_EXPR_CMD_ARG(1, _syntax, _subcmd, _help, \
					   _handler, _mand, _opt)\
		), \
		(static shell_cmd_handler dummy_##syntax##_handler __unused = _handler;\
		 static const union shell_cmd_entry dummy_subcmd_##syntax __unused = { \
			.entry = (const struct shell_static_entry *)_subcmd\
		 } \
		) \
	)

/** @brief Add command to the set of subcommands.
 *
 * Add command to the set created with @ref SHELL_SUBCMD_SET_CREATE.
 *
 * @param[in] _parent	 Parent command sequence. Comma separated in parenthesis.
 * @param[in] _syntax	 Command syntax (for example: history).
 * @param[in] _subcmd	 Pointer to a subcommands array.
 * @param[in] _help	 Pointer to a command help string.
 * @param[in] _handler	 Pointer to a function handler.
 * @param[in] _mand	 Number of mandatory arguments including command name.
 * @param[in] _opt	 Number of optional arguments.
 */
#define SHELL_SUBCMD_ADD(_parent, _syntax, _subcmd, _help, _handler, _mand, _opt) \
	SHELL_SUBCMD_COND_ADD(1, _parent, _syntax, _subcmd, _help, _handler, _mand, _opt)

/**
 * @brief Define ending subcommands set.
 *
 */
#define SHELL_SUBCMD_SET_END {NULL}

/**
 * @brief Macro for creating a dynamic entry.
 *
 * @param[in] name	Name of the dynamic entry.
 * @param[in] get	Pointer to the function returning dynamic commands array
 */
#define SHELL_DYNAMIC_CMD_CREATE(name, get)					\
	static const TYPE_SECTION_ITERABLE(union shell_cmd_entry, name,		\
		shell_dynamic_subcmds, name) =					\
	{									\
		.dynamic_get = get						\
	}

/**
 * @brief Initializes a shell command with arguments.
 *
 * @note If a command will be called with wrong number of arguments shell will
 * print an error message and command handler will not be called.
 *
 * @param[in] syntax	 Command syntax (for example: history).
 * @param[in] subcmd	 Pointer to a subcommands array.
 * @param[in] help	 Pointer to a command help string.
 * @param[in] handler	 Pointer to a function handler.
 * @param[in] mand	 Number of mandatory arguments including command name.
 * @param[in] opt	 Number of optional arguments.
 */
#define SHELL_CMD_ARG(syntax, subcmd, help, handler, mand, opt) \
	SHELL_EXPR_CMD_ARG(1, syntax, subcmd, help, handler, mand, opt)

/**
 * @brief Initializes a conditional shell command with arguments.
 *
 * @see SHELL_CMD_ARG. Based on the flag, creates a valid entry or an empty
 * command which is ignored by the shell. It is an alternative to \#ifdefs
 * around command registration and command handler. However, empty structure is
 * present in the flash even if command is disabled (subcommands and handler are
 * removed). Macro internally handles case if flag is not defined so flag must
 * be provided without any wrapper, e.g.: SHELL_COND_CMD_ARG(CONFIG_FOO, ...)
 *
 * @param[in] flag	 Compile time flag. Command is present only if flag
 *			 exists and equals 1.
 * @param[in] syntax	 Command syntax (for example: history).
 * @param[in] subcmd	 Pointer to a subcommands array.
 * @param[in] help	 Pointer to a command help string.
 * @param[in] handler	 Pointer to a function handler.
 * @param[in] mand	 Number of mandatory arguments including command name.
 * @param[in] opt	 Number of optional arguments.
 */
#define SHELL_COND_CMD_ARG(flag, syntax, subcmd, help, handler, mand, opt) \
	SHELL_EXPR_CMD_ARG(IS_ENABLED(flag), syntax, subcmd, help, \
			  handler, mand, opt)

/**
 * @brief Initializes a conditional shell command with arguments if expression
 *	  gives non-zero result at compile time.
 *
 * @see SHELL_CMD_ARG. Based on the expression, creates a valid entry or an
 * empty command which is ignored by the shell. It should be used instead of
 * @ref SHELL_COND_CMD_ARG if condition is not a single configuration flag,
 * e.g.:
 * SHELL_EXPR_CMD_ARG(IS_ENABLED(CONFIG_FOO) &&
 *		      IS_ENABLED(CONFIG_FOO_SETTING_1), ...)
 *
 * @param[in] _expr	 Expression.
 * @param[in] _syntax	 Command syntax (for example: history).
 * @param[in] _subcmd	 Pointer to a subcommands array.
 * @param[in] _help	 Pointer to a command help string.
 * @param[in] _handler	 Pointer to a function handler.
 * @param[in] _mand	 Number of mandatory arguments including command name.
 * @param[in] _opt	 Number of optional arguments.
 */
#define SHELL_EXPR_CMD_ARG(_expr, _syntax, _subcmd, _help, _handler, \
			   _mand, _opt) \
	{ \
		.syntax = (_expr) ? (const char *)STRINGIFY(_syntax) : "", \
		.help  = (_expr) ? (const char *)_help : NULL, \
		.subcmd = (const union shell_cmd_entry *)((_expr) ? \
				_subcmd : NULL), \
		.handler = (shell_cmd_handler)((_expr) ? _handler : NULL), \
		.args = { .mandatory = _mand, .optional = _opt} \
	}

/**
 * @brief Initializes a shell command.
 *
 * @param[in] _syntax	Command syntax (for example: history).
 * @param[in] _subcmd	Pointer to a subcommands array.
 * @param[in] _help	Pointer to a command help string.
 * @param[in] _handler	Pointer to a function handler.
 */
#define SHELL_CMD(_syntax, _subcmd, _help, _handler) \
	SHELL_CMD_ARG(_syntax, _subcmd, _help, _handler, 0, 0)

/**
 * @brief Initializes a conditional shell command.
 *
 * @see SHELL_COND_CMD_ARG.
 *
 * @param[in] _flag	Compile time flag. Command is present only if flag
 *			exists and equals 1.
 * @param[in] _syntax	Command syntax (for example: history).
 * @param[in] _subcmd	Pointer to a subcommands array.
 * @param[in] _help	Pointer to a command help string.
 * @param[in] _handler	Pointer to a function handler.
 */
#define SHELL_COND_CMD(_flag, _syntax, _subcmd, _help, _handler) \
	SHELL_COND_CMD_ARG(_flag, _syntax, _subcmd, _help, _handler, 0, 0)

/**
 * @brief Initializes shell command if expression gives non-zero result at
 *	  compile time.
 *
 * @see SHELL_EXPR_CMD_ARG.
 *
 * @param[in] _expr	Compile time expression. Command is present only if
 *			expression is non-zero.
 * @param[in] _syntax	Command syntax (for example: history).
 * @param[in] _subcmd	Pointer to a subcommands array.
 * @param[in] _help	Pointer to a command help string.
 * @param[in] _handler	Pointer to a function handler.
 */
#define SHELL_EXPR_CMD(_expr, _syntax, _subcmd, _help, _handler) \
	SHELL_EXPR_CMD_ARG(_expr, _syntax, _subcmd, _help, _handler, 0, 0)

/* Internal macro used for creating handlers for dictionary commands. */
#define Z_SHELL_CMD_DICT_HANDLER_CREATE(_data, _handler)		\
static int UTIL_CAT(UTIL_CAT(cmd_dict_, UTIL_CAT(_handler, _)),		\
			GET_ARG_N(1, __DEBRACKET _data))(		\
		const struct shell *sh, size_t argc, char **argv)	\
{									\
	return _handler(sh, argc, argv,					\
			(void *)GET_ARG_N(2, __DEBRACKET _data));	\
}

/* Internal macro used for creating dictionary commands. */
#define SHELL_CMD_DICT_CREATE(_data, _handler)				\
	SHELL_CMD_ARG(GET_ARG_N(1, __DEBRACKET _data), NULL, GET_ARG_N(3, __DEBRACKET _data),	\
		UTIL_CAT(UTIL_CAT(cmd_dict_, UTIL_CAT(_handler, _)),	\
			GET_ARG_N(1, __DEBRACKET _data)), 1, 0)

/**
 * @brief Initializes shell dictionary commands.
 *
 * This is a special kind of static commands. Dictionary commands can be used
 * every time you want to use a pair: (string <-> corresponding data) in
 * a command handler. The string is usually a verbal description of a given
 * data. The idea is to use the string as a command syntax that can be prompted
 * by the shell and corresponding data can be used to process the command.
 *
 * @param[in] _name	Name of the dictionary subcommand set
 * @param[in] _handler	Command handler common for all dictionary commands.
 *			@see shell_dict_cmd_handler
 * @param[in] ...	Dictionary pairs: (command_syntax, value). Value will be
 *			passed to the _handler as user data.
 *
 * Example usage:
 *	static int my_handler(const struct shell *sh,
 *			      size_t argc, char **argv, void *data)
 *	{
 *		int val = (int)data;
 *
 *		shell_print(sh, "(syntax, value) : (%s, %d)", argv[0], val);
 *		return 0;
 *	}
 *
 *	SHELL_SUBCMD_DICT_SET_CREATE(sub_dict_cmds, my_handler,
 *		(value_0, 0, "value 0"), (value_1, 1, "value 1"),
 *		(value_2, 2, "value 2"), (value_3, 3, "value 3")
 *	);
 *	SHELL_CMD_REGISTER(dictionary, &sub_dict_cmds, NULL, NULL);
 */
#define SHELL_SUBCMD_DICT_SET_CREATE(_name, _handler, ...)		\
	FOR_EACH_FIXED_ARG(Z_SHELL_CMD_DICT_HANDLER_CREATE, (),		\
			   _handler, __VA_ARGS__)			\
	SHELL_STATIC_SUBCMD_SET_CREATE(_name,				\
		FOR_EACH_FIXED_ARG(SHELL_CMD_DICT_CREATE, (,), _handler, __VA_ARGS__),	\
		SHELL_SUBCMD_SET_END					\
	)

/**
 * @internal @brief Internal shell state in response to data received from the
 * terminal.
 */
enum shell_receive_state {
	SHELL_RECEIVE_DEFAULT,
	SHELL_RECEIVE_ESC,
	SHELL_RECEIVE_ESC_SEQ,
	SHELL_RECEIVE_TILDE_EXP
};

/**
 * @internal @brief Internal shell state.
 */
enum shell_state {
	SHELL_STATE_UNINITIALIZED,
	SHELL_STATE_INITIALIZED,
	SHELL_STATE_ACTIVE,
	SHELL_STATE_PANIC_MODE_ACTIVE,  /*!< Panic activated.*/
	SHELL_STATE_PANIC_MODE_INACTIVE /*!< Panic requested, not supported.*/
};

/** @brief Shell transport event. */
enum shell_transport_evt {
	SHELL_TRANSPORT_EVT_RX_RDY,
	SHELL_TRANSPORT_EVT_TX_RDY
};

typedef void (*shell_transport_handler_t)(enum shell_transport_evt evt,
					  void *context);


typedef void (*shell_uninit_cb_t)(const struct shell *sh, int res);

/** @brief Bypass callback.
 *
 * @param sh Shell instance.
 * @param data  Raw data from transport.
 * @param len   Data length.
 */
typedef void (*shell_bypass_cb_t)(const struct shell *sh,
				  uint8_t *data,
				  size_t len);

struct shell_transport;

/**
 * @brief Unified shell transport interface.
 */
struct shell_transport_api {
	/**
	 * @brief Function for initializing the shell transport interface.
	 *
	 * @param[in] transport   Pointer to the transfer instance.
	 * @param[in] config      Pointer to instance configuration.
	 * @param[in] evt_handler Event handler.
	 * @param[in] context     Pointer to the context passed to event
	 *			  handler.
	 *
	 * @return Standard error code.
	 */
	int (*init)(const struct shell_transport *transport,
		    const void *config,
		    shell_transport_handler_t evt_handler,
		    void *context);

	/**
	 * @brief Function for uninitializing the shell transport interface.
	 *
	 * @param[in] transport  Pointer to the transfer instance.
	 *
	 * @return Standard error code.
	 */
	int (*uninit)(const struct shell_transport *transport);

	/**
	 * @brief Function for enabling transport in given TX mode.
	 *
	 * Function can be used to reconfigure TX to work in blocking mode.
	 *
	 * @param transport   Pointer to the transfer instance.
	 * @param blocking_tx If true, the transport TX is enabled in blocking
	 *		      mode.
	 *
	 * @return NRF_SUCCESS on successful enabling, error otherwise (also if
	 * not supported).
	 */
	int (*enable)(const struct shell_transport *transport,
		      bool blocking_tx);

	/**
	 * @brief Function for writing data to the transport interface.
	 *
	 * @param[in]  transport  Pointer to the transfer instance.
	 * @param[in]  data       Pointer to the source buffer.
	 * @param[in]  length     Source buffer length.
	 * @param[out] cnt        Pointer to the sent bytes counter.
	 *
	 * @return Standard error code.
	 */
	int (*write)(const struct shell_transport *transport,
		     const void *data, size_t length, size_t *cnt);

	/**
	 * @brief Function for reading data from the transport interface.
	 *
	 * @param[in]  p_transport  Pointer to the transfer instance.
	 * @param[in]  p_data       Pointer to the destination buffer.
	 * @param[in]  length       Destination buffer length.
	 * @param[out] cnt          Pointer to the received bytes counter.
	 *
	 * @return Standard error code.
	 */
	int (*read)(const struct shell_transport *transport,
		    void *data, size_t length, size_t *cnt);

	/**
	 * @brief Function called in shell thread loop.
	 *
	 * Can be used for backend operations that require longer execution time
	 *
	 * @param[in] transport Pointer to the transfer instance.
	 */
	void (*update)(const struct shell_transport *transport);

};

struct shell_transport {
	const struct shell_transport_api *api;
	void *ctx;
};

/**
 * @brief Shell statistics structure.
 */
struct shell_stats {
	atomic_t log_lost_cnt; /*!< Lost log counter.*/
};

#ifdef CONFIG_SHELL_STATS
#define Z_SHELL_STATS_DEFINE(_name) static struct shell_stats _name##_stats
#define Z_SHELL_STATS_PTR(_name) (&(_name##_stats))
#else
#define Z_SHELL_STATS_DEFINE(_name)
#define Z_SHELL_STATS_PTR(_name) NULL
#endif /* CONFIG_SHELL_STATS */

/**
 * @internal @brief Flags for shell backend configuration.
 */
struct shell_backend_config_flags {
	uint32_t insert_mode :1; /*!< Controls insert mode for text introduction */
	uint32_t echo        :1; /*!< Controls shell echo */
	uint32_t obscure     :1; /*!< If echo on, print asterisk instead */
	uint32_t mode_delete :1; /*!< Operation mode of backspace key */
	uint32_t use_colors  :1; /*!< Controls colored syntax */
	uint32_t use_vt100   :1; /*!< Controls VT100 commands usage in shell */
};

BUILD_ASSERT((sizeof(struct shell_backend_config_flags) == sizeof(uint32_t)),
	     "Structure must fit in 4 bytes");

/**
 * @internal @brief Default backend configuration.
 */
#define SHELL_DEFAULT_BACKEND_CONFIG_FLAGS				\
{									\
	.insert_mode	= 0,						\
	.echo		= 1,						\
	.obscure	= IS_ENABLED(CONFIG_SHELL_START_OBSCURED),	\
	.mode_delete	= 1,						\
	.use_colors	= 1,						\
	.use_vt100	= 1,						\
};

struct shell_backend_ctx_flags {
	uint32_t processing   :1; /*!< Shell is executing process function */
	uint32_t tx_rdy       :1;
	uint32_t history_exit :1; /*!< Request to exit history mode */
	uint32_t last_nl      :8; /*!< Last received new line character */
	uint32_t cmd_ctx      :1; /*!< Shell is executing command */
	uint32_t print_noinit :1; /*!< Print request from not initialized shell */
	uint32_t sync_mode    :1; /*!< Shell in synchronous mode */
};

BUILD_ASSERT((sizeof(struct shell_backend_ctx_flags) == sizeof(uint32_t)),
	     "Structure must fit in 4 bytes");

/**
 * @internal @brief Union for internal shell usage.
 */
union shell_backend_cfg {
	atomic_t value;
	struct shell_backend_config_flags flags;
};

/**
 * @internal @brief Union for internal shell usage.
 */
union shell_backend_ctx {
	uint32_t value;
	struct shell_backend_ctx_flags flags;
};

enum shell_signal {
	SHELL_SIGNAL_RXRDY,
	SHELL_SIGNAL_LOG_MSG,
	SHELL_SIGNAL_KILL,
	SHELL_SIGNAL_TXDONE, /* TXDONE must be last one before SHELL_SIGNALS */
	SHELL_SIGNALS
};

/**
 * @brief Shell instance context.
 */
struct shell_ctx {
	const char *prompt; /*!< shell current prompt. */

	enum shell_state state; /*!< Internal module state.*/
	enum shell_receive_state receive_state;/*!< Escape sequence indicator.*/

	/*!< Currently executed command.*/
	struct shell_static_entry active_cmd;

	/* New root command. If NULL shell uses default root commands. */
	const struct shell_static_entry *selected_cmd;

	/*!< VT100 color and cursor position, terminal width.*/
	struct shell_vt100_ctx vt100_ctx;

	/*!< Callback called from shell thread context when unitialization is
	 * completed just before aborting shell thread.
	 */
	shell_uninit_cb_t uninit_cb;

	/*!< When bypass is set, all incoming data is passed to the callback. */
	shell_bypass_cb_t bypass;

#if defined CONFIG_SHELL_GETOPT
	/*!< getopt context for a shell backend. */
	struct getopt_state getopt;
#endif

	uint16_t cmd_buff_len; /*!< Command length.*/
	uint16_t cmd_buff_pos; /*!< Command buffer cursor position.*/

	uint16_t cmd_tmp_buff_len; /*!< Command length in tmp buffer.*/

	/*!< Command input buffer.*/
	char cmd_buff[CONFIG_SHELL_CMD_BUFF_SIZE];

	/*!< Command temporary buffer.*/
	char temp_buff[CONFIG_SHELL_CMD_BUFF_SIZE];

	/*!< Printf buffer size.*/
	char printf_buff[CONFIG_SHELL_PRINTF_BUFF_SIZE];

	volatile union shell_backend_cfg cfg;
	volatile union shell_backend_ctx ctx;

	struct k_poll_signal signals[SHELL_SIGNALS];

	/*!< Events that should be used only internally by shell thread.
	 * Event for SHELL_SIGNAL_TXDONE is initialized but unused.
	 */
	struct k_poll_event events[SHELL_SIGNALS];

	struct k_mutex wr_mtx;
	k_tid_t tid;
};

extern const struct log_backend_api log_backend_shell_api;

/**
 * @brief Flags for setting shell output newline sequence.
 */
enum shell_flag {
	SHELL_FLAG_CRLF_DEFAULT	= (1<<0),	/* Do not map CR or LF */
	SHELL_FLAG_OLF_CRLF	= (1<<1)	/* Map LF to CRLF on output */
};

/**
 * @brief Shell instance internals.
 */
struct shell {
	const char *default_prompt; /*!< shell default prompt. */

	const struct shell_transport *iface; /*!< Transport interface.*/
	struct shell_ctx *ctx; /*!< Internal context.*/

	struct shell_history *history;

	const enum shell_flag shell_flag;

	const struct shell_fprintf *fprintf_ctx;

	struct shell_stats *stats;

	const struct shell_log_backend *log_backend;

	LOG_INSTANCE_PTR_DECLARE(log);

	const char *thread_name;
	struct k_thread *thread;
	k_thread_stack_t *stack;
};

extern void z_shell_print_stream(const void *user_ctx, const char *data,
				 size_t data_len);
/**
 * @brief Macro for defining a shell instance.
 *
 * @param[in] _name		Instance name.
 * @param[in] _prompt		Shell default prompt string.
 * @param[in] _transport_iface	Pointer to the transport interface.
 * @param[in] _log_queue_size	Logger processing queue size.
 * @param[in] _log_timeout	Logger thread timeout in milliseconds on full
 *				log queue. If queue is full logger thread is
 *				blocked for given amount of time before log
 *				message is dropped.
 * @param[in] _shell_flag	Shell output newline sequence.
 */
#define SHELL_DEFINE(_name, _prompt, _transport_iface,			      \
		     _log_queue_size, _log_timeout, _shell_flag)	      \
	static const struct shell _name;				      \
	static struct shell_ctx UTIL_CAT(_name, _ctx);			      \
	static uint8_t _name##_out_buffer[CONFIG_SHELL_PRINTF_BUFF_SIZE];     \
	Z_SHELL_LOG_BACKEND_DEFINE(_name, _name##_out_buffer,		      \
				 CONFIG_SHELL_PRINTF_BUFF_SIZE,		      \
				 _log_queue_size, _log_timeout);	      \
	Z_SHELL_HISTORY_DEFINE(_name##_history, CONFIG_SHELL_HISTORY_BUFFER); \
	Z_SHELL_FPRINTF_DEFINE(_name##_fprintf, &_name, _name##_out_buffer,   \
			     CONFIG_SHELL_PRINTF_BUFF_SIZE,		      \
			     true, z_shell_print_stream);		      \
	LOG_INSTANCE_REGISTER(shell, _name, CONFIG_SHELL_LOG_LEVEL);	      \
	Z_SHELL_STATS_DEFINE(_name);					      \
	static K_KERNEL_STACK_DEFINE(_name##_stack, CONFIG_SHELL_STACK_SIZE); \
	static struct k_thread _name##_thread;				      \
	static const STRUCT_SECTION_ITERABLE(shell, _name) = {		      \
		.default_prompt = _prompt,				      \
		.iface = _transport_iface,				      \
		.ctx = &UTIL_CAT(_name, _ctx),				      \
		.history = IS_ENABLED(CONFIG_SHELL_HISTORY) ?		      \
				&_name##_history : NULL,		      \
		.shell_flag = _shell_flag,				      \
		.fprintf_ctx = &_name##_fprintf,			      \
		.stats = Z_SHELL_STATS_PTR(_name),			      \
		.log_backend = Z_SHELL_LOG_BACKEND_PTR(_name),		      \
		LOG_INSTANCE_PTR_INIT(log, shell, _name)		      \
		.thread_name = STRINGIFY(_name),			      \
		.thread = &_name##_thread,				      \
		.stack = _name##_stack					      \
	}

/**
 * @brief Function for initializing a transport layer and internal shell state.
 *
 * @param[in] sh		Pointer to shell instance.
 * @param[in] transport_config	Transport configuration during initialization.
 * @param[in] cfg_flags		Initial backend configuration flags.
 *				Shell will copy this data.
 * @param[in] log_backend	If true, the console will be used as logger
 *				backend.
 * @param[in] init_log_level	Default severity level for the logger.
 *
 * @return Standard error code.
 */
int shell_init(const struct shell *sh, const void *transport_config,
	       struct shell_backend_config_flags cfg_flags,
	       bool log_backend, uint32_t init_log_level);

/**
 * @brief Uninitializes the transport layer and the internal shell state.
 *
 * @param sh Pointer to shell instance.
 * @param cb Callback called when uninitialization is completed.
 */
void shell_uninit(const struct shell *sh, shell_uninit_cb_t cb);

/**
 * @brief Function for starting shell processing.
 *
 * @param sh Pointer to the shell instance.
 *
 * @return Standard error code.
 */
int shell_start(const struct shell *sh);

/**
 * @brief Function for stopping shell processing.
 *
 * @param sh Pointer to shell instance.
 *
 * @return Standard error code.
 */
int shell_stop(const struct shell *sh);

/**
 * @brief Terminal default text color for shell_fprintf function.
 */
#define SHELL_NORMAL	SHELL_VT100_COLOR_DEFAULT

/**
 * @brief Green text color for shell_fprintf function.
 */
#define SHELL_INFO	SHELL_VT100_COLOR_GREEN

/**
 * @brief Cyan text color for shell_fprintf function.
 */
#define SHELL_OPTION	SHELL_VT100_COLOR_CYAN

/**
 * @brief Yellow text color for shell_fprintf function.
 */
#define SHELL_WARNING	SHELL_VT100_COLOR_YELLOW

/**
 * @brief Red text color for shell_fprintf function.
 */
#define SHELL_ERROR	SHELL_VT100_COLOR_RED

/**
 * @brief printf-like function which sends formatted data stream to the shell.
 *
 * This function can be used from the command handler or from threads, but not
 * from an interrupt context.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] color	Printed text color.
 * @param[in] fmt	Format string.
 * @param[in] ...	List of parameters to print.
 */
void __printf_like(3, 4) shell_fprintf(const struct shell *sh,
				       enum shell_vt100_color color,
				       const char *fmt, ...);

/**
 * @brief vprintf-like function which sends formatted data stream to the shell.
 *
 * This function can be used from the command handler or from threads, but not
 * from an interrupt context. It is similar to shell_fprintf() but takes a
 * va_list instead of variable arguments.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] color	Printed text color.
 * @param[in] fmt	Format string.
 * @param[in] args	List of parameters to print.
 */
void shell_vfprintf(const struct shell *sh, enum shell_vt100_color color,
		   const char *fmt, va_list args);

/**
 * @brief Print a line of data in hexadecimal format.
 *
 * Each line shows the offset, bytes and then ASCII representation.
 *
 * For example:
 *
 * 00008010: 20 25 00 20 2f 48 00 08  80 05 00 20 af 46 00
 *	| %. /H.. ... .F. |
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] offset	Offset to show for this line.
 * @param[in] data	Pointer to data.
 * @param[in] len	Length of data.
 */
void shell_hexdump_line(const struct shell *sh, unsigned int offset,
			const uint8_t *data, size_t len);

/**
 * @brief Print data in hexadecimal format.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] data	Pointer to data.
 * @param[in] len	Length of data.
 */
void shell_hexdump(const struct shell *sh, const uint8_t *data, size_t len);

/**
 * @brief Print info message to the shell.
 *
 * See @ref shell_fprintf.
 *
 * @param[in] _sh Pointer to the shell instance.
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define shell_info(_sh, _ft, ...) \
	shell_fprintf(_sh, SHELL_INFO, _ft "\n", ##__VA_ARGS__)

/**
 * @brief Print normal message to the shell.
 *
 * See @ref shell_fprintf.
 *
 * @param[in] _sh Pointer to the shell instance.
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define shell_print(_sh, _ft, ...) \
	shell_fprintf(_sh, SHELL_NORMAL, _ft "\n", ##__VA_ARGS__)

/**
 * @brief Print warning message to the shell.
 *
 * See @ref shell_fprintf.
 *
 * @param[in] _sh Pointer to the shell instance.
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define shell_warn(_sh, _ft, ...) \
	shell_fprintf(_sh, SHELL_WARNING, _ft "\n", ##__VA_ARGS__)

/**
 * @brief Print error message to the shell.
 *
 * See @ref shell_fprintf.
 *
 * @param[in] _sh Pointer to the shell instance.
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define shell_error(_sh, _ft, ...) \
	shell_fprintf(_sh, SHELL_ERROR, _ft "\n", ##__VA_ARGS__)

/**
 * @brief Process function, which should be executed when data is ready in the
 *	  transport interface. To be used if shell thread is disabled.
 *
 * @param[in] sh Pointer to the shell instance.
 */
void shell_process(const struct shell *sh);

/**
 * @brief Change displayed shell prompt.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] prompt	New shell prompt.
 *
 * @return 0		Success.
 * @return -EINVAL	Pointer to new prompt is not correct.
 */
int shell_prompt_change(const struct shell *sh, const char *prompt);

/**
 * @brief Prints the current command help.
 *
 * Function will print a help string with: the currently entered command
 * and subcommands (if they exist).
 *
 * @param[in] sh      Pointer to the shell instance.
 */
void shell_help(const struct shell *sh);

/* @brief Command's help has been printed */
#define SHELL_CMD_HELP_PRINTED	(1)

/** @brief Execute command.
 *
 * Pass command line to shell to execute.
 *
 * Note: This by no means makes any of the commands a stable interface, so
 *	 this function should only be used for debugging/diagnostic.
 *
 *	 This function must not be called from shell command context!

 *
 * @param[in] sh	Pointer to the shell instance.
 *			It can be NULL when the
 *			@kconfig{CONFIG_SHELL_BACKEND_DUMMY} option is enabled.
 * @param[in] cmd	Command to be executed.
 *
 * @return		Result of the execution
 */
int shell_execute_cmd(const struct shell *sh, const char *cmd);

/** @brief Set root command for all shell instances.
 *
 * It allows setting from the code the root command. It is an equivalent of
 * calling select command with one of the root commands as the argument
 * (e.g "select log") except it sets command for all shell instances.
 *
 * @param cmd String with one of the root commands or null pointer to reset.
 *
 * @retval 0 if root command is set.
 * @retval -EINVAL if invalid root command is provided.
 */
int shell_set_root_cmd(const char *cmd);

/** @brief Set bypass callback.
 *
 * Bypass callback is called whenever data is received. Shell is bypassed and
 * data is passed directly to the callback. Use null to disable bypass functionality.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] bypass	Bypass callback or null to disable.
 */
void shell_set_bypass(const struct shell *sh, shell_bypass_cb_t bypass);

/** @brief Get shell readiness to execute commands.
 *
 * @param[in] sh	Pointer to the shell instance.
 *
 * @retval true		Shell backend is ready to execute commands.
 * @retval false	Shell backend is not initialized or not started.
 */
bool shell_ready(const struct shell *sh);

/**
 * @brief Allow application to control text insert mode.
 * Value is modified atomically and the previous value is returned.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] val	Insert mode.
 *
 * @retval 0 or 1: previous value
 * @retval -EINVAL if shell is NULL.
 */
int shell_insert_mode_set(const struct shell *sh, bool val);

/**
 * @brief Allow application to control whether terminal output uses colored
 * syntax.
 * Value is modified atomically and the previous value is returned.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] val	Color mode.
 *
 * @retval 0 or 1: previous value
 * @retval -EINVAL if shell is NULL.
 */
int shell_use_colors_set(const struct shell *sh, bool val);

/**
 * @brief Allow application to control whether terminal is using vt100 commands.
 * Value is modified atomically and the previous value is returned.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] val	vt100 mode.
 *
 * @retval 0 or 1: previous value
 * @retval -EINVAL if shell is NULL.
 */
int shell_use_vt100_set(const struct shell *sh, bool val);

/**
 * @brief Allow application to control whether user input is echoed back.
 * Value is modified atomically and the previous value is returned.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] val	Echo mode.
 *
 * @retval 0 or 1: previous value
 * @retval -EINVAL if shell is NULL.
 */
int shell_echo_set(const struct shell *sh, bool val);

/**
 * @brief Allow application to control whether user input is obscured with
 * asterisks -- useful for implementing passwords.
 * Value is modified atomically and the previous value is returned.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] obscure	Obscure mode.
 *
 * @retval 0 or 1: previous value.
 * @retval -EINVAL if shell is NULL.
 */
int shell_obscure_set(const struct shell *sh, bool obscure);

/**
 * @brief Allow application to control whether the delete key backspaces or
 * deletes.
 * Value is modified atomically and the previous value is returned.
 *
 * @param[in] sh	Pointer to the shell instance.
 * @param[in] val	Delete mode.
 *
 * @retval 0 or 1: previous value
 * @retval -EINVAL if shell is NULL.
 */
int shell_mode_delete_set(const struct shell *sh, bool val);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H__ */
