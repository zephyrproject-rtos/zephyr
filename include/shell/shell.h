/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_H__
#define SHELL_H__

#include <zephyr.h>
#include <shell/shell_types.h>
#include <shell/shell_history.h>
#include <shell/shell_fprintf.h>
#include <shell/shell_log_backend.h>
#include <logging/log_instance.h>
#include <logging/log.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHELL_RX_BUFF_SIZE 16

#ifndef CONFIG_SHELL_CMD_BUFF_SIZE
#define CONFIG_SHELL_CMD_BUFF_SIZE 0
#endif

#ifndef CONFIG_SHELL_PRINTF_BUFF_SIZE
#define CONFIG_SHELL_PRINTF_BUFF_SIZE 0
#endif

#ifndef CONFIG_SHELL_HISTORY_BUFFER
#define CONFIG_SHELL_HISTORY_BUFFER 0
#endif

#define SHELL_CMD_ROOT_LVL		(0u)

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
struct shell_cmd_entry {
	bool is_dynamic;
	union union_cmd_entry {
		/*!< Pointer to function returning dynamic commands.*/
		shell_dynamic_get dynamic_get;

		/*!< Pointer to array of static commands. */
		const struct shell_static_entry *entry;
	} u;
};

struct shell;

struct shell_static_args {
	u8_t mandatory; /*!< Number of mandatory arguments. */
	u8_t optional;  /*!< Number of optional arguments. */
};

/**
 * @brief Shell command handler prototype.
 *
 * @param shell Shell instance.
 * @param argc  Arguments count.
 * @param argv  Arguments.
 *
 * @retval 0 Successful command execution.
 * @retval 1 Help printed and command not executed.
 * @retval -EINVAL Argument validation failed.
 * @retval -ENOEXEC Command not executed.
 */
typedef int (*shell_cmd_handler)(const struct shell *shell,
				 size_t argc, char **argv);

/*
 * @brief Shell static command descriptor.
 */
