/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Generic part of the GDB server
 *
 * This module provides the embedded GDB Remote Serial Protocol for Zephyr.
 *
 * The following is a list of all currently defined GDB RSP commands.
 *
 * `?'
 *     Indicate the reason the target halted.
 *
 * `c [addr]'
 *     Continue. addr is address to resume. If addr is omitted, resume at
 *     current address.
 *
 * `C sig[;addr]'
 *     Continue with signal sig (hex signal number). If `;addr' is omitted,
 *     resume at same address.
 *
 *     _WRS_XXX - Current limitation: Even if this syntax is understood, the
 *     GDB server does not resume the context with the specified signal but
 *     resumes it with the exception vector that caused the context to stop.
 *
 * `D'
 *     Detach GDB from the remote system.
 *
 * `g'
 *     Read general registers.
 *
 * `G XX...'
 *     Write general registers.
 *
 * `k'
 *     Detach GDB from the remote system.
 *
 * `m addr,length'
 *     Read length bytes of memory starting at address addr. Note that addr
 *     may not be aligned to any particular boundary.  The stub need not use
 *     any particular size or alignment when gathering data from memory for
 *     the response; even if addr is word-aligned and length is a multiple of
 *     the word size, the stub is free to use byte accesses, or not. For this
 *     reason, this packet may not be suitable for accessing memory-mapped I/O
 *     devices.
 *
 * `M addr,length:XX...'
 *     Write length bytes of memory starting at address addr. XX... is the data.
 *     Each byte is transmitted as a two-digit hexadecimal number.
 *
 * `p n'
 *     Read the value of register n; n is in hex.
 *
 * `P n...=r...'
 *     Write register n... with value r.... The register number n is in
 *     hexadecimal, and r... contains two hex digits for each byte in the
 *     register (target byte order).
 *
 * `q name params...'
 *     General query. See General Query Packets description
 *
 * `s [addr]'
 *     Single step. addr is the address at which to resume. If addr is omitted,
 *     resume at same address.
 *
 * `S sig[;addr]'
 *     Step with signal. This is analogous to the `C' packet, but requests a
 *     single-step, rather than a normal resumption of execution.
 *
 *     NOTE: Current limitation: Even if this syntax is understood, the GDB
 *     server steps the context without the specified signal (i.e like the
 *     `s [addr]' command).
 *
 * `T thread-id'
 *     Find out if the thread thread-id is alive.
 *
 * `vCont[;action[:thread-id]]...'
 *     Resume the inferior, specifying different actions for each thread. If an
 *     action is specified with no thread-id, then it is applied to any threads
 *     that don't have a specific action specified; if no default action is
 *     specified then other threads should remain stopped. Specifying multiple
 *     default actions is an error; specifying no actions is also an error.
 *
 *     Currently supported actions are:
 *
 *     `c'
 *         Continue.
 *     `C sig'
 *         Continue with signal sig. The signal sig should be two hex digits.
 *     `s'
 *         Step.
 *     `S sig'
 *         Step with signal sig. The signal sig should be two hex digits.
 *
 *     The optional argument addr normally associated with the `c', `C', `s',
 *     and `S' packets is not supported in `vCont'.
 *
 * `X addr,length:XX...'
 *     Write length bytes of memory starting at address addr, where the data
 *     is transmitted in binary. The binary data representation uses 7d (ascii
 *     ‘}’) as an escape character.  Any escaped byte is transmitted as the
 *     escape character followed by the original character XORed with 0x20.
 *
 * `z type,addr,length'
 * `Z type,addr,length'
 *     Insert (`Z') or remove (`z') a type breakpoint starting at addr address
 *     of length length.
 *
 * General Query Packets:
 * `qC'
 *     Return the current thread ID.
 *
 * `qSupported'
 *     Query the GDB server for features it supports. This packet allows
 *     client to take advantage of GDB server's features.
 *
 *     These are the currently defined GDB server features, in more detail:
 *
 *     `PacketSize=bytes'
 *         The GDB server can accept packets up to at least bytes in length.
 *         client will send packets up to this size for bulk transfers, and
 *         will never send larger packets. This is a limit on the data
 *         characters in the packet, including the frame and checksum. There
 *         is no trailing NUL byte in a remote protocol packet;
 *
 *     `qXfer:features:read'
 *         Access the target description. Target description can identify the
 *         architecture of the remote target and (for some architectures)
 *         provide information about custom register sets. They can also
 *         identify the OS ABI of the remote target. Client can use this
 *         information to autoconfigure for your target, or to warn you if you
 *         connect to an unsupported target.
 *
 *         By default, the following simple target description is supported:
 *
 *         <target version="1.0">
 *             <architecture>i386</architecture>
 *         </target>
 *
 *         But architectures may also reports information on specific features
 *         such as extended registers definitions or hardware breakpoint
 *         definitions.
 *
 *         Each `<feature>' describes some logical portion of the target
 *         system.
 *         A `<feature>' element has this form:
 *
 *         <feature name="NAME">
 *             [TYPE...]
 *             REG...
 *         </feature>
 *
 *         Each feature's name should be unique within the description.  The
 *         name of a feature does not matter unless GDB has some special
 *         knowledge of the contents of that feature; if it does, the feature
 *         should have its standard name.
 *
 *         Extended registers definitions are reported following the standard
 *         register format defined by GDB Remote protocol:
 *
 *         Each register is represented as an element with this form:
 *
 *         <reg name="NAME"
 *            bitsize="SIZE"
 *            [regnum="NUM"]
 *            [save-restore="SAVE-RESTORE"]
 *            [type="TYPE"]
 *            [group="GROUP"]/>
 *
 *         The components are as follows:
 *
 *         NAME
 *             The register's name; it must be unique within the target
 *             description.
 *
 *         BITSIZE
 *             The register's size, in bits.
 *
 *         REGNUM
 *             The register's number.  If omitted, a register's number is one
 *             greater than that of the previous register (either in the
 *             current feature or in a preceding feature); the first register
 *             in the target description defaults to zero.  This register
 *             number is used to read or write the register; e.g. it is used
 *             in the remote `p' and `P' packets, and registers appear in the
 *             `g' and `G' packets in order of increasing register number.
 *
 *         SAVE-RESTORE
 *             Whether the register should be preserved across inferior
 *             function calls; this must be either `yes' or `no'.  The default
 *             is `yes', which is appropriate for most registers except for
 *             some system control registers; this is not related to the
 *             target's ABI.
 *
 *         TYPE
 *             The type of the register.  TYPE may be a predefined type, a
 *             type defined in the current feature, or one of the special
 *             types `int' and `float'.  `int' is an integer type of the
 *             correct size for BITSIZE, and `float' is a floating point type
 *             (in the architecture's normal floating point format) of the
 *             correct size for BITSIZE.  The default is `int'.
 *
 *         GROUP
 *             The register group to which this register belongs.  GROUP must
 *             be either `general', `float', or `vector'.  If no GROUP is
 *             specified, GDB will not display the register in `info
 *             registers'.
 *
 *
 *         Hardware breakpoint definitions are reported using the following
 *         format:
 *
 *         <feature name="HW_BP_FEATURE">
 *             <defaults
 *                     max_bp="MAX_BP"
 *                     max_inst_bp="MAX_INST_BP"
 *                 max_watch_bp="MAX_WATCH_BP"
 *                 length="LENGTH"
 *                 >
 *             HW_BP_DESC...
 *         </feature>
 *
 *         The defaults section allows to define some default values and avoid
 *         to list them in each HW_BP_DESC.
 *
 *         Each HW_BP_DESC entry has the form:
 *
 *         <hwbp type="ACCESS_TYPE"
 *               [length="LENGTH"]
 *               [max_bp="MAX_BP"]
 *               />
 *
 *         If HW_BP_DESC defines an item which has a default value defined,
 *         then it overwrite the default value for HW_BP_DESC entry.
 *
 *         Items in [brackets] are optional. The components are as follows:
 *
 *         MAX_BP
 *             Maximum number of hardware breakpoints that can be set.
 *
 *         MAX_INST_BP
 *             Maximum number of instruction hardware breakpoints that can be
 *             set.
 *
 *         MAX_WATCH_BP
 *             Maximum number of data hardware breakpoints that can be set.
 *
 *         LENGTH
 *             Supported access lengths (in hexadecimal without 0x prefix).
 *             Access lengths are encoded as powers of 2 which can be OR'ed.
 *             For example, if an hardware breakpoint type supports 1, 2, 4,
 *             8 bytes access, length will be f (0x1|0x2|0x4|0x8).
 *
 *         ACCESS_TYPE
 *             Hardware breakpoint type:
 *                 inst  : Instruction breakpoint
 *                 watch : Write access breakpoint
 *                 rwatch: Read access breakpoint
 *                 awatch: Read|Write access breakpoint
 *
 *         The GDB server can also reports additional information using the
 *         "WR_AGENT_FEATURE" feature. The purpose of this feature is to report
 *         information about the agent configuration.
 *         The GDB server feature is using the following format:
 *
 *         <feature name="WR_AGENT_FEATURE">
 *             <config max_sw_bp="MAX_SW_BP"
 *                     step_only_on_bp="STEP_ONLY_ON_BP"
 *                     />
 *         </feature>
 *
 *         The components are as follows:
 *
 *         MAX_SW_BP
 *             Maximum number of software breakpoint that can be set.
 *
 *         STEP_ONLY_ON_BP
 *             This parameter is set to 1 if the GDB server is only able to
 *             step the context which hit the breakpoint.
 *             This parameter is set to 0 if the GDB server is able to step
 *             any context.
 *
 *     `QStartNoAckMode'
 *         By default, when either the client or the server sends a packet,
 *         the first response expected is an acknowledgment: either `+' (to
 *         indicate the package was received correctly) or `-' (to request
 *         retransmission). This mechanism allows the GDB remote protocol to
 *         operate over unreliable transport mechanisms, such as a serial
 *         line.
 *
 *         In cases where the transport mechanism is itself reliable (such as
 *         a pipe or TCP connection), the `+'/`-' acknowledgments are
 *         redundant. It may be desirable to disable them in that case to
 *         reduce communication overhead, or for other reasons. This can be
 *         accomplished by means of the `QStartNoAckMode' packet.
 *
 *     `CONFIG_GDB_REMOTE_SERIAL_EXT_NOTIF_PREFIX_STR`
 *         This parameter indicates that the GDB server supports transfer of
 *         Zephyr console I/O to the client using GDB notification packets.
 *
 *         NOTE: Current limitation: For now, the GDB server only supports the
 *         console output.
 *
 * Notification Packets:
 *     Extension of the GDB Remote Serial Protocol uses notification packets
 *     (See `CONFIG_GDB_REMOTE_SERIAL_EXT_NOTIF_PREFIX_STR` support).
 *     Those packets are transferred using the following format:
 *         %<notificationName:<notificationData>#<checksum>
 *
 *     For example:
 *       %CONFIG_GDB_REMOTE_SERIAL_EXT_NOTIF_PREFIX_STR:<notificationData>#<checksum>
*/

#include <nanokernel.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <kernel_structs.h>
#include <board.h>
#include <device.h>
#include <uart.h>
#include <cache.h>
#include <init.h>
#include <debug/gdb_arch.h>
#include <misc/debug/mem_safe.h>
#include <gdb_server.h>
#include <debug_info.h>
#ifdef CONFIG_GDB_SERVER_INTERRUPT_DRIVEN
#include <drivers/console/uart_console.h>
#endif
#ifdef CONFIG_REBOOT
#include <misc/reboot.h>
#endif

#define STUB_OK "OK"
#define STUB_ERROR "ENN"

/* Size of notification data buffers */
#ifndef GDB_NOTIF_DATA_SIZE
#define GDB_NOTIF_DATA_SIZE 100
#endif

/* Overhead size for notification packet encoding. */
#define NOTIF_PACKET_OVERHEAD 6

/* Maximum number of software breakpoints */
#define MAX_SW_BP CONFIG_GDB_SERVER_MAX_SW_BP

#define GDB_INVALID_REG_SET ((void *)-1)

#define fill_output_buffer(x) strncpy((char *)gdb_buffer, x, GDB_BUF_SIZE - 1)

#ifdef CONFIG_GDB_SERVER_BOOTLOADER
#define STR_TYPE ";type=zephyr_boot"
#else
#define STR_TYPE ";type=zephyr"
#endif

#ifdef GDB_ARCH_HAS_RUNCONTROL
#define	RESUME_SYSTEM() resume_system()
#define	REMOVE_ALL_INSTALLED_BREAKPOINTS() \
			remove_all_installed_breakpoints()
#define UNINSTALL_BREAKPOINTS() uninstall_breakpoints()
#else
#define	RESUME_SYSTEM()
#define	REMOVE_ALL_INSTALLED_BREAKPOINTS()
#define UNINSTALL_BREAKPOINTS()
#endif

#ifdef GDB_ARCH_HAS_RUNCONTROL
struct bp_array {
	gdb_instr_t *addr;	/* breakpoint address */
	gdb_instr_t instr;	/* saved instruction */
	char valid;		/* breakpoint is valid? */
	char enabled;		/* breakpoint is enabled? */
};
#endif

static const unsigned char hex_chars[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static int client_is_connected;
static int in_no_ack_mode;
static int valid_registers;
static volatile int event_is_pending;
static volatile int cpu_stop_signal = GDB_SIG_NULL;
static volatile int cpu_pending_signal;
static struct gdb_reg_set gdb_regs;

static const char *xml_target_header = "<?xml version=\"1.0\"?> "
	"<!DOCTYPE target SYSTEM "
	"\"gdb-target.dtd\"> <target version=\"1.0\">\n";
static const char *xml_target_footer = "</target>";
static unsigned char gdb_buffer[GDB_BUF_SIZE];

static unsigned char tmp_reg_buffer[GDB_NUM_REG_BYTES];

#ifdef GDB_ARCH_HAS_RUNCONTROL

#ifdef GDB_ARCH_HAS_HW_BP
static int hw_bp_cnt;
static struct gdb_debug_regs dbg_regs;
#endif
static int trace_lock_key;

/*
 * GDB breakpoint table. Note that all the valid entries in the breakpoint
 * table are kept contiguous. When parsing the table, the first invalid entry
 * in the table marks the end of the table.
 */
static struct bp_array bp_array[MAX_SW_BP];

#ifdef GDB_ARCH_NO_SINGLE_STEP
static gdb_instr_t *gdb_step_emu_next_pc;
static gdb_instr_t gdb_step_emu_instr;
#endif

#endif

#ifdef GDB_ARCH_HAS_REMOTE_SERIAL_EXT_USING_NOTIF_PACKETS
static volatile int notif_pkt_pending;
static volatile int notif_data_idx;
static unsigned char notif_data[GDB_NOTIF_DATA_SIZE];
#endif

#ifdef CONFIG_GDB_SERVER_INTERRUPT_DRIVEN
static struct nano_fifo avail_queue;
static struct nano_fifo cmds_queue;
#endif

static struct device *uart_console_dev;

/* global definitions */

volatile int gdb_debug_status = NOT_DEBUGGING;

#ifdef GDB_ARCH_HAS_RUNCONTROL
#ifdef GDB_ARCH_HAS_HW_BP
volatile int cpu_stop_bp_type = GDB_SOFT_BP;
long cpu_stop_hw_bp_addr;
#endif
#endif

static int put_packet(unsigned char *buffer);
static void put_debug_char(unsigned char ch);
static unsigned char get_debug_char(void);

static void post_event(void);
static void control_loop(void);
static void handle_system_stop(NANO_ISF *reg, int sig);

#ifdef GDB_ARCH_HAS_RUNCONTROL

#define ADD_DEL_BP_SIG(x) \
	int(x)(enum gdb_bp_type type, long addr, int len, \
		enum gdb_error_code *err)

typedef ADD_DEL_BP_SIG(add_del_bp_t);
static ADD_DEL_BP_SIG(add_bp);
static ADD_DEL_BP_SIG(delete_bp);

static void resume_system(void);
static int set_instruction(void *addr, gdb_instr_t *instruction, size_t size);
static void remove_all_installed_breakpoints(void);
#endif

#ifdef GDB_ARCH_HAS_REMOTE_SERIAL_EXT_USING_NOTIF_PACKETS
static void handle_notification(void);
static void request_notification_packet_flush(void);
static uint32_t write_to_console(char *buf, uint32_t len);
#endif

#ifdef CONFIG_GDB_SERVER_INTERRUPT_DRIVEN
static int console_irq_input_hook(uint8_t ch);
#endif

static int get_hex_char_value(unsigned char ch)
{
	if ((ch >= 'a') && (ch <= 'f')) {
		return ch - 'a' + 10;
	}
	if ((ch >= '0') && (ch <= '9')) {
		return ch - '0';
	}
	if ((ch >= 'A') && (ch <= 'F')) {
		return ch - 'A' + 10;
	}
	return -1;
}

/**
 * @brief Consume a hex number from a string and convert to long long number
 *
 * Consume the string up to the end of the hex number, i.e. the pointer to the
 * string is advanced and output that number via the @a value parameter.
 *
 * Does not handle negative numbers.
 *
 * @return The number of characters consumed from the string.
 */
static int hex_str_to_longlong(unsigned char **hex_str, long long *value)
{
	int num_chars_consumed = 0;
	int hex_value;

	*value = 0;
	while (**hex_str) {
		hex_value = get_hex_char_value(**hex_str);
		if (hex_value < 0) {
			break;
		}
		*value = (*value << 4) | hex_value;
		num_chars_consumed++;
		(*hex_str)++;
	}
	return num_chars_consumed;
}

/*
 * Similar to hex_str_to_longlong, but outputs an int and handles negative
 * numbers.
 */
static int hex_str_to_int(unsigned char **ptr, int *value)
{
	int num_chars_consumed = 0;
	int hex_value;
	int neg = 0;

	*value = 0;

	if (**ptr == '-') {
		neg = 1;
		(*ptr)++;
		num_chars_consumed++;
	}

	while (**ptr) {
		hex_value = get_hex_char_value(**ptr);
		if (hex_value < 0) {
			break;
		}
		*value = (*value << 4) | hex_value;
		num_chars_consumed++;
		(*ptr)++;
	}

	if (neg) {
		if (num_chars_consumed == 1) {
			(*ptr)--;
			num_chars_consumed = 0;
		} else {
			*value = -(*value);
		}
	}
	return num_chars_consumed;
}

/*
 * Consume two hex characters from a string and return the corresponding
 * value.
*/
static int hex_str_to_byte(unsigned char **str)
{
	unsigned char *ptr = *str;
	int byte;

	byte = get_hex_char_value(*ptr++);
	byte = (byte << 4) + get_hex_char_value(*ptr++);
	if (byte >= 0) {
		*str = ptr;
	}
	return byte;
}

/*
 * Turn a one-byte value into two hex characters and write them to the buffer.
 * Return the next position in the buffer.
 */

static unsigned char *put_hex_byte(unsigned char *buf, int value)
{
	*buf++ = hex_chars[value >> 4];
	*buf++ = hex_chars[value & 0xf];
	return buf;
}

static inline int is_size_encoder(int num)
{
	/*
	 * It is not possible to use the '$', '#' and '%' characters to encode
	 * the size per GDB remote protocol specification.
	 */
	return (num + 29) != '$' && (num + 29) != '#' && (num + 29) != '%';
}

/*
 * Compress memory pointed to by buffer into its ascii hex value, the
 * character '*' and the number of times it is reapeated, placing result in
 * the same buffer. Return a pointer to the last char put in buf (null).
 */

static unsigned char *compress(unsigned char *buf)
{
	unsigned char *read_ptr = buf;
	unsigned char *write_ptr = buf;
	unsigned char ch;
	size_t count = strlen((char *)buf);
	int max_repeat = 126 - 29;
	size_t ix;

	for (ix = 0; ix < count; ix++) {
		int num = 0;
		int jx;

		ch = *read_ptr++;
		*write_ptr++ = ch;
		for (jx = 1; ((jx + ix) < count) && (jx < max_repeat); jx++) {
			if (read_ptr[jx - 1] == ch) {
				num++;
			} else {
				break;
			}
		}
		if (num >= 3) {
			/*
			 * Skip characters that cannot be used as size
			 * encoders.
			 */
			while (!is_size_encoder(num)) {
				num--;
			}

			*write_ptr++ = '*';
			*write_ptr++ = (unsigned char)(num + 29);
			read_ptr += num;
			ix += num;
		}
	}
	*write_ptr = 0;
	return write_ptr;
}

/**
 * @brief encode memory data using hexadecimal value of chars from '0' to 'f'
 *
 * For example, 0x3 (CTRL+C) will be encoded with hexadecimal values of
 * '0' (0x30) and '3' (0x33): 0x3033.
 *
 * Use mem2hex() to encode a buffer avoid to send control chars that could
 * perturbate communication protocol.
 *
 * @param data to encode
 * @param output buffer
 * @param size of data to encode
 * @param Compress output data ?
 *
 * @return Pointer to the last char put in buf (null).
 */

static unsigned char *mem2hex(unsigned char *mem, unsigned char *buf,
				int count, int do_compress)
{
	int i;
	unsigned char ch;
	unsigned char *saved_buf = buf;

	for (i = 0; i < count; i++) {
		ch = *mem++;
		buf = put_hex_byte(buf, ch);
	}
	*buf = 0;

	if (do_compress) {
		return compress(saved_buf);
	}

	return buf;
}

static inline int do_mem_probe(char *addr, int mode, int width,
				 int preserve, long *dummy)
{
	char *p = (char *)dummy;

	if (preserve) {
		if (_mem_probe(addr, SYS_MEM_SAFE_READ, width, p) < 0) {
			return -1;
		}
	}
	if (_mem_probe(addr, mode, width, p) < 0) {
		return -1;
	}

	return 0;
}

/**
* @brief Probe if memory location is valid
*
* @param addr Address to test
* @param mode Mode of access (SYS_MEM_SAFE_READ/WRITE)
* @param size Number of bytes to test
* @param width Width of memory access (1, 2, or 4)
* @param preserve Preserve memory on write test ?
*
* @return 0 if memory is accessible, -1 otherwise.
*/

static int mem_probe(char *addr, int mode, int size,
			int width, int preserve)
{
	long dummy;

	/* if memory length is zero, test is done */
	if (size == 0) {
		return 0;
	}

	/* Validate parameters */

	preserve = mode == SYS_MEM_SAFE_READ ? 0 : preserve;

	if (width == 0) {
		width = 1;
	} else {
		if ((width != 1) && (width != 2) && (width != 4)) {
			return -1;
		}
	}

	/* Check addr, size & width parameters coherency */

	if (((unsigned long)addr % width) || (size % width)) {
		return -1;
	}

	/* Check first address */

	if (do_mem_probe(addr, mode, width, preserve, &dummy) < 0) {
		return -1;
	}

	/* Check if we have tested the whole memory */

	if (width == size) {
		return 0;
	}

	/* Check last address */

	addr = addr + size - width;
	if (do_mem_probe(addr, mode, width, preserve, &dummy) < 0) {
		return -1;
	}

	return 0;
}

static int put_packet(unsigned char *buffer)
{
	unsigned char checksum = 0;
	int count = 0;
	unsigned char ch;

	/*  $<packet info>#<checksum>. */
	do {
		put_debug_char('$');
		checksum = 0;
		count = 0;

		/* Buffer terminated with null character */

		while ((ch = buffer[count])) {
			put_debug_char(ch);
			checksum = (unsigned char)(checksum + ch);
			count += 1;
		}

		put_debug_char('#');
		put_debug_char(hex_chars[(checksum >> 4)]);
		put_debug_char(hex_chars[(checksum & 0xf)]);

		if (!in_no_ack_mode) {
			/* Wait for ack */

			ch = get_debug_char();
			if (ch == '+') {
				return 0;
			}
			if (ch == '$') {
				put_debug_char('-');
				return 0;
			}
			if (ch == GDB_STOP_CHAR) {
				cpu_stop_signal = GDB_SIG_INT;
				gdb_debug_status = DEBUGGING;
				post_event();
				return 0;
			}
		} else {
			return 0;
		}
	} while (1);
}

#ifdef GDB_ARCH_HAS_REMOTE_SERIAL_EXT_USING_NOTIF_PACKETS
/*
 * Request a flush of pending notification packets. This is done by setting
 * notif_pkt_pending to 1 before stopping the CPU. Once stopped, the
 * control loop will send pending packets before resuming the system.
 */

static void request_notification_packet_flush(void)
{
	/*
	 * Before stopping CPU we must indicate that we're stopping the system
	 * to handle a packet notification. During the packet notification, we
	 * should prevent CPU from reading protocol...
	 */

	if (gdb_debug_status != NOT_DEBUGGING) {
		return;
	}
	notif_pkt_pending = 1;
	gdb_debug_status = DEBUGGING;
	control_loop();
	gdb_debug_status = NOT_DEBUGGING;
	RESUME_SYSTEM();
}

/*
 * If the notification buffer for current CPU is full, or if we found a new
 * line or carriage return character, then we must flush received data to
 * remote client.
 */
static inline int must_flush_notification_buffer(unsigned char ch)
{
	return (notif_data_idx == GDB_NOTIF_DATA_SIZE) ||
		    (ch == '\n') || (ch == '\r');
}

/*
 * Write data to debug agent console. For performance reason, the data is
 * bufferized until we receive a carriage return character or until the buffer
 * gets full.
 *
 * The buffer is also automatically flushed when system is stopped.
 */
static uint32_t write_to_console(char *buf, uint32_t len)
{
	uint32_t ix;
	unsigned char ch;

	int key = irq_lock();

	/* Copy data to notification buffer for current CPU */
	for (ix = 0; ix < len; ix++) {
		ch = buf[ix];
		notif_data[notif_data_idx++] = ch;

		if (must_flush_notification_buffer(ch)) {
			notif_data[notif_data_idx] = '\0';
			request_notification_packet_flush();
		}
	}

	irq_unlock(key);

	return len;
}

/*
 * Handle pending notification packets for the CPU. It is invoked while
 * running in GDB CPU control loop (When system is stopped).
 */
static void handle_notification(void)
{
	const char *name = CONFIG_GDB_REMOTE_SERIAL_EXT_NOTIF_PREFIX_STR;
	unsigned char checksum = 0;
	int ix = 0;
	unsigned char ch;
	int more_data = 0;
	uint32_t max_packet_size;
	uint32_t data_size;
	unsigned char *ptr = notif_data;

	/* First, check if there is pending data */

	if (notif_data[0] == '\0') {
		return;
	}

again:
	/*
	 * Build notification packet.
	 *
	 * A notification packet has the form `%<data>#<checksum>', where data
	 * is the content of the notification, and checksum is a checksum of
	 * data, computed and formatted as for ordinary gdb packets. A
	 * notification's data never contains `$', `%' or `#' characters. Upon
	 * receiving a notification, the recipient sends no `+' or `-' to
	 * acknowledge the notification's receipt or to report its corruption.
	 *
	 * Every notification's data begins with a name, which contains no
	 * colon characters, followed by a colon character.
	 */
	put_debug_char('%');
	checksum = 0;

	/* Add name to notification packet */
	ix = 0;
	while ((ch = name[ix++])) {
		put_debug_char(ch);
		checksum += ch;
	}

	/* Name must be followed by a colon character. */
	put_debug_char(':');
	checksum += ':';

	/*
	 * Add data to notification packet.
	 * Warning: The value to hex encoding double the size of the data,
	 * so we must not encode more than the remaining GDB buffer size
	 * divided by 2.
	 */
	max_packet_size =
		GDB_BUF_SIZE - (strlen(name) + NOTIF_PACKET_OVERHEAD);

	data_size = strlen((char *)ptr);
	if (data_size <= (max_packet_size / 2)) {
		more_data = 0;
	} else {
		data_size = max_packet_size / 2;
		more_data = 1;	/* Not enough room in notif packet */
	}

	/* Encode data using hex values */
	for (ix = 0; ix < data_size; ix++) {
		ch = hex_chars[(*ptr >> 4)];
		put_debug_char(ch);
		checksum += ch;
		ch = hex_chars[(*ptr & 0xf)];
		put_debug_char(ch);
		checksum += ch;
		ptr++;
	}

	/* Terminate packet with #<checksum> */
	put_debug_char('#');
	put_debug_char(hex_chars[(checksum >> 4)]);
	put_debug_char(hex_chars[(checksum & 0xf)]);

	if (more_data) {
		goto again;
	}

	/* Clear buffer & index */
	notif_data[0] = '\0';
	notif_data_idx = 0;
}
#endif

