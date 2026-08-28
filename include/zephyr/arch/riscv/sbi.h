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
 * instruction.  The subset defined here covers only the extensions
 * used by Zephyr's in-tree minimal SBI runtime
 * (@c arch/riscv/core/sbi.S).
 *
 * References: RISC-V SBI Specification v2.0
 * (https://github.com/riscv-non-isa/riscv-sbi-doc)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_

#ifndef _ASMLANGUAGE
#include <errno.h>
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
	case SBI_ERR_NOT_SUPPORTED:
		return -EOPNOTSUPP;
	case SBI_ERR_INVALID_PARAM:
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
	case SBI_ERR_INVALID_STATE:
		return -EINVAL;
	case SBI_ERR_BAD_RANGE:
		return -ERANGE;
	case SBI_ERR_TIMEOUT:
		return -ETIMEDOUT;
	case SBI_ERR_IO:
		return -EIO;
	default:
		return -EOPNOTSUPP;
	}
}
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_SBI_H_ */
