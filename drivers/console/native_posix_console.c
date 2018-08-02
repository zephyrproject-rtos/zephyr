/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ctype.h>
#include "init.h"
#include "kernel.h"
#include "console/console.h"
#include "posix_board_if.h"
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

#define DEBUG_ECHO 0

#if (DEBUG_ECHO)
#define ECHO(...) printf(__VA_ARGS__)
#else
#define ECHO(...)
#endif

#if defined(CONFIG_NATIVE_POSIX_STDOUT_CONSOLE)
/**
 *
 * @brief Initialize the driver that provides the printk output
 *
 */
static void native_posix_stdout_init(void)
{
	/* Let's ensure that even if we are redirecting to a file, we get stdout
	 * and stderr line buffered (default for console). Note that glibc
	 * ignores size. But just in case we set a reasonable number in case
	 * somebody tries to compile against a different library
	 */
	setvbuf(stdout, NULL, _IOLBF, 512);
	setvbuf(stderr, NULL, _IOLBF, 512);

	extern void __printk_hook_install(int (*fn)(int));
	__printk_hook_install(putchar);
}

/**
 * Ensure that whatever was written thru printk is displayed now
 */
void posix_flush_stdout(void)
{
	fflush(stdout);
}
#endif /* CONFIG_NATIVE_POSIX_STDOUT_CONSOLE */

#if defined(CONFIG_NATIVE_POSIX_STDIN_CONSOLE)

#define VALID_DIRECTIVES \
	"Valid native console driver directives:\n" \
	"  !wait %%u\n" \
	"  !quit\n"

static struct k_fifo *avail_queue;
static struct k_fifo *lines_queue;
static u8_t (*completion_cb)(char *line, u8_t len);
static bool stdin_is_tty;

static K_THREAD_STACK_DEFINE(stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
static struct k_thread native_stdio_thread;

static inline void found_eof(void)
{
	/*
	 * Once stdin is closed or the input file has ended,
	 * there is no need to try again
	 */
	ECHO("Got EOF\n");
	k_thread_abort(&native_stdio_thread);
}

/*
 * Check if the command is a directive the driver handles on its own
 * and if it is, handle it.
 * If not return 0 (so it can be passed to the shell)
 *
 * Inputs
 *   s       Command string
 *   towait  Pointer to the amount of time wait until attempting to receive
 *           the next command
 *
 * return 0 if it is not a directive
 * return > 0 if it was a directive (command starts with '!')
 * return 2 if the driver directive requires to pause processing input
 */
static int catch_directive(char *s, s32_t *towait)
{
	while (*s != 0  && isspace(*s)) {
		s++;
	}

	if (*s != '!') {
		return 0;
	}

	if (strncmp(s, "!wait", 5) == 0) {
		int ret;

		ret = sscanf(&s[5], "%i", towait);
		if (ret != 1) {
			posix_print_error_and_exit("%s(): '%s' not understood, "
						   "!wait syntax: !wait %%i\n",
						   __func__, s);
		}
		return 2;
	} else if (strcmp(s, "!quit") == 0) {
		posix_exit(0);
	}

	posix_print_warning("%s(): '%s' not understood\n" VALID_DIRECTIVES,
			    __func__, s);

	return 1;
}

/**
 * Check if there is data ready in stdin
 */
static int stdin_not_ready(void)
{
	int ready;
	fd_set readfds;
	struct timeval timeout;

	timeout.tv_usec = 0;
	timeout.tv_sec  = 0;

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	ready = select(STDIN_FILENO+1, &readfds, NULL, NULL, &timeout);

	if (ready == 0) {
		return 1;
	} else if (ready == -1) {
		posix_print_error_and_exit("%s: Error on select ()\n",
					   __func__);
	}

	return 0;
}

/**
 * Check if there is any line in the stdin buffer,
 * if there is and we have available shell buffers feed it to the shell
 *
 * This function returns how long the thread should wait in ms,
 * before checking again the stdin buffer
 */
static s32_t attempt_read_from_stdin(void)
{
	static struct console_input *cmd;
	s32_t towait = CONFIG_NATIVE_STDIN_POLL_PERIOD;

	while (1) {
		char *ret;
		int last;
		int is_directive;

		if (feof(stdin)) {
			found_eof();
		}

		/*
		 * If stdin comes from a terminal, we check if the user has
		 * input something, and if not we pause the process.
		 *
		 * If stdin is not coming from a terminal, but from a file or
		 * pipe, we always proceed to try to get data and block until
		 * we do
		 */
		if (stdin_is_tty && stdin_not_ready()) {
			return towait;
		}

		/* Pick next available shell line buffer */
		if (!cmd) {
			cmd = k_fifo_get(avail_queue, K_NO_WAIT);
			if (!cmd) {
				return towait;
			}
		}

		/*
		 * By default stdin is (_IOLBF) line buffered when connected to
		 * a terminal and fully buffered (_IOFBF) when connected to a
		 * pipe/file.
		 * If we got a terminal: we already checked for it to be ready
		 * and therefore a full line should be there for us.
		 *
		 * If we got a pipe or file we will block until we get a line,
		 * or we reach EOF
		 */
		ret = fgets(cmd->line, CONSOLE_MAX_LINE_LEN, stdin);
		if (ret == NULL) {
			if (feof(stdin)) {
				found_eof();
			}
			/*
			 * Otherwise this was an unexpected error we do
			 * not try to handle
			 */
			return towait;
		}

		/* Remove a possible end of line and other trailing spaces */
		last = (int)strlen(cmd->line) - 1;
		while ((last >= 0) && isspace(cmd->line[last])) {
			cmd->line[last--] = 0;
		}

		ECHO("Got: \"%s\"\n", cmd->line);

		/*
		 * This console has a special set of directives which start with
		 * "!" which we capture here
		 */
		is_directive = catch_directive(cmd->line, &towait);
		if (is_directive == 2) {
			return towait;
		} else if (is_directive > 0) {
			continue;
		}

		/* Let's give it to the shell to handle */
		k_fifo_put(lines_queue, cmd);
		cmd = NULL;
	}

	return towait;
}

/**
 * This thread will check if there is any new line in the stdin buffer
 * every CONFIG_NATIVE_STDIN_POLL_PERIOD ms
 *
 * If there is, it will feed it to the shell
 */
static void native_stdio_runner(void *p1, void *p2, void *p3)
{
	stdin_is_tty = isatty(STDIN_FILENO);

	while (1) {
		s32_t wait_time = attempt_read_from_stdin();

		k_sleep(wait_time);
	}
}

void native_stdin_register_input(struct k_fifo *avail, struct k_fifo *lines,
			 u8_t (*completion)(char *str, u8_t len))
{
	avail_queue = avail;
	lines_queue = lines;
	completion_cb = completion;

	k_thread_create(&native_stdio_thread, stack,
			CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
			native_stdio_runner, NULL, NULL, NULL,
			CONFIG_NATIVE_STDIN_PRIO, 0, K_NO_WAIT);
}
#endif /* CONFIG_NATIVE_POSIX_STDIN_CONSOLE */

static int native_posix_console_init(struct device *arg)
{
	ARG_UNUSED(arg);

#if defined(CONFIG_NATIVE_POSIX_STDOUT_CONSOLE)
	native_posix_stdout_init();
#endif

	return 0;
}

SYS_INIT(native_posix_console_init, PRE_KERNEL_1,
	CONFIG_NATIVE_POSIX_CONSOLE_INIT_PRIORITY);