#ifdef GDB_ARCH_HAS_HW_BP
static int has_hit_a_hw_bp(void)
{
	/* instruction hw breakpoints are reported as sw breakpoints */
	return (cpu_stop_signal == GDB_SIG_TRAP) &&
		(cpu_stop_bp_type != GDB_SOFT_BP) &&
		(cpu_stop_bp_type != GDB_HW_INST_BP);
}
#endif

static void do_post_event_hw_bp(unsigned char **buf, size_t *buf_size)
{
#ifdef GDB_ARCH_HAS_HW_BP
	/*
	 * If it's an hardware breakpoint, report the address and access
	 * type at the origin of the HW breakpoint. Supported syntaxes:
	 *        watch:<dataAddr> : Write access
	 *        rwatch:<dataAddr> : Read access
	 *        awatch:<dataAddr> : Read/Write Access
	 * Instruction hardware breakpoints are reported as software
	 * breakpoints
	 */

	if (!has_hit_a_hw_bp()) {
		return;
	}

	int count = 0;

	switch (cpu_stop_bp_type) {
	case GDB_HW_DATA_WRITE_BP:
		count = snprintf((char *)*buf, *buf_size, ";watch");
		break;

	case GDB_HW_DATA_READ_BP:
		count = snprintf((char *)*buf, *buf_size, ";rwatch");
		break;

	case GDB_HW_DATA_ACCESS_BP:
		count = snprintf((char *)*buf, *buf_size, ";awatch");
		break;
	}
	if (count != 0) {
		*buf += count;
		*buf_size -= count;
		count = snprintf((char *)*buf, *buf_size, ":%lx",
					cpu_stop_hw_bp_addr);
		*buf += count;
		*buf_size -= count;
	}
	cpu_stop_hw_bp_addr = 0;
	cpu_stop_bp_type = GDB_SOFT_BP;
	cpu_stop_signal = GDB_SIG_NULL;
#endif
}

