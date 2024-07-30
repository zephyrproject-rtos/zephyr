/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Based on the ARM semihosting API from:
 * https://github.com/ARM-software/abi-aa/blob/main/semihosting/semihosting.rst
 *
 * RISC-V semihosting also follows these conventions:
 * https://github.com/riscv/riscv-semihosting-spec/blob/main/riscv-semihosting-spec.adoc
 */

/**
 * @file
 *
 * @brief public Semihosting APIs based on ARM definitions.
 * @defgroup semihost Semihosting APIs
 * @ingroup os_services
 * @{
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_SEMIHOST_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_SEMIHOST_H_

/** @brief Semihosting instructions */
enum semihost_instr {
	/*
	 * File I/O operations
	 */

	/** Open a file or stream on the host system. */
	SEMIHOST_OPEN   = 0x01,
	/** Check whether a file is associated with a stream/terminal */
	SEMIHOST_ISTTY  = 0x09,
	/** Write to a file or stream. */
	SEMIHOST_WRITE  = 0x05,
	/** Read from a file at the current cursor position. */
	SEMIHOST_READ   = 0x06,
	/** Closes a file on the host which has been opened by SEMIHOST_OPEN. */
	SEMIHOST_CLOSE  = 0x02,
	/** Get the length of a file. */
	SEMIHOST_FLEN   = 0x0C,
	/** Set the file cursor to a given position in a file. */
	SEMIHOST_SEEK   = 0x0A,
	/** Get a temporary absolute file path to create a temporary file. */
	SEMIHOST_TMPNAM = 0x0D,
	/** Remove a file on the host system. Possibly insecure! */
	SEMIHOST_REMOVE = 0x0E,
	/** Rename a file on the host system. Possibly insecure! */
	SEMIHOST_RENAME = 0x0F,

	/*
	 * Terminal I/O operations
	 */

	/** Write one character to the debug terminal. */
	SEMIHOST_WRITEC         = 0x03,
	/** Write a NULL terminated string to the debug terminal. */
	SEMIHOST_WRITE0         = 0x04,
	/** Read one character from the debug terminal. */
	SEMIHOST_READC          = 0x07,

	/*
	 * Time operations
	 */
	SEMIHOST_CLOCK          = 0x10,
	SEMIHOST_ELAPSED        = 0x30,
	SEMIHOST_TICKFREQ       = 0x31,
	SEMIHOST_TIME           = 0x11,

	/*
	 * System/Misc. operations
	 */

	/** Retrieve the errno variable from semihosting operations. */
	SEMIHOST_ERRNO          = 0x13,
	/** Get commandline parameters for the application to run with */
	SEMIHOST_GET_CMDLINE    = 0x15,
	SEMIHOST_HEAPINFO       = 0x16,
	SEMIHOST_ISERROR        = 0x08,
	SEMIHOST_SYSTEM         = 0x12
};

/**
 * @brief Modes to open a file with
 *
 * Behaviour corresponds to equivalent fopen strings.
 * i.e. SEMIHOST_OPEN_RB_PLUS == "rb+"
 */
enum semihost_open_mode {
	SEMIHOST_OPEN_R         = 0,
	SEMIHOST_OPEN_RB        = 1,
	SEMIHOST_OPEN_R_PLUS    = 2,
	SEMIHOST_OPEN_RB_PLUS   = 3,
	SEMIHOST_OPEN_W         = 4,
	SEMIHOST_OPEN_WB        = 5,
	SEMIHOST_OPEN_W_PLUS    = 6,
	SEMIHOST_OPEN_WB_PLUS   = 7,
	SEMIHOST_OPEN_A         = 8,
	SEMIHOST_OPEN_AB        = 9,
	SEMIHOST_OPEN_A_PLUS    = 10,
	SEMIHOST_OPEN_AB_PLUS   = 11,
};

/**
 * @brief Manually execute a semihosting instruction
 *
 * @param instr instruction code to run
 * @param args instruction specific arguments
 *
 * @return integer return code of instruction
 */
long semihost_exec(enum semihost_instr instr, void *args);

/**
 * @brief Read a byte from the console
 *
 * @return char byte read from the console.
 */
char semihost_poll_in(void);

/**
 * @brief Write a byte to the console
 *
 * @param c byte to write to console
 */
void semihost_poll_out(char c);

/**
 * @brief Open a file on the host system
 *
 * @param path file path to open. Can be absolute or relative to current
 *             directory of the running process.
 * @param mode value from @ref semihost_open_mode.
 *
 * @retval handle positive handle on success.
 * @retval -1 on failure.
 */
long semihost_open(const char *path, long mode);

/**
 * @brief Close a file
 *
 * @param fd handle returned by @ref semihost_open.
 *
 * @retval 0 on success.
 * @retval -1 on failure.
 */
long semihost_close(long fd);

/**
 * @brief Query the size of a file
 *
 * @param fd handle returned by @ref semihost_open.
 *
 * @retval positive file size on success.
 * @retval -1 on failure.
 */
long semihost_flen(long fd);

/**
 * @brief Seeks to an absolute position in a file.
 *
 * @param fd handle returned by @ref semihost_open.
 * @param offset offset from the start of the file in bytes.
 *
 * @retval 0 on success.
 * @retval -errno negative error code on failure.
 */
long semihost_seek(long fd, long offset);

/**
 * @brief Read the contents of a file into a buffer.
 *
 * @param fd handle returned by @ref semihost_open.
 * @param buf buffer to read data into.
 * @param len number of bytes to read.
 *
 * @retval read number of bytes read on success.
 * @retval -errno negative error code on failure.
 */
long semihost_read(long fd, void *buf, long len);

/**
 * @brief Write the contents of a buffer into a file.
 *
 * @param fd handle returned by @ref semihost_open.
 * @param buf buffer to write data from.
 * @param len number of bytes to write.
 *
 * @retval 0 on success.
 * @retval -errno negative error code on failure.
 */
long semihost_write(long fd, const void *buf, long len);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_SEMIHOST_H_ */
