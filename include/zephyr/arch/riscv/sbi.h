/*
 * Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V Supervisor Binary Interface (SBI) definitions.
 *
 * Defines SBI extension IDs, function IDs, and error codes used by
 * S-mode code to request M-mode firmware services via the @c ecall
 * instruction. The same calls can be used with Zephyr's minimal SBI
 * runtime or external firmware that provides the requested extension.
 *
 * References: RISC-V SBI Specification v2.0
 * (https://github.com/riscv-non-isa/riscv-sbi-doc)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_

#ifndef _ASMLANGUAGE
#include <errno.h>
#include <stdint.h>
#include <zephyr/sys/util_macro.h>
#endif /* _ASMLANGUAGE */

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

/** @brief SBI extension ID for the Hart State Management extension (HSM) */
#define SBI_EXT_HSM 0x48534D

/** @brief SBI_EXT_HSM function ID: start a stopped hart */
#define SBI_FUNC_HART_START 0
/** @brief SBI_EXT_HSM function ID: stop the calling hart */
#define SBI_FUNC_HART_STOP  1

/** @brief SBI extension ID for the IPI extension (sPI) */
#define SBI_EXT_IPI 0x735049

/** @brief SBI_EXT_IPI function ID: send an IPI to a set of harts */
#define SBI_FUNC_SEND_IPI 0

/** @brief SBI return code: call completed successfully */
#define SBI_SUCCESS			0
/** @brief SBI return code: the call failed for an unspecified reason */
#define SBI_ERR_FAILED                  -1
/** @brief SBI return code: requested extension/function is not available */
#define SBI_ERR_NOT_SUPPORTED           -2
/** @brief SBI return code: one or more arguments were invalid */
#define SBI_ERR_INVALID_PARAM           -3
/** @brief SBI return code: the caller is not permitted to perform the operation */
#define SBI_ERR_DENIED                  -4
/** @brief SBI return code: an address argument was invalid or inaccessible */
#define SBI_ERR_INVALID_ADDRESS         -5
/** @brief SBI return code: the resource is already available */
#define SBI_ERR_ALREADY_AVAILABLE       -6
/** @brief SBI return code: the target has already been started */
#define SBI_ERR_ALREADY_STARTED         -7
/** @brief SBI return code: the target has already been stopped */
#define SBI_ERR_ALREADY_STOPPED         -8
/** @brief SBI return code: no shared memory region has been set up */
#define SBI_ERR_NO_SHMEM                -9
/** @brief SBI return code: the operation is not valid in the current state */
#define SBI_ERR_INVALID_STATE           -10
/** @brief SBI return code: the supplied range is malformed or out of bounds */
#define SBI_ERR_BAD_RANGE               -11
/** @brief SBI return code: the operation did not complete in time */
#define SBI_ERR_TIMEOUT                 -12
/** @brief SBI return code: an I/O error occurred during the operation */
#define SBI_ERR_IO                      -13
/** @brief SBI return code: denied because the resource is locked */
#define SBI_ERR_DENIED_LOCKED           -14

#ifndef _ASMLANGUAGE
/**
 * @brief Translate an SBI return code into a negative errno value.
 *
 * @param err Value taken from the @c error field of an SBI call's return
 *            struct (i.e. register a0).
 * @return 0 on SBI_SUCCESS, otherwise a negative errno. Unknown codes are
 *         reported as -EOPNOTSUPP, since an unrecognised code most likely
 *         comes from an SBI version newer than this header.
 */
static inline int sbi_err_to_errno(unsigned long err)
{
	switch ((long)err) {
	case SBI_SUCCESS:
		return 0;
	case SBI_ERR_FAILED:
		return -EIO;
	case SBI_ERR_INVALID_PARAM:
	case SBI_ERR_INVALID_STATE:
		return -EINVAL;
	case SBI_ERR_DENIED:
	case SBI_ERR_DENIED_LOCKED:
		return -EPERM;
	case SBI_ERR_INVALID_ADDRESS:
		return -EFAULT;
	case SBI_ERR_ALREADY_AVAILABLE:
	case SBI_ERR_ALREADY_STARTED:
	case SBI_ERR_ALREADY_STOPPED:
		return -EALREADY;
	case SBI_ERR_NO_SHMEM:
		return -ENOMEM;
	case SBI_ERR_BAD_RANGE:
		return -ERANGE;
	case SBI_ERR_TIMEOUT:
		return -ETIMEDOUT;
	case SBI_ERR_IO:
		return -EIO;
	case SBI_ERR_NOT_SUPPORTED:
	default:
		return -ENOTSUP;
	}
}

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

/** Program the absolute 64-bit timer deadline, split across a0/a1 on RV32. */
static inline struct sbiret sbi_set_timer(uint64_t deadline)
{
	unsigned long hi = IS_ENABLED(CONFIG_64BIT) ? 0 : (unsigned long)(deadline >> 32);

	return sbi_ecall((unsigned long)deadline, hi, 0, 0, 0, 0, SBI_FUNC_SET_TIMER, SBI_EXT_TIME);
}

/** Request a system reset or shutdown. */
static inline struct sbiret sbi_system_reset(uint32_t type, uint32_t reason)
{
	return sbi_ecall(type, reason, 0, 0, 0, 0, SBI_FUNC_SYSTEM_RESET, SBI_EXT_SRST);
}

/** Send a supervisor software interrupt to the selected harts. */
static inline struct sbiret sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base)
{
	return sbi_ecall(hart_mask, hart_mask_base, 0, 0, 0, 0, SBI_FUNC_SEND_IPI, SBI_EXT_IPI);
}

/** Start a hart at a physical address, passing opaque in a1 on entry. */
static inline struct sbiret sbi_hart_start(unsigned long hartid, unsigned long start_addr,
					   unsigned long opaque)
{
	return sbi_ecall(hartid, start_addr, opaque, 0, 0, 0, SBI_FUNC_HART_START, SBI_EXT_HSM);
}

/** Stop the calling hart. A successful call does not return. */
static inline struct sbiret sbi_hart_stop(void)
{
	return sbi_ecall(0, 0, 0, 0, 0, 0, SBI_FUNC_HART_STOP, SBI_EXT_HSM);
}
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_ */