static void write_regs_to_buffer(unsigned char **buf, size_t *buf_size)
{
	unsigned char *saved_buf;
	int count;

#ifdef GDB_ARCH_HAS_ALL_REGS
	count = snprintf((char *)*buf, *buf_size, ";regs:");
	*buf += count;
	*buf_size -= count;
	saved_buf = *buf;
	*buf = mem2hex(tmp_reg_buffer, *buf, sizeof(gdb_regs), 1);
	*buf_size -= (*buf - saved_buf);
#else
	int offset = 0;
	int size = 4;

	count = snprintf((char *)*buf, *buf_size, ";%x:", GDB_PC_REG);
	*buf += count;
	*buf_size -= count;
	gdb_arch_reg_info_get(GDB_PC_REG, &size, &offset);
	saved_buf = *buf;
	*buf = mem2hex(tmp_reg_buffer + offset, *buf, size, 1);
	*buf_size -= (*buf - saved_buf);
#endif
}

static void do_post_event(void)
{
	unsigned char *buf = gdb_buffer;
	size_t buf_size = GDB_BUF_SIZE;
	int count;

	if (buf != gdb_buffer) {
		*buf++ = '|';
		buf_size--;
	}
	count = snprintf((char *)buf, buf_size, "T%02xthread:%02x",
				cpu_stop_signal, 1);
	buf += count;
	buf_size -= count;

	do_post_event_hw_bp(&buf, &buf_size);

	if (valid_registers) {
		gdb_arch_regs_get(&gdb_regs, (char *)tmp_reg_buffer);
		write_regs_to_buffer(&buf, &buf_size);
	}

	/* clear stop reason */
	cpu_stop_signal = GDB_SIG_NULL;

	*buf = '\0';
}