struct shell_static_entry {
	const char *syntax;			/*!< Command syntax strings. */
	const char *help;			/*!< Command help string. */
	const struct shell_cmd_entry *subcmd;	/*!< Pointer to subcommand. */
	shell_cmd_handler handler;		/*!< Command handler. */
	struct shell_static_args args;		/*!< Command arguments. */
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
 * @param[in] mandatory	Number of mandatory arguments.
 * @param[in] optional	Number of optional arguments.
 */
#define SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler,		   \
			       mandatory, optional)			   \
	static const struct shell_static_entry UTIL_CAT(_shell_, syntax) = \
	SHELL_CMD_ARG(syntax, subcmd, help, handler, mandatory, optional); \
	static const struct shell_cmd_entry UTIL_CAT(shell_cmd_, syntax)   \
	__attribute__ ((section("."					   \
			STRINGIFY(UTIL_CAT(shell_root_cmd_, syntax)))))	   \
	__attribute__((used)) = {					   \
		.is_dynamic = false,					   \
		.u = {.entry = &UTIL_CAT(_shell_, syntax)}		   \
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
 * @param[in] mandatory	Number of mandatory arguments.
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
		static shell_cmd_handler dummy_##syntax##handler \
			__attribute__((unused)) = handler;\
		static const struct shell_cmd_entry *dummy_subcmd_##syntax \
			__attribute__((unused)) = subcmd\
		)\
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
	static const struct shell_cmd_entry name = {			\
		.is_dynamic = false,					\
		.u = { .entry = shell_##name }				\
	}

/**
 * @brief Deprecated macro for creating a subcommand set.
 *
 * It must be used outside of any function body.
 *
 * @param[in] name	Name of the subcommand set.
 */
#define SHELL_CREATE_STATIC_SUBCMD_SET(name)			\
	__DEPRECATED_MACRO					\
	static const struct shell_static_entry shell_##name[];	\
	static const struct shell_cmd_entry name = {		\
		.is_dynamic = false,				\
		.u.entry = shell_##name				\
	};							\
	static const struct shell_static_entry shell_##name[] =


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
#define SHELL_DYNAMIC_CMD_CREATE(name, get)		\
	static const struct shell_cmd_entry name = {	\
		.is_dynamic = true,			\
		.u = { .dynamic_get = get }		\
	}

/**
 * @brief Deprecated macro for creating a dynamic entry.
 *
 * @param[in] name	Name of the dynamic entry.
 * @param[in] get	Pointer to the function returning dynamic commands array
 */
#define SHELL_CREATE_DYNAMIC_CMD(name, get)		\
	__DEPRECATED_MACRO SHELL_DYNAMIC_CMD_CREATE(name, get)

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
 * @param[in] mand	 Number of mandatory arguments.
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
 * @param[in] mand	 Number of mandatory arguments.
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
 * @param[in] _mand	 Number of mandatory arguments.
 * @param[in] _opt	 Number of optional arguments.
 */
#define SHELL_EXPR_CMD_ARG(_expr, _syntax, _subcmd, _help, _handler, \
			   _mand, _opt) \
	{ \
		.syntax = (_expr) ? (const char *)STRINGIFY(_syntax) : "", \
		.help  = (_expr) ? (const char *)_help : NULL, \
		.subcmd = (const struct shell_cmd_entry *)((_expr) ? \
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
	u32_t log_lost_cnt; /*!< Lost log counter.*/
};

#ifdef CONFIG_SHELL_STATS
#define SHELL_STATS_DEFINE(_name) static struct shell_stats _name##_stats
#define SHELL_STATS_PTR(_name) (&(_name##_stats))
#else
#define SHELL_STATS_DEFINE(_name)
#define SHELL_STATS_PTR(_name) NULL
#endif /* CONFIG_SHELL_STATS */

/**
 * @internal @brief Flags for internal shell usage.
 */
struct shell_flags {
	u32_t insert_mode :1; /*!< Controls insert mode for text introduction.*/
	u32_t use_colors  :1; /*!< Controls colored syntax.*/
	u32_t echo        :1; /*!< Controls shell echo.*/
	u32_t processing  :1; /*!< Shell is executing process function.*/
	u32_t tx_rdy      :1;
	u32_t mode_delete :1; /*!< Operation mode of backspace key */
	u32_t history_exit:1; /*!< Request to exit history mode */
	u32_t cmd_ctx	  :1; /*!< Shell is executing command */
	u32_t last_nl     :8; /*!< Last received new line character */
};

BUILD_ASSERT_MSG((sizeof(struct shell_flags) == sizeof(u32_t)),
		 "Structure must fit in 4 bytes");


/**
 * @internal @brief Union for internal shell usage.
 */
union shell_internal {
	u32_t value;
	struct shell_flags flags;
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

	u16_t cmd_buff_len; /*!< Command length.*/
	u16_t cmd_buff_pos; /*!< Command buffer cursor position.*/

	u16_t cmd_tmp_buff_len; /*!< Command length in tmp buffer.*/

	/*!< Command input buffer.*/
	char cmd_buff[CONFIG_SHELL_CMD_BUFF_SIZE];

	/*!< Command temporary buffer.*/
	char temp_buff[CONFIG_SHELL_CMD_BUFF_SIZE];

	/*!< Printf buffer size.*/
	char printf_buff[CONFIG_SHELL_PRINTF_BUFF_SIZE];

	volatile union shell_internal internal; /*!< Internal shell data.*/

	struct k_poll_signal signals[SHELL_SIGNALS];
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

extern void shell_print_stream(const void *user_ctx, const char *data,
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
	static u8_t _name##_out_buffer[CONFIG_SHELL_PRINTF_BUFF_SIZE];	      \
	SHELL_LOG_BACKEND_DEFINE(_name, _name##_out_buffer,		      \
				 CONFIG_SHELL_PRINTF_BUFF_SIZE,		      \
				 _log_queue_size, _log_timeout);	      \
	SHELL_HISTORY_DEFINE(_name##_history, CONFIG_SHELL_HISTORY_BUFFER);   \
	SHELL_FPRINTF_DEFINE(_name##_fprintf, &_name, _name##_out_buffer,     \
			     CONFIG_SHELL_PRINTF_BUFF_SIZE,		      \
			     true, shell_print_stream);			      \
	LOG_INSTANCE_REGISTER(shell, _name, CONFIG_SHELL_LOG_LEVEL);	      \
	SHELL_STATS_DEFINE(_name);					      \
	static K_THREAD_STACK_DEFINE(_name##_stack, CONFIG_SHELL_STACK_SIZE); \
	static struct k_thread _name##_thread;				      \
	static const struct shell _name = {				      \
		.default_prompt = _prompt,				      \
		.iface = _transport_iface,				      \
		.ctx = &UTIL_CAT(_name, _ctx),				      \
		.history = IS_ENABLED(CONFIG_SHELL_HISTORY) ?		      \
				&_name##_history : NULL,		      \
		.shell_flag = _shell_flag,				      \
		.fprintf_ctx = &_name##_fprintf,			      \
		.stats = SHELL_STATS_PTR(_name),			      \
		.log_backend = SHELL_LOG_BACKEND_PTR(_name),		      \
		LOG_INSTANCE_PTR_INIT(log, shell, _name)		      \
		.thread_name = STRINGIFY(_name),			      \
		.thread = &_name##_thread,				      \
		.stack = _name##_stack					      \
	}

/**
 * @brief Function for initializing a transport layer and internal shell state.
 *
 * @param[in] shell		Pointer to shell instance.
 * @param[in] transport_config	Transport configuration during initialization.
 * @param[in] use_colors	Enables colored prints.
 * @param[in] log_backend	If true, the console will be used as logger
 *				backend.
 * @param[in] init_log_level	Default severity level for the logger.
 *
 * @return Standard error code.
 */
int shell_init(const struct shell *shell, const void *transport_config,
	       bool use_colors, bool log_backend, u32_t init_log_level);

/**
 * @brief Uninitializes the transport layer and the internal shell state.
 *
 * @param shell Pointer to shell instance.
 *
 * @return Standard error code.
 */
int shell_uninit(const struct shell *shell);

/**
 * @brief Function for starting shell processing.
 *
 * @param shell Pointer to the shell instance.
 *
 * @return Standard error code.
 */
int shell_start(const struct shell *shell);

/**
 * @brief Function for stopping shell processing.
 *
 * @param shell Pointer to shell instance.
 *
 * @return Standard error code.
 */
int shell_stop(const struct shell *shell);

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
 * @param[in] shell	Pointer to the shell instance.
 * @param[in] color	Printed text color.
 * @param[in] fmt	Format string.
 * @param[in] ...	List of parameters to print.
 */
void shell_fprintf(const struct shell *shell, enum shell_vt100_color color,
		   const char *fmt, ...);

/**
 * @brief Print data in hexadecimal format.
 *
 * @param[in] shell	Pointer to the shell instance.
 * @param[in] data	Pointer to data.
 * @param[in] len	Length of data.
 */
void shell_hexdump(const struct shell *shell, const u8_t *data, size_t len);

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
 * @param[in] shell Pointer to the shell instance.
 */
void shell_process(const struct shell *shell);

/**
 * @brief Change displayed shell prompt.
 *
 * @param[in] shell	Pointer to the shell instance.
 * @param[in] prompt	New shell prompt.
 *
 * @return 0		Success.
 * @return -EINVAL	Pointer to new prompt is not correct.
 */
int shell_prompt_change(const struct shell *shell, const char *prompt);

/**
 * @brief Prints the current command help.
 *
 * Function will print a help string with: the currently entered command
 * and subcommands (if they exist).
 *
 * @param[in] shell      Pointer to the shell instance.
 */
void shell_help(const struct shell *shell);

/* @brief Command's help has been printed */
#define SHELL_CMD_HELP_PRINTED	(1)

/** @brief Execute command.
 *
 * Pass command line to shell to execute.
 *
 * Note: This by no means makes any of the commands a stable interface, so
 * 	 this function should only be used for debugging/diagnostic.
 *
 *	 This function must not be called from shell command context!

 *
 * @param[in] shell	Pointer to the shell instance.
 *			@rst
 *			It can be NULL when
 *			the :option:`CONFIG_SHELL_BACKEND_DUMMY` option is
 *			enabled.
 *			@endrst
 * @param[in] cmd	Command to be executed.
 *
 * @returns		Result of the execution
 */
int shell_execute_cmd(const struct shell *shell, const char *cmd);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H__ */
