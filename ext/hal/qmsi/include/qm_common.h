/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_COMMON_H__
#define __QM_COMMON_H__

#if (UNIT_TEST)
#define __volatile__(x)
#define __asm__
#endif /* UNIT_TEST */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#define QM_R volatile const
#define QM_W volatile
#define QM_RW volatile

/* __attribute__((interrupt)) API requires that the interrupt handlers
 * take an interrupt_frame parameter, but it is still undefined, so add
 * an empty definition.
 */
struct interrupt_frame;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define REG_VAL(addr) (*((volatile uint32_t *)addr))

/* QM_ASSERT is not currently available for Zephyr. */
#define ASSERT_EXCLUDE (ZEPHYR_OS)

/**
 * In our reference implementation, by default DEBUG enables QM_PUTS and
 * QM_ASSERT but not QM_PRINTF.
 * User can modify this block to customise the default DEBUG configuration.
 */

#if (DEBUG)
#if !ASSERT_EXCLUDE
#ifndef ASSERT_ENABLE
#define ASSERT_ENABLE (1)
#endif
#endif /* ASSERT_EXCLUDE */
#ifndef PUTS_ENABLE
#define PUTS_ENABLE (1)
#endif
#endif /* DEBUG */

/**
 * Decide how to handle user input validation. Possible options upon discovering
 * an invalid input parameter are:
 *  - Return an error code.
 *  - Assert.
 *  - Assert and save the error into the General Purpose Sticky Register 0.
 *  - Do nothing, only recommended when code size is a major concern.
 */
#if (DEBUG)
#ifndef QM_CHECK_RETURN_ERROR
#define QM_CHECK_RETURN_ERROR (1)
#endif

#ifndef QM_CHECK_ASSERT
#define QM_CHECK_ASSERT (0)
#endif

#ifndef QM_CHECK_ASSERT_SAVE_ERROR
#define QM_CHECK_ASSERT_SAVE_ERROR (0)
#endif

#endif /* DEBUG */

/*
 * Setup UART for stdout/stderr.
 */
void stdout_uart_setup(uint32_t baud_divisors);

/*
 * Default UART baudrate divisors at the sysclk speed set by Boot ROM
 */
#define BOOTROM_UART_115200 (QM_UART_CFG_BAUD_DL_PACK(0, 17, 6))

/**
 * Selectively enable printf/puts/assert.
 * User can modify the defines to divert the calls to custom implementations.
 */

#if (PRINTF_ENABLE || PUTS_ENABLE || ASSERT_ENABLE)
#include <stdio.h>
#endif /* PRINTF_ENABLE || PUTS_ENABLE || ASSERT_ENABLE */

#if (PRINTF_ENABLE)
int pico_printf(const char *format, ...);
#define QM_PRINTF(...) pico_printf(__VA_ARGS__)
#else
#define QM_PRINTF(...)
#endif /* PRINTF_ENABLE */

#if (PUTS_ENABLE)
#define QM_PUTS(x) puts(x)
#else
#define QM_PUTS(x)
#endif /* PUTS_ENABLE */

#if (ASSERT_ENABLE)
#include <assert.h>
#define QM_ASSERT(x) assert(x)
#else
#define QM_ASSERT(x)
#endif /* ASSERT_ENABLE */

/**
 * Select which UART to use for stdout/err (default: UART0)
 */
#if (STDOUT_UART_0 && STDOUT_UART_1)
#error "STDOUT_UART_0 and STDOUT_UART_1 are mutually exclusive"
#elif(!(STDOUT_UART_0 || STDOUT_UART_1))
#define STDOUT_UART_0 (1)
#endif

#if (STDOUT_UART_0)
#define STDOUT_UART (QM_UART_0)
#elif(STDOUT_UART_1)
#define STDOUT_UART (QM_UART_1)
#endif

/*
 * Stdout UART initialization is enabled by default. Use this switch if you wish
 * to disable it (e.g. if the UART is already initialized by an application
 * running on the other core).
 */
#ifndef STDOUT_UART_INIT_DISABLE
#define STDOUT_UART_INIT (1)
#endif

/**
 * Select assert action (default: put the IA core into HLT state)
 */
#if (ASSERT_ACTION_HALT && ASSERT_ACTION_RESET)
#error "ASSERT_ACTION_HALT and ASSERT_ACTION_RESET are mutually exclusive"
#elif(!(ASSERT_ACTION_HALT || ASSERT_ACTION_RESET))
#define ASSERT_ACTION_HALT (1)
#endif

/**
 * Select check action
 */
#if (1 < QM_CHECK_RETURN_ERROR + QM_CHECK_ASSERT + QM_CHECK_ASSERT_SAVE_ERROR)
#error "Only a single check action may be performed"
#endif