static void post_event(void)
{
	event_is_pending = 0;

	if (cpu_stop_signal != GDB_SIG_NULL) {
		do_post_event();
	} else {
		(void)snprintf((char *)gdb_buffer, GDB_BUF_SIZE, "S%02x",
			       GDB_SIG_INT);
	}

	(void)put_packet(gdb_buffer);
}

/**
 * @brief Get a character from serial line
 *
 * It loops until it has received a character or until it has detected that a
 * GDB event is pending and should be handled.
 *
 * Note that this routine should only be called from the gdb control loop when
 * the system is stopped.
 *
 * @return -1 if no character has been received and there is a GDB event
 * pending or debug operation pending, number of received character otherwise.
 */

static int get_debug_char_raw(void)
{
	char ch;

	while (uart_poll_in(uart_console_dev, &ch) != 0) {
		if (event_is_pending) {
			return -1;
		}
	}
	return ch;
}

static unsigned char get_debug_char(void)
{
	return (unsigned char)get_debug_char_raw();
}

/**
 * @brief Get a GDB serial packet
 *
 * Poll the serial line to get a full GDB serial packet. Once
 * the packet is received, it computes its checksum and return acknowledgment.
 * It then returns the packet to the caller.
 *
 * This routine must only be called when all CPUs are stopped (from the GDB
 * CPU control loop).
 *
 * If a pending GDB event is detected or if a stop event is received from the
 * client, the corresponding GDB stop event is sent to the client. This
 * loop does also handle the GDB cpu loop hooks by the intermediate of
 * get_debug_char() API.
 *
 * If a debug operation is pending, this routine returns immediately.
 *
 * @return Pointer to received packet or NULL on pending debug operation
 */

static unsigned char *get_packet(unsigned char *buffer, size_t size)
{
	unsigned char checksum, c, *p;

	while (1) {
		while ((c = get_debug_char()) != '$') {
			if (!event_is_pending) {
				return NULL;
			}

			/* ignore other chars than GDB break character */
			if ((c == GDB_STOP_CHAR) || event_is_pending) {
				post_event();
			}
		}

		checksum = 0;
		p = buffer;

		/*
		 * Continue reading characters until a '#' is found or until
		 * the end of the buffer is reached.
		 */

		while (p < &buffer[size]) {
			c = get_debug_char();

			if (c == '#') {
				break;
			} else if (c == '$') {
				/* start over */
				checksum = 0;
				p = buffer;
				continue;
			} else {
				checksum += c;
				*p++ = c;
			}
		}

		*p = 0;

		if (c == '#') {
			if (in_no_ack_mode) {
				(void)get_debug_char();
				(void)get_debug_char();
				return buffer;
			}

			unsigned char cs[2];

			cs[0] = get_hex_char_value(get_debug_char()) << 4;
			cs[1] = get_hex_char_value(get_debug_char());

			if (checksum != (cs[0]|cs[1])) {
				/* checksum failed */
				put_debug_char('-');
			} else {
				/* checksum passed */
				put_debug_char('+');

				if (buffer[2] == ':') {
					put_debug_char(buffer[0]);
					put_debug_char(buffer[1]);
					return &buffer[3];
				}
				return buffer;
			}
		}
	}

	return NULL;
}

