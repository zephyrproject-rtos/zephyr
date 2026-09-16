/*
 * Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V Supervisor Binary Interface (SBI) definitions.
 *
 * Defines SBI extension IDs, function IDs, and error codes used by
 * S-mode code to request M-mode firmware services via the @c ecall
 * instruction.  The subset defined here covers only the extensions
 * used by Zephyr's in-tree minimal SBI runtime
 * (@c arch/riscv/core/sbi.S).
 *
 * References: RISC-V SBI Specification v2.0
 * (https://github.com/riscv-non-isa/riscv-sbi-doc)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_

/** @brief SBI extension ID for the Timer extension (TIME) */
#define SBI_EXT_TIME			0x54494D45

/** @brief SBI_EXT_TIME function ID: set the next timer deadline */
#define SBI_FUNC_SET_TIMER		0

/** @brief SBI extension ID for the System Reset extension (SRST) */
#define SBI_EXT_SRST			0x53525354

/** @brief SBI_EXT_SRST function ID: reset or power off the system */
#define SBI_FUNC_SYSTEM_RESET		0

/** @brief SBI_EXT_SRST reset type: clean shutdown (power off) */
#define SBI_SRST_RESET_TYPE_SHUTDOWN	0
/** @brief SBI_EXT_SRST reset type: cold reboot */
#define SBI_SRST_RESET_TYPE_COLD_REBOOT	1
/** @brief SBI_EXT_SRST reset type: warm reboot */
#define SBI_SRST_RESET_TYPE_WARM_REBOOT	2

/** @brief SBI_EXT_SRST reset reason: no specific reason */
#define SBI_SRST_RESET_REASON_NONE	0

/** @brief SBI return code: call completed successfully */
#define SBI_SUCCESS			0
/** @brief SBI return code: requested extension/function is not available */
#define SBI_ERR_NOT_SUPPORTED		-1

#ifndef _ASMLANGUAGE
struct sbiret {
	long error;
	union {
		long value;
		unsigned long uvalue;
	};
};

static inline struct sbiret sbi_ecall(unsigned long arg0, unsigned long arg1, unsigned long arg2,
				      unsigned long arg3, unsigned long arg4, unsigned long arg5,
				      unsigned long fid, unsigned long ext)
{
	struct sbiret ret;

	register unsigned long a0 __asm__("a0") = (arg0);
	register unsigned long a1 __asm__("a1") = (arg1);
	register unsigned long a2 __asm__("a2") = (arg2);
	register unsigned long a3 __asm__("a3") = (arg3);
	register unsigned long a4 __asm__("a4") = (arg4);
	register unsigned long a5 __asm__("a5") = (arg5);
	register unsigned long a6 __asm__("a6") = (fid);
	register unsigned long a7 __asm__("a7") = (ext);

	__asm__ volatile("ecall"
			 : "+r"(a0), "+r"(a1)
			 : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
			 : "memory");

	ret.error = a0;
	ret.value = a1;

	return ret;
}
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_ */