#if (QM_CHECK_RETURN_ERROR)
#define QM_CHECK(cond, error)                                                  \
	do {                                                                   \
		if (!(cond)) {                                                 \
			return (error);                                        \
		}                                                              \
	} while (0)

#elif(QM_CHECK_ASSERT)
#define QM_CHECK(cond, error) QM_ASSERT(cond)

#elif(QM_CHECK_ASSERT_SAVE_ERROR)

#define SAVE_LAST_ERROR(x)                                                     \
	do {                                                                   \
		QM_SCSS_GP->gps[0] = x;                                        \
	} while (0)

#define QM_CHECK(cond, error)                                                  \
	do {                                                                   \
		SAVE_LAST_ERROR(error);                                        \
		QM_ASSERT(cond);                                               \
	} while (0)

#else
/* Do nothing with checks */
#define QM_CHECK(cond, error)
#endif

/* Bitwise operation helpers */
#ifndef BIT
#define BIT(x) (1U << (x))
#endif

/* Set all bits */
#ifndef SET_ALL_BITS
#define SET_ALL_BITS (-1)
#endif

/*
 * ISR declaration.
 *
 * The x86 'interrupt' attribute requires an interrupt_frame parameter.
 * To keep consistency between different cores and compiler capabilities, we add
 * the interrupt_frame parameter to all ISR handlers. When not needed, the value
 * passed is a dummy one (NULL).
 */
#if (UNIT_TEST)
#define QM_ISR_DECLARE(handler)                                                \
	void handler(__attribute__(                                            \
	    (unused)) struct interrupt_frame *__interrupt_frame__)
#else /* !UNIT_TEST */
#if (QM_SENSOR) && !(ISR_HANDLED)
/*
 * Sensor Subsystem 'interrupt' attribute.
 */
#define QM_ISR_DECLARE(handler)                                                \
	__attribute__((interrupt("ilink"))) void handler(__attribute__(        \
	    (unused)) struct interrupt_frame *__interrupt_frame__)
#elif(ISR_HANDLED)
/*
 * Allow users to define their own ISR management. This includes optimisations
 * and clearing EOI registers.
 */
#define QM_ISR_DECLARE(handler) void handler(__attribute__((unused)) void *data)

#elif(__iamcu__)
/*
 * Lakemont with compiler supporting 'interrupt' attribute.
 * We assume that if the compiler supports the IAMCU ABI it also supports the
 * 'interrupt' attribute.
 */
#define QM_ISR_DECLARE(handler)                                                \
	__attribute__((interrupt)) void handler(__attribute__(                 \
	    (unused)) struct interrupt_frame *__interrupt_frame__)
#else
/*
 * Lakemont with compiler not supporting the 'interrupt' attribute.
 */
#define QM_ISR_DECLARE(handler)                                                \
	void handler(__attribute__(                                            \
	    (unused)) struct interrupt_frame *__interrupt_frame__)
#endif
#endif /* UNIT_TEST */

/**
 * Helper to convert a macro parameter into its literal text.
 */
#define QM_STRINGIFY(s) #s

/**
 * Convert the source version numbers into an ASCII string.
 */
#define QM_VER_STRINGIFY(major, minor, patch)                                  \
	QM_STRINGIFY(major) "." QM_STRINGIFY(minor) "." QM_STRINGIFY(patch)

#if (SOC_WATCH_ENABLE) && (!QM_SENSOR)
/**
 * Front-end macro for logging a SoC Watch event.  When SOC_WATCH_ENABLE
 * is not set to 1, the macro expands to nothing, there is no overhead.
 *
 * @param[in] event_id The Event ID of the profile event.
 * @param[in] ev_data  A parameter to the event ID (if the event needs one).
 *
 * @returns Nothing.
 */
#define SOC_WATCH_LOG_EVENT(event, param)                                      \
	do {                                                                   \
		soc_watch_log_event(event, param);                             \
	} while (0)

/**
 * Front-end macro for logging application events via the power profiler
 * logger.  When SOC_WATCH_ENABLE is not set to 1, the macro expands to
 * nothing, there is no overhead.
 *
 * @param[in] event_id    The Event ID of the profile event.
 * @param[in] ev_subtype  A 1-byte user-defined event_id.
 * @param[in] ev_data     A parameter to the event ID (if the event needs one).
 *
 * @returns Nothing.
 */
#define SOC_WATCH_LOG_APP_EVENT(event, subtype, param)                         \
	do {                                                                   \
		soc_watch_log_app_event(event, subtype, param);                \
	} while (0)
#else
#define SOC_WATCH_LOG_EVENT(event, param)
#define SOC_WATCH_LOG_APP_EVENT(event, subtype, param)
#endif

#endif /* __QM_COMMON_H__ */