/**
 * @brief write a XML string into output buffer
 *
 * It takes care of offset, length and also deal with overflow (if the XML
 * string is bigger than the output buffer).
 */

static void write_xml_string(char *buf, const char *xml_str, int off, int len)
{
	size_t max_len = strlen(xml_str);

	if (off == max_len) {
		strncat((char *)buf, "l", len - 1);
	} else if (off > max_len) {
		fill_output_buffer("E00");
	} else {
		if ((off + max_len) <= len) {
			/* we can read the full data */
			buf[0] = 'l';
			int size_to_copy = len <= (GDB_BUF_SIZE - 2) ? len :
						GDB_BUF_SIZE - 2;
			strncpy(&buf[1], xml_str + off, size_to_copy);
		} else {
			buf[0] = 'm';
			strncpy(&buf[1], xml_str + off, GDB_BUF_SIZE - 2);
			buf[len + 1] = '\0';
		}
	}
}

/**
* @brief get XML target description
*
* This routine is used to build the string that will hold the XML target
* description provided to the GDB client.
*
* NOTE: Non-re-entrant, since it uses a static buffer.
*
* @return a pointer on XML target description
*/

static char *get_xml_target_description(void)
{
	static char target_description[GDB_BUF_SIZE] = { 0 };
	char *ptr = target_description;
	size_t buf_size = sizeof(target_description);
	size_t size;

	if (target_description[0] != 0) {
		return target_description;
	}

	strncpy(ptr, xml_target_header, GDB_BUF_SIZE - 1);
	size = strlen(ptr);
	ptr += size;
	buf_size -= size;

	/* Add architecture definition */

	(void)snprintf(ptr, buf_size, "  <architecture>%s</architecture>\n",
		       GDB_TGT_ARCH);
	size = strlen(ptr);
	ptr += size;
	buf_size -= size;

	strncpy(ptr, xml_target_footer,
		GDB_BUF_SIZE - (ptr - target_description) - 1);

	return target_description;
}

/* utility functions for handling each case of protocal_parse() */

static void handle_new_connection(void)
{
	/*
	 * This is a new connection. Clear in_no_ack_mode field if it was set
	 * and send acknowledgment for this command that has not been sent as
	 * it should have.
	 */
	if (in_no_ack_mode) {
		put_debug_char('+');
		in_no_ack_mode = 0;
	}

	snprintf((char *)gdb_buffer, GDB_BUF_SIZE, "T02thread:%02x;", 1);

	/*
	 * This is an initial connection, should remove all
	 * the breakpoints and cleanup.
	 */
	REMOVE_ALL_INSTALLED_BREAKPOINTS();
	client_is_connected = 1;
}

static void reboot(void)
{
#ifdef CONFIG_REBOOT
	sys_reboot(SYS_REBOOT_COLD);
	fill_output_buffer(STUB_OK);
#endif
}

static void detach(void)
{
	fill_output_buffer(STUB_OK);
	REMOVE_ALL_INSTALLED_BREAKPOINTS();
	client_is_connected = 0;
	gdb_debug_status = NOT_DEBUGGING;
	RESUME_SYSTEM();
	in_no_ack_mode = 0;
}

static unsigned char *handle_thread_query(unsigned char *packet)
{
	int thread;

	if (!hex_str_to_int(&packet, &thread)) {
		gdb_buffer[0] = '\0';
		return packet;
	}
	if (thread != 1) {
		fill_output_buffer(STUB_ERROR);
	} else {
		fill_output_buffer(STUB_OK);
	}

	return packet;
}

#ifdef CONFIG_REBOOT
#define STR_REBOOT ";reboot+"
static size_t concat_reboot_feature_if_supported(size_t size)
{
	strncat((char *)gdb_buffer, STR_REBOOT, size);
	return sizeof(STR_REBOOT);
}
#else
#define concat_reboot_feature_if_supported(size) (0)
#endif

static ALWAYS_INLINE int is_valid_xml_query(unsigned char **packet,
						int *off, int *len)
{
	unsigned char *p = *packet;
	int is_valid = hex_str_to_int(&p, off) && *p++ == ','
			&& hex_str_to_int(&p, len) && *p == '\0';
	*packet = p;
	return is_valid;
}

static unsigned char *handle_xml_query(unsigned char *packet)
{
	int off, len;

	packet += 11;
	if (is_valid_xml_query(&packet, &off, &len)) {
		char *xml = get_xml_target_description();

		write_xml_string((char *)gdb_buffer, xml, off, len);
	} else {
		fill_output_buffer(STUB_ERROR);
	}

	return packet;
}

static const char *supported_features_cmd =
	"PacketSize=%x;qXfer:features:read+;QStartNoAckMode+"
#ifdef GDB_ARCH_HAS_REMOTE_SERIAL_EXT_USING_NOTIF_PACKETS
	";" CONFIG_GDB_REMOTE_SERIAL_EXT_NOTIF_PREFIX_STR "+"
#endif
	;

static unsigned char *handle_general_query(unsigned char *packet)
{
	if (packet[0] == 'C') {
		snprintf((char *)gdb_buffer, GDB_BUF_SIZE, "QC%x", 1);
	} else if (strncmp((char *)packet, "wr.", 3) == 0) {
		packet += 3;
		gdb_buffer[0] = '\0';
	} else if (strcmp((char *)packet, "Supported") == 0) {
		size_t size = GDB_BUF_SIZE;

		snprintf((char *)gdb_buffer, size, supported_features_cmd,
				GDB_BUF_SIZE);
		size -= (strlen((char *)gdb_buffer) + 1);

		size -= concat_reboot_feature_if_supported(size);

		strncat((char *)gdb_buffer, STR_TYPE, size);
		size -= sizeof(STR_TYPE);
	} else if (strncmp((char *)packet, "Xfer:features:read:", 19) == 0) {
		packet += 19;
		if (strncmp((char *)packet, "target.xml:", 11) == 0) {
			packet = handle_xml_query(packet);
		} else {
			gdb_buffer[0] = '\0';
		}
	} else {
		gdb_buffer[0] = '\0';
	}

	return packet;
}

static ALWAYS_INLINE void handle_get_registers(void)
{
	if (!valid_registers) {
		fill_output_buffer("E02");
		return;
	}
	(void)gdb_arch_regs_get(&gdb_regs, (char *)tmp_reg_buffer);
	mem2hex(tmp_reg_buffer, gdb_buffer, GDB_NUM_REG_BYTES, 1);
}

static ALWAYS_INLINE
unsigned char *handle_write_registers(unsigned char *packet)
{
	if (!valid_registers) {
		fill_output_buffer("E02");
		return packet;
	}

	(void)gdb_arch_regs_get(&gdb_regs, (char *)tmp_reg_buffer);

	for (int i = 0; i < GDB_NUM_REG_BYTES; i++) {
		int value = hex_str_to_byte(&packet);

		if (value < 0) {
			break;
		}
		tmp_reg_buffer[i] = (unsigned char)value;
	}

	gdb_arch_regs_set(&gdb_regs, (char *)tmp_reg_buffer);

	fill_output_buffer(STUB_OK);
	return packet;
}

#ifdef GDB_HAS_SINGLE_REG_ACCESS
static ALWAYS_INLINE
unsigned char *handle_write_single_register(unsigned char *packet)
{
	int reg_num = 0;
	int offset = 0;
	int size = 4;
	int i, value;

	if (!valid_registers) {
		fill_output_buffer("E02");
		return packet;
	}
	if (!hex_str_to_int(&packet, &reg_num) || *(packet++) != '=') {
		fill_output_buffer("E02");
		return packet;
	}

	gdb_arch_regs_get(&gdb_regs, (char *)tmp_reg_buffer);
	gdb_arch_reg_info_get(reg_num, &size, &offset);

	for (i = 0; i < size; i++) {
		value = hex_str_to_byte(&packet);
		if (value < 0) {
			break;
		}
		tmp_reg_buffer[offset + i] = (unsigned char)value;
	}
	if (i != size) {
		fill_output_buffer(STUB_ERROR);
		return packet;
	}
	gdb_arch_regs_set(&gdb_regs, (char *)tmp_reg_buffer);
	fill_output_buffer(STUB_OK);
	return packet;
}

static ALWAYS_INLINE
unsigned char *handle_read_single_register(unsigned char *packet)
{
	int reg_num = 0;
	int offset = 0;
	int size = 4;

	if (!valid_registers) {
		fill_output_buffer("E02");
		return packet;
	}
	/* p<regno> */

	if (!hex_str_to_int(&packet, &reg_num)) {
		fill_output_buffer("E02");
		return packet;
	}

	gdb_arch_regs_get(&gdb_regs, (char *)tmp_reg_buffer);
	gdb_arch_reg_info_get(reg_num, &size, &offset);
	mem2hex(tmp_reg_buffer + offset, gdb_buffer, size, 1);
	return packet;
}
#endif

static ALWAYS_INLINE
unsigned char *handle_read_memory(unsigned char *packet)
{
	/* m<addr>,<length> */

	long long addr;
	int length;
	void *p;

	if (hex_str_to_longlong((unsigned char **)&packet, &addr) == 0) {
		fill_output_buffer("E01");
		return packet;
	}

	if (!(*packet++ == ',' && hex_str_to_int(&packet, &length))) {
		fill_output_buffer("E01");
		return packet;
	}

	p = (void *)((long)addr);
	if (mem_probe(p, SYS_MEM_SAFE_READ, length, 0, 1) == -1) {
		/* No read access */
		fill_output_buffer("E01");
		return packet;
	}

	/* Now read memory */
	mem2hex(p, gdb_buffer, length, 1);
	return packet;
}

#define WRITE_MEM_SIG(x) \
	unsigned char *(x)(unsigned char *packet, unsigned char *dest, int len)
typedef WRITE_MEM_SIG(write_mem_t);

static ALWAYS_INLINE unsigned char *handle_write_memory(unsigned char *packet,
							write_mem_t *write_mem)
{
	long long addr;
	int len;
	unsigned char *p;

	/* [X or P]<addr>,<length>:<val><val>...<val> */

	if (hex_str_to_longlong(&packet, &addr) == 0) {
		fill_output_buffer("E02");
		return packet;
	}

	p = packet; /* to allow the if expression to fit on one line */
	if (!(*p++ == ',' && hex_str_to_int(&p, &len) && *p++ == ':')) {
		fill_output_buffer("E02");
		return p;
	}
	packet = p;

	p = (void *)((long)addr);
	if (mem_probe(p, SYS_MEM_SAFE_WRITE, len, 0, 1) == -1) {
		/* No write access */
		fill_output_buffer("E02");
		return packet;
	}

	packet = write_mem(packet, p, len);

	fill_output_buffer(STUB_OK);
	return packet;
}


static WRITE_MEM_SIG(write_memory)
{
	unsigned char value;
	int i;

	for (i = 0; i < len; i++) {
		value = hex_str_to_byte(&packet);
		if (value < 0) {
			break;
		}
		dest[i] = (unsigned char)value;
	}

	return packet;
}

static WRITE_MEM_SIG(write_memory_from_binary_format)
{
	unsigned char value;
	int i;

	for (i = 0; i < len; i++) {
		value = packet[0];
		packet++;
		if (value == '}') {
			value = packet[0] ^ 0x20;
			packet++;
		}
		dest[i] = (unsigned char)value;
	}

	return packet;
}

static ALWAYS_INLINE
unsigned char *handle_pass_signal_to_context(unsigned char *packet)
{
	int signal;

	/* read signal number */
	if (!hex_str_to_int(&packet, &signal)) {
		fill_output_buffer("E02");
		return packet;
	}

	cpu_pending_signal = signal;

	if (*packet == ';') {
		packet++;
	}

	return packet;
}

static ALWAYS_INLINE
unsigned char *handle_continue_execution(unsigned char *packet)
{
	long long addr;

	/* try to read optional parameter, PC unchanged if no param */
	hex_str_to_longlong(&packet, &addr);
	gdb_debug_status = NOT_DEBUGGING;

	return packet;
}

static ALWAYS_INLINE unsigned char *handle_step(unsigned char *packet)
{
	long long addr;

	/* try to read optional parameter, PC unchanged if no param */
	hex_str_to_longlong(&packet, &addr);

	gdb_debug_status = SINGLE_STEP;

	return packet;
}

static ALWAYS_INLINE
unsigned char *handle_vcont_action(unsigned char *packet, int *do_not_send_ack)
{
	char action;
	int signal = 0, thread;

	packet += 5;
	action = *packet++;
	if ((action != 'c') && (action != 'C') &&
	    (action != 's') && (action != 'S')) {
		gdb_buffer[0] = '\0';
		return packet;
	}

	if ((action == 'C') || (action == 'S')) {
		/* read signal number */
		if (!hex_str_to_int(&packet, &signal)) {
			fill_output_buffer("E02");
			return packet;
		}
	}

	if (*packet == ':') {
		packet++;
		hex_str_to_int(&packet, &thread);
	}

	if (signal != 0) {
		cpu_pending_signal = signal;
	}

	if ((action == 'c') || (action == 'C')) {
		gdb_debug_status = NOT_DEBUGGING;
	} else {
		gdb_debug_status = SINGLE_STEP;
	}

	*do_not_send_ack = 1;
	return packet;
}

#ifdef GDB_ARCH_HAS_RUNCONTROL
static ALWAYS_INLINE
unsigned char *handle_breakpoint_install(unsigned char *packet,
					 add_del_bp_t *bp_op)
{
	/* remove (ztype,addr,length) or insert (Ztype,addr,length) */

	int type, len;
	long long addr;
	enum gdb_error_code err;

	/* read <type> & <addr> */
	if (!(hex_str_to_int(&packet, &type) && *packet++ == ',' &&
	      hex_str_to_longlong(&packet, &addr))) {
		fill_output_buffer("E07");
		return packet;
	}

	/* read length */
	if (!(*packet++ == ',' && hex_str_to_int(&packet, &len))) {
		fill_output_buffer("E07");
		return packet;
	}

	if (bp_op(type, (long)addr, len, &err) == 0) {
		fill_output_buffer(STUB_OK);
	} else {
		snprintf((char *)gdb_buffer, GDB_BUF_SIZE, "E%02d", err);
	}

	return packet;
}
#endif

/**
* @brief parse given GDB command string
*
* Parse and execute the given GDB command string, and send acknowledgment if
* acknowledgment is enabled.
*
* @return 0 on success, -1 if failed to send acknowledgment.
*/

static int protocol_parse(unsigned char *packet)
{
	unsigned char ch;
	int do_not_send_ack = 0;

	ch = *packet++;

	switch (ch) {
	case '?':
		handle_new_connection();
		break;
	case 'k':
		/* Kill request: we use it to reboot */
		reboot();
		break;
	case 'D':
		detach();
		break;
	case 'T':
		packet = handle_thread_query(packet);
		break;
	case 'Q':
		/* the only 'Q' command we support is "start no-ack mode" */
		if (strcmp((const char *)packet, "StartNoAckMode") == 0) {
			in_no_ack_mode = 1;
			fill_output_buffer(STUB_OK);
		} else {
			gdb_buffer[0] = '\0';
		}
		break;
	case 'q':
		packet = handle_general_query(packet);
		break;
	case 'g':
		handle_get_registers();
		break;
	case 'G':
		packet = handle_write_registers(packet);
		break;

#ifdef GDB_HAS_SINGLE_REG_ACCESS
	case 'P':
		packet = handle_write_single_register(packet);
		break;
	case 'p':
		packet = handle_read_single_register(packet);
		break;
#endif

	case 'm':
		packet = handle_read_memory(packet);
		break;
	case 'M':
		packet = handle_write_memory(packet, write_memory);
		break;
	case 'X':
		packet = handle_write_memory(packet,
					write_memory_from_binary_format);
		break;

	case 'C':
		packet = handle_pass_signal_to_context(packet);
		/* fall through */
	case 'c':
		packet = handle_continue_execution(packet);
		do_not_send_ack = 1;
		break;

	case 'S':
		packet = handle_pass_signal_to_context(packet);
		/* fall through */
	case 's':
		packet = handle_step(packet);
		do_not_send_ack = 1;
		break;
	case 'v':
		if (strcmp((const char *)packet, "Cont?") == 0) {
			fill_output_buffer("vCont;c;s;C;S");
			break;
		} else if (strncmp((const char *)packet, "Cont;", 5) != 0) {
			gdb_buffer[0] = '\0';
			break;
		}

		packet = handle_vcont_action(packet, &do_not_send_ack);
		break;

#ifdef GDB_ARCH_HAS_RUNCONTROL
	case 'z':
		packet = handle_breakpoint_install(packet, delete_bp);
		break;
	case 'Z':
		packet = handle_breakpoint_install(packet, add_bp);
		break;
#endif
	default:
		/* in case of an unsupported command, send empty response */
		gdb_buffer[0] = '\0';
		break;
	}

	/* Send the acknowledgment command when necessary */

	if (!do_not_send_ack) {
		if (put_packet(gdb_buffer) < 0) {
			return -1;
		}
	}

	return 0;
}

/*
 * function: put_debug_char
 * description:
 *      - "What you must do for the stub"
 *      - Write a single character from a port.
 */
static void put_debug_char(unsigned char ch)
{
	(void)uart_poll_out(uart_console_dev, ch);
}

#ifdef GDB_ARCH_HAS_RUNCONTROL

/**
 * @brief add an hardware breakpoint to debug registers set
 *
 * This routine adds an hardware breakpoint to debug registers structure.
 * It does not update the debug registers.
 *
 * @param addr Address where to set the breakpoint
 * @param type Type of breakpoint
 * @param len Length of data
 * @param err Container for returning error code
 *
 * @return 0 on success, -1 if failed (Error code returned via @a err).
 */

static int add_hw_bp(long addr, enum gdb_bp_type type, int len,
			enum gdb_error_code *err)
{
#ifdef GDB_ARCH_HAS_HW_BP
	if (gdb_hw_bp_set(&dbg_regs, addr, type, len, err) == -1) {
		return -1;
	}

	hw_bp_cnt++;
	return 0;
#else
	*err = GDB_ERROR_HW_BP_NOT_SUP;
	return -1;
#endif
}

/**
 * @brief remove an hardware breakpoint from debug registers set
 *
 * This routine removes an hardware breakpoint from debug registers structure.
 * It does not update the debug registers.
 *
 * @param addr Address where to set the breakpoint
 * @param type Type of breakpoint
 * @param len Length of data
 * @param err Container for returning error code
 *
 * @return 0 on success, -1 if failed (Error code returned via @a err).
*/

static int remove_hw_bp(long addr, enum gdb_bp_type type, int len,
			enum gdb_error_code *err)
{
#ifdef GDB_ARCH_HAS_HW_BP
	if (gdb_hw_bp_clear(&dbg_regs, addr, type, len, err) == -1) {
		return -1;
	}

	hw_bp_cnt--;
	return 0;
#else
	*err = GDB_ERROR_HW_BP_NOT_SUP;
	return -1;
#endif
}

/**
 * @brief add a new breakpoint or watchpoint to breakpoint list
 *
 * This routine adds a new breakpoint or watchpoint to breakpoint list. For
 * watchpoints, this routine checks that the given type/length combination is
 * supported on current architecture, and that debug registers are not full.
 *
 * @param type GDB breakpoint type:
 *
 *        0 : software breakpoint    (GDB_SOFT_BP)
 *        1 : hardware breakpoint    (GDB_HW_INST_BP)
 *        2 : write watchpoint       (GDB_HW_DATA_WRITE_BP)
 *        3 : read watchpoint        (GDB_HW_DATA_READ_BP)
 *        4 : access watchpoint      (GDB_HW_DATA_ACCESS_BP)
 *
 * @param addr Breakpoint address
 * @param len For a software breakpoint, len specifies the size of the
 *            instruction to be patched. For hardware breakpoints and
 *            watchpoints length specifies the memory region to be monitored.
 * @param err Pointer to error code if failed to add breakpoint.
 *
 * @return 0 on success, -1 if failed to add breakpoint.
 */

static int add_bp(enum gdb_bp_type type, long addr, int len,
			enum gdb_error_code *err)
{
	if (type != GDB_SOFT_BP) {
		return add_hw_bp(addr, type, len, err);
	}

	if (mem_probe((void *)addr, SYS_MEM_SAFE_READ, len, 0, 1) == -1) {
		*err = GDB_ERROR_INVALID_MEM;
		return -1;
	}

	/* Add software breakpoint to BP list */

	for (int ix = 0; ix < MAX_SW_BP; ix++) {
		if (bp_array[ix].valid == 0) {
			bp_array[ix].valid = 1;
			bp_array[ix].enabled = 0;
			bp_array[ix].addr = (gdb_instr_t *)addr;
			return 0;
		}
	}

	*err = GDB_ERROR_BP_LIST_FULL;
	return -1;
}

/**
* @brief delete a breakpoint or watchpoint from breakpoint list
*
* @return 0 on success, -1 if failed to remove breakpoint.
*/

static int delete_bp(enum gdb_bp_type type, long addr, int len,
			enum gdb_error_code *err)
{
	gdb_instr_t *bp_addr = (gdb_instr_t *)addr;

	if (type != GDB_SOFT_BP) {
		return remove_hw_bp(addr, type, len, err);
	}

	for (int ix = 0; ix < MAX_SW_BP; ix++) {
		if (bp_array[ix].valid && bp_array[ix].addr == bp_addr) {

			bp_array[ix].valid = 0;

			/*
			 * Make sure all valid entries are contiguous to speed
			 * up breakpoint table parsing.
			 */

			for (int jx = ix + 1; jx < MAX_SW_BP; jx++) {
				if (bp_array[jx].valid == 1) {
					bp_array[jx - 1] = bp_array[jx];
					bp_array[jx].valid = 0;
				} else {
					break;
				}
			}

			return 0;

		} else if (!bp_array[ix].valid) {
			break;
		}
	}

	*err = GDB_ERROR_INVALID_BP;
	return -1;
}

static void remove_all_installed_breakpoints(void)
{
	for (int ix = 0; ix < MAX_SW_BP; ix++) {
		if (!bp_array[ix].valid) {
			break;
		}
		bp_array[ix].valid = 0;
	}
}

#ifdef GDB_ARCH_HAS_HW_BP
static inline void set_debug_regs_for_hw_breakpoints(void)
{
	if (hw_bp_cnt > 0) {
		gdb_dbg_regs_set(&dbg_regs);
	}
}
#else
#define set_debug_regs_for_hw_breakpoints() do { } while ((0))
#endif

/*
 * Physically install breakpoints, and make sure that modified memory is
 * flushed on all CPUs.
 *
 * Must only be called when ready to exit the CPU control loop.
 */

static void install_breakpoints(void)
{
	gdb_instr_t instr = GDB_BREAK_INSTRUCTION;

	/* Software breakpoints installation */

	for (int ix = 0; ix < MAX_SW_BP; ix++) {
		if (bp_array[ix].valid && !bp_array[ix].enabled) {
			gdb_instr_t *addr = bp_array[ix].addr;

			bp_array[ix].instr = *addr;
			(void)set_instruction(addr, &instr, sizeof(instr));
			bp_array[ix].enabled = 1;
		} else if (!bp_array[ix].valid) {
			break;
		}
	}

	set_debug_regs_for_hw_breakpoints();
}


#ifdef GDB_ARCH_HAS_HW_BP
static inline void clear_debug_regs_for_hw_breakpoints(void)
{
	if (hw_bp_cnt > 0) {
		gdb_dbg_regs_clear();
	}
}
#else
#define clear_debug_regs_for_hw_breakpoints() do { } while ((0))
#endif

/*
* Physically uninstall breakpoints, and make sure that modified memory is
* flushed on all CPUs.
*
* Must only be called in the CPU control loop.
*/

static void uninstall_breakpoints(void)
{
	for (int ix = 0; ix < MAX_SW_BP; ix++) {
		if (bp_array[ix].valid == 1 && bp_array[ix].enabled) {
			gdb_instr_t *addr = bp_array[ix].addr;
			(void)set_instruction(addr, &bp_array[ix].instr,
					      sizeof(gdb_instr_t));
			bp_array[ix].enabled = 0;
		} else if (bp_array[ix].valid == 0) {
			break;
		}
	}

	clear_debug_regs_for_hw_breakpoints();
}

/* Re-install breakpoints and resume the system. */

static void resume_system(void)
{
	/*
	 * System must not be resumed if we're going to execute a single step.
	 */

	if (gdb_debug_status == SINGLE_STEP) {
		return;
	}

	install_breakpoints();
}

static inline void enter_trace_mode(void)
{
#ifdef GDB_ARCH_NO_SINGLE_STEP
	gdb_instr_t bp_instr = GDB_BREAK_INSTRUCTION;

	gdb_step_emu_next_pc = gdb_get_next_pc(&gdb_regs);
	gdb_step_emu_instr = *gdb_step_emu_next_pc;
	(void)set_instruction(gdb_step_emu_next_pc, &bp_instr,
			      sizeof(gdb_instr_t));
	trace_lock_key = gdb_int_regs_lock(&gdb_regs);
#else
	/* Handle single step request for runcontrol CPU */

	trace_lock_key = gdb_trace_mode_set(&gdb_regs);
#endif
}
static inline void disable_trace_mode(void)
{
#ifdef GDB_ARCH_NO_SINGLE_STEP
	/* remove temporary breakpoint */
	(void)set_instruction(gdb_step_emu_next_pc,
				&gdb_step_emu_instr,
				sizeof(gdb_instr_t));
	/* Disable trace mode */
	gdb_int_regs_unlock(&gdb_regs, trace_lock_key);
#else
	/* Disable trace mode */
	gdb_trace_mode_clear(&gdb_regs, trace_lock_key);
#endif
}

/**
* @brief stop mode agent BP/trace handler
*
* Common handler of breakpoint and trace mode exceptions.
* It is invoked with interrupts locked.
*
* @return n/a
*/

void gdb_handler(enum gdb_exc_mode mode, void *esf, int signal)
{
	/* Save BP/Trace handler registers */
	gdb_arch_regs_from_esf(&gdb_regs, (NANO_ESF *)esf);
	valid_registers = 1;

	if (mode == GDB_EXC_TRACE) {
		/* Check if GDB did request a step */
		if (gdb_debug_status != SINGLE_STEP) {
			return;
		}

		/* No longer pending trace mode exception */
		gdb_debug_status = DEBUGGING;

		disable_trace_mode();
	}

	event_is_pending = 1;
	cpu_stop_signal = signal;

	/* Enter stop mode agent control loop */
	control_loop();

	/* Restore BP handler registers */
	gdb_arch_regs_to_esf(&gdb_regs, (NANO_ESF *)esf);

	/* Resume system if not handling a single step */
	RESUME_SYSTEM();
}

static int set_instruction(void *addr, gdb_instr_t *instr, size_t size)
{

	if (_mem_safe_write_to_text_section(addr, (char *)instr, size) < 0) {
		return -EFAULT;
	}
	sys_cache_flush((vaddr_t) addr, size);
	return 0;
}

#endif /* GDB_ARCH_HAS_RUNCONTROL */

static inline void setup_singlestep_if_non_steppable_instruction(void)
{
#ifdef GDB_ARCH_CAN_STEP
	if (gdb_debug_status == SINGLE_STEP) {
		if (!GDB_ARCH_CAN_STEP(&gdb_regs)) {
			gdb_debug_status = DEBUGGING;
			event_is_pending = 1;
			cpu_stop_signal = GDB_SIG_TRAP;
		}
	}
#endif
}

static inline int handle_single_stepping(void)
{
#ifdef GDB_ARCH_HAS_RUNCONTROL
	setup_singlestep_if_non_steppable_instruction();

	if (gdb_debug_status == SINGLE_STEP) {
		enter_trace_mode();
		return 1;
	}
#endif
	return 0;
}

/**
* @brief GDB control loop
*
* The CPU control loop is an active wait loop used to stop CPU activity.
*
* It must be called with interrupts locked.
*
* It loops while waiting for debug events which can be:
*
* - System resumed: gdb_debug_status != NOT_DEBUGGING
*       The control loop must be exited.
*
* - Single step request: gdb_debug_status == SINGLE_STEP
*       Notify client that CPU is already stopped.
*       This is done by setting event_is_pending = 1.
*       event_is_pending will be handled by next get_packet().
*
* @return n/a
*/

static void control_loop(void)
{
	char ch;

	UNINSTALL_BREAKPOINTS();

	/* Flush input buffer */
	while (uart_poll_in(uart_console_dev, &ch) == 0) {
		if (ch == GDB_STOP_CHAR) {
			gdb_debug_status = DEBUGGING;
			cpu_stop_signal = GDB_SIG_INT;
			event_is_pending = 1;
			break;
		}
	}

	while (gdb_debug_status != NOT_DEBUGGING) {
#ifdef GDB_ARCH_HAS_REMOTE_SERIAL_EXT_USING_NOTIF_PACKETS
		/*
		 * Check if system has been stopped to handle a notification
		 * packet: If a notification is pending (notif_pkt_pending),
		 * but no stop signal has been set.
		 */
		if ((cpu_stop_signal == GDB_SIG_NULL) && notif_pkt_pending) {
			handle_notification();
			/* Mark packet notification as done */
			notif_pkt_pending = 0;
			break;
		}
#endif

		unsigned char *packet = get_packet(gdb_buffer, GDB_BUF_SIZE);

		if (packet) {
			protocol_parse(packet);
		}

		if (handle_single_stepping()) {
			return;
		}
	}
}

/**
* @brief handle a system stop request
*
* The purpose of this routine is to handle a stop request issued by remote
* debug client. It is called when receiving a break char.
*
* It indicates that a GDB event is pending (the answer to stop request) and
* transfer control from the runtime system to the stop mode agent. The event
* will be posted by this control loop.
*
* @return n/a
*/

static void handle_system_stop(NANO_ISF *regs, int signal)
{
	int key = irq_lock();

	gdb_debug_status = DEBUGGING;
	if (signal != 0) {
		cpu_stop_signal = signal;
	} else {
		cpu_stop_signal = GDB_SIG_INT;	/* Stopped by a command */
	}

	/* Save registers */
	if (regs == GDB_INVALID_REG_SET) {
		valid_registers = 0;
	} else {
		if (!regs) {
			regs = sys_debug_current_isf_get();
		}
		gdb_arch_regs_from_isf(&gdb_regs, regs);
		valid_registers = 1;
	}

	/* A GDB event is pending */
	event_is_pending = 1;

	/* Transfer control to the control loop */
	control_loop();

	/* Load registers */
	if (valid_registers) {
		gdb_arch_regs_to_isf(&gdb_regs, regs);
	}

	/* Resume system if not a single step request */
	RESUME_SYSTEM();

	irq_unlock(key);
}

/**
* @brief wrapper to send a character to console
*
* This routine is a specific wrapper to send a character to console.
* If the GDB Server is started, this routine intercepts the data and transfer
* it to the connected debug clients using a GDB notification packet.
*
* @return n/a
*/

static UART_CONSOLE_OUT_DEBUG_HOOK_SIG(gdb_console_out)
{
#ifdef GDB_ARCH_HAS_REMOTE_SERIAL_EXT_USING_NOTIF_PACKETS
	/*
	 * If remote debug client is connected, then transfer data to remote
	 * client. Otherwise, discard this character.
	 */
	if (client_is_connected) {
		write_to_console(&c, 1);
		return UART_CONSOLE_DEBUG_HOOK_HANDLED;
	}
#endif
	return !UART_CONSOLE_DEBUG_HOOK_HANDLED;
}

#ifdef CONFIG_GDB_SERVER_INTERRUPT_DRIVEN
static int console_irq_input_hook(uint8_t ch)
{
	if (ch == GDB_STOP_CHAR) {
		(void)irq_lock();

		handle_system_stop(NULL, 0);
		return 1;
	}
	return 0;
}
#endif

void system_stop_here(void *regs)
{
	int key = irq_lock();

	handle_system_stop((NANO_ISF *) regs, GDB_SIG_STOP);

	irq_unlock(key);
}

void _debug_fatal_hook(const NANO_ESF *esf)
{
	struct gdb_reg_set regs;

	gdb_arch_regs_from_esf(&regs, (NANO_ESF *) esf);
	system_stop_here((void *)&regs);
	gdb_arch_regs_to_esf(&regs, (NANO_ESF *) esf);
}

#ifdef CONFIG_GDB_SERVER_INTERRUPT_DRIVEN
static void init_interrupt_handling(void)
{
	nano_fifo_init(&cmds_queue);
	nano_fifo_init(&avail_queue);
	uart_console_in_debug_hook_install(console_irq_input_hook);
	uart_register_input(&avail_queue, &cmds_queue, NULL);
}
#else
#define init_interrupt_handling() do { } while ((0))
#endif

#ifdef CONFIG_MEM_SAFE_NUM_EXTRA_REGIONS
static void init_mem_safe_access(void)
{
	(void)_mem_safe_region_add((void *)CONFIG_GDB_RAM_ADDRESS,
				   CONFIG_GDB_RAM_SIZE, SYS_MEM_SAFE_READ);
	(void)_mem_safe_region_add((void *)CONFIG_GDB_RAM_ADDRESS,
				   CONFIG_GDB_RAM_SIZE, SYS_MEM_SAFE_WRITE);
}
#else
#define init_mem_safe_access() do { } while ((0))
#endif

static int init_gdb_server(struct device *unused)
{
	static int gdb_is_initialized;

	if (gdb_is_initialized) {
		return -1;
	}

	gdb_arch_init();

	uart_console_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

	uart_console_out_debug_hook_install(gdb_console_out);

	init_interrupt_handling();
	init_mem_safe_access();

	gdb_is_initialized = 1;
	system_stop_here(GDB_INVALID_REG_SET);

	return 0;
}

SYS_INIT(init_gdb_server, NANOKERNEL, 1);
