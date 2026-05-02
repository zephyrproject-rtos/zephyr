/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2015-2021, Linaro Limited
 */
#ifndef OPTEE_SMC_H
#define OPTEE_SMC_H

#include <stdint.h>

/*
 * This file is exported by OP-TEE and is in kept in sync between secure
 * world and normal world kernel driver. We're following ARM SMC Calling
 * Convention as specified in
 * http://infocenter.arm.com/help/topic/com.arm.doc.den0028a/index.html
 *
 * This file depends on optee_msg.h being included to expand the SMC id
 * macros below.
 */

#define OPTEE_SMC_32			U(0)
#define OPTEE_SMC_64			U(0x40000000)
#define OPTEE_SMC_FAST_CALL		U(0x80000000)
#define OPTEE_SMC_STD_CALL		U(0)

#define OPTEE_SMC_OWNER_MASK		U(0x3F)
#define OPTEE_SMC_OWNER_SHIFT		U(24)

#define OPTEE_SMC_FUNC_MASK		U(0xFFFF)

#define OPTEE_SMC_IS_FAST_CALL(smc_val)	((smc_val) & OPTEE_SMC_FAST_CALL)
#define OPTEE_SMC_IS_64(smc_val)	((smc_val) & OPTEE_SMC_64)
#define OPTEE_SMC_FUNC_NUM(smc_val)	((smc_val) & OPTEE_SMC_FUNC_MASK)
#define OPTEE_SMC_OWNER_NUM(smc_val) \
	(((smc_val) >> OPTEE_SMC_OWNER_SHIFT) & OPTEE_SMC_OWNER_MASK)

#define OPTEE_SMC_CALL_VAL(type, calling_convention, owner, func_num) \
			((type) | (calling_convention) | \
			(((owner) & OPTEE_SMC_OWNER_MASK) << \
				OPTEE_SMC_OWNER_SHIFT) |\
			((func_num) & OPTEE_SMC_FUNC_MASK))

#define OPTEE_SMC_STD_CALL_VAL(func_num) \
	OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_STD_CALL, \
			   OPTEE_SMC_OWNER_TRUSTED_OS, (func_num))
#define OPTEE_SMC_FAST_CALL_VAL(func_num) \
	OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_FAST_CALL, \
			   OPTEE_SMC_OWNER_TRUSTED_OS, (func_num))

#define OPTEE_SMC_OWNER_ARCH		U(0)
#define OPTEE_SMC_OWNER_CPU		U(1)
#define OPTEE_SMC_OWNER_SIP		U(2)
#define OPTEE_SMC_OWNER_OEM		U(3)
#define OPTEE_SMC_OWNER_STANDARD	U(4)
#define OPTEE_SMC_OWNER_TRUSTED_APP	U(48)
#define OPTEE_SMC_OWNER_TRUSTED_OS	U(50)

#define OPTEE_SMC_OWNER_TRUSTED_OS_OPTEED U(62)
#define OPTEE_SMC_OWNER_TRUSTED_OS_API	U(63)

/*
 * Function specified by SMC Calling convention.
 */
#define OPTEE_SMC_FUNCID_CALLS_COUNT	U(0xFF00)
#define OPTEE_SMC_CALLS_COUNT \
	OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_FAST_CALL, \
			   OPTEE_SMC_OWNER_TRUSTED_OS_API, \
			   OPTEE_SMC_FUNCID_CALLS_COUNT)

/*
 * Normal cached memory (write-back), shareable for SMP systems and not
 * shareable for UP systems.
 */
#define OPTEE_SMC_SHM_CACHED		U(1)

/*
 * a0..a7 is used as register names in the descriptions below, on arm32
 * that translates to r0..r7 and on arm64 to w0..w7. In both cases it's
 * 32-bit registers.
 */

/*
 * Function specified by SMC Calling convention
 *
 * Return the following UID if using API specified in this file
 * without further extensions:
 * 384fb3e0-e7f8-11e3-af63-0002a5d5c51b.
 * see also OPTEE_MSG_UID_* in optee_msg.h
 */
#define OPTEE_SMC_FUNCID_CALLS_UID OPTEE_MSG_FUNCID_CALLS_UID
#define OPTEE_SMC_CALLS_UID \
	OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_FAST_CALL, \
			   OPTEE_SMC_OWNER_TRUSTED_OS_API, \
			   OPTEE_SMC_FUNCID_CALLS_UID)

/*
 * Function specified by SMC Calling convention
 *
 * Returns 2.0 if using API specified in this file without further extensions.
 * see also OPTEE_MSG_REVISION_* in optee_msg.h
 */
#define OPTEE_SMC_FUNCID_CALLS_REVISION OPTEE_MSG_FUNCID_CALLS_REVISION
#define OPTEE_SMC_CALLS_REVISION \
	OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_FAST_CALL, \
			   OPTEE_SMC_OWNER_TRUSTED_OS_API, \
			   OPTEE_SMC_FUNCID_CALLS_REVISION)

/*
 * Get UUID of Trusted OS.
 *
 * Used by non-secure world to figure out which Trusted OS is installed.
 * Note that returned UUID is the UUID of the Trusted OS, not of the API.
 *
 * Returns UUID in a0-4 in the same way as OPTEE_SMC_CALLS_UID
 * described above.
 */
#define OPTEE_SMC_FUNCID_GET_OS_UUID OPTEE_MSG_FUNCID_GET_OS_UUID
#define OPTEE_SMC_CALL_GET_OS_UUID \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_OS_UUID)

/*
 * Get revision of Trusted OS.
 *
 * Used by non-secure world to figure out which version of the Trusted OS
 * is installed. Note that the returned revision is the revision of the
 * Trusted OS, not of the API.
 *
 * Returns revision in a0-1 in the same way as OPTEE_SMC_CALLS_REVISION
 * described above. May optionally return a 32-bit build identifier in a2,
 * with zero meaning unspecified.
 */
#define OPTEE_SMC_FUNCID_GET_OS_REVISION OPTEE_MSG_FUNCID_GET_OS_REVISION
#define OPTEE_SMC_CALL_GET_OS_REVISION \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_OS_REVISION)

/*
 * Call with struct optee_msg_arg as argument
 *
 * When called with OPTEE_SMC_CALL_WITH_RPC_ARG or
 * OPTEE_SMC_CALL_WITH_REGD_ARG in a0 there is one RPC struct optee_msg_arg
 * following after the first struct optee_msg_arg. The RPC struct
 * optee_msg_arg has reserved space for the number of RPC parameters as
 * returned by OPTEE_SMC_EXCHANGE_CAPABILITIES.
 *
 * When calling these functions normal world has a few responsibilities:
 * 1. It must be able to handle eventual RPCs
 * 2. Non-secure interrupts should not be masked
 * 3. If asynchronous notifications has been negotiated successfully, then
 *    the interrupt for asynchronous notifications should be unmasked
 *    during this call.
 *
 * Call register usage, OPTEE_SMC_CALL_WITH_ARG and
 * OPTEE_SMC_CALL_WITH_RPC_ARG:
 * a0	SMC Function ID, OPTEE_SMC_CALL_WITH_ARG or OPTEE_SMC_CALL_WITH_RPC_ARG
 * a1	Upper 32 bits of a 64-bit physical pointer to a struct optee_msg_arg
 * a2	Lower 32 bits of a 64-bit physical pointer to a struct optee_msg_arg
 * a3	Cache settings, not used if physical pointer is in a predefined shared
 *	memory area else per OPTEE_SMC_SHM_*
 * a4-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Call register usage, OPTEE_SMC_CALL_WITH_REGD_ARG:
 * a0	SMC Function ID, OPTEE_SMC_CALL_WITH_REGD_ARG
 * a1	Upper 32 bits of a 64-bit shared memory cookie
 * a2	Lower 32 bits of a 64-bit shared memory cookie
 * a3	Offset of the struct optee_msg_arg in the shared memory with the
 *	supplied cookie
 * a4-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	Return value, OPTEE_SMC_RETURN_*
 * a1-3	Not used
 * a4-7	Preserved
 *
 * OPTEE_SMC_RETURN_ETHREAD_LIMIT return register usage:
 * a0	Return value, OPTEE_SMC_RETURN_ETHREAD_LIMIT
 * a1-3	Preserved
 * a4-7	Preserved
 *
 * RPC return register usage:
 * a0	Return value, OPTEE_SMC_RETURN_IS_RPC(val)
 * a1-2	RPC parameters
 * a3-7	Resume information, must be preserved
 *
 * Possible return values:
 * OPTEE_SMC_RETURN_UNKNOWN_FUNCTION	Trusted OS does not recognize this
 *					function.
 * OPTEE_SMC_RETURN_OK			Call completed, result updated in
 *					the previously supplied struct
 *					optee_msg_arg.
 * OPTEE_SMC_RETURN_ETHREAD_LIMIT	Number of Trusted OS threads exceeded,
 *					try again later.
 * OPTEE_SMC_RETURN_EBADADDR		Bad physical pointer to struct
 *					optee_msg_arg.
 * OPTEE_SMC_RETURN_EBADCMD		Bad/unknown cmd in struct optee_msg_arg
 * OPTEE_SMC_RETURN_IS_RPC()		Call suspended by RPC call to normal
 *					world.
 */
#define OPTEE_SMC_FUNCID_CALL_WITH_ARG OPTEE_MSG_FUNCID_CALL_WITH_ARG
#define OPTEE_SMC_CALL_WITH_ARG \
	OPTEE_SMC_STD_CALL_VAL(OPTEE_SMC_FUNCID_CALL_WITH_ARG)
#define OPTEE_SMC_CALL_WITH_RPC_ARG \
	OPTEE_SMC_STD_CALL_VAL(OPTEE_SMC_FUNCID_CALL_WITH_RPC_ARG)
#define OPTEE_SMC_CALL_WITH_REGD_ARG \
	OPTEE_SMC_STD_CALL_VAL(OPTEE_SMC_FUNCID_CALL_WITH_REGD_ARG)

/*
 * Get Shared Memory Config
 *
 * Returns the Secure/Non-secure shared memory config.
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC_GET_SHM_CONFIG
 * a1-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Have config return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1	Physical address of start of SHM
 * a2	Size of of SHM
 * a3	Cache settings of memory, as defined by the
 *	OPTEE_SMC_SHM_* values above
 * a4-7	Preserved
 *
 * Not available register usage:
 * a0	OPTEE_SMC_RETURN_ENOTAVAIL
 * a1-3 Not used
 * a4-7	Preserved
 */
#define OPTEE_SMC_FUNCID_GET_SHM_CONFIG	7
#define OPTEE_SMC_GET_SHM_CONFIG \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_SHM_CONFIG)

/*
 * Configures L2CC mutex
 *
 * Disables, enables usage of L2CC mutex. Returns or sets physical address
 * of L2CC mutex.
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC_L2CC_MUTEX
 * a1	OPTEE_SMC_L2CC_MUTEX_GET_ADDR	Get physical address of mutex
 *	OPTEE_SMC_L2CC_MUTEX_SET_ADDR	Set physical address of mutex
 *	OPTEE_SMC_L2CC_MUTEX_ENABLE	Enable usage of mutex
 *	OPTEE_SMC_L2CC_MUTEX_DISABLE	Disable usage of mutex
 * a2	if a1 == OPTEE_SMC_L2CC_MUTEX_SET_ADDR, upper 32bit of a 64bit
 *      physical address of mutex
 * a3	if a1 == OPTEE_SMC_L2CC_MUTEX_SET_ADDR, lower 32bit of a 64bit
 *      physical address of mutex
 * a3-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Have config return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1	Preserved
 * a2	if a1 == OPTEE_SMC_L2CC_MUTEX_GET_ADDR, upper 32bit of a 64bit
 *      physical address of mutex
 * a3	if a1 == OPTEE_SMC_L2CC_MUTEX_GET_ADDR, lower 32bit of a 64bit
 *      physical address of mutex
 * a3-7	Preserved
 *
 * Error return register usage:
 * a0	OPTEE_SMC_RETURN_ENOTAVAIL	Physical address not available
 *	OPTEE_SMC_RETURN_EBADADDR	Bad supplied physical address
 *	OPTEE_SMC_RETURN_EBADCMD	Unsupported value in a1
 * a1-7	Preserved
 */
#define OPTEE_SMC_L2CC_MUTEX_GET_ADDR	U(0)
#define OPTEE_SMC_L2CC_MUTEX_SET_ADDR	U(1)
#define OPTEE_SMC_L2CC_MUTEX_ENABLE	U(2)
#define OPTEE_SMC_L2CC_MUTEX_DISABLE	U(3)
#define OPTEE_SMC_FUNCID_L2CC_MUTEX	U(8)
#define OPTEE_SMC_L2CC_MUTEX \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_L2CC_MUTEX)

/*
 * Exchanges capabilities between normal world and secure world
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC_EXCHANGE_CAPABILITIES
 * a1	bitfield of normal world capabilities OPTEE_SMC_NSEC_CAP_*
 * a2-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1	bitfield of secure world capabilities OPTEE_SMC_SEC_CAP_*
 * a2	The maximum secure world notification number
 * a3	Bit[7:0]: Number of parameters needed for RPC to be supplied
 *		  as the second MSG arg struct for
 *		  OPTEE_SMC_CALL_WITH_ARG
 *	Bit[31:8]: Reserved (MBZ)
 * a3-7	Preserved
 *
 * Error return register usage:
 * a0	OPTEE_SMC_RETURN_ENOTAVAIL, can't use the capabilities from normal world
 * a1	bitfield of secure world capabilities OPTEE_SMC_SEC_CAP_*
 * a2-7 Preserved
 */
/* Normal world works as a uniprocessor system */
#define OPTEE_SMC_NSEC_CAP_UNIPROCESSOR		BIT(0)
/* Secure world has reserved shared memory for normal world to use */
#define OPTEE_SMC_SEC_CAP_HAVE_RESERVED_SHM	BIT(0)
/* Secure world can communicate via previously unregistered shared memory */
#define OPTEE_SMC_SEC_CAP_UNREGISTERED_SHM	BIT(1)
/*
 * Secure world supports commands "register/unregister shared memory",
 * secure world accepts command buffers located in any parts of non-secure RAM
 */
#define OPTEE_SMC_SEC_CAP_DYNAMIC_SHM		BIT(2)
/* Secure world is built with virtualization support */
#define OPTEE_SMC_SEC_CAP_VIRTUALIZATION	BIT(3)
/* Secure world supports Shared Memory with a NULL reference */
#define OPTEE_SMC_SEC_CAP_MEMREF_NULL		BIT(4)
/* Secure world supports asynchronous notification of normal world */
#define OPTEE_SMC_SEC_CAP_ASYNC_NOTIF		BIT(5)
/* Secure world supports pre-allocating RPC arg struct */
#define OPTEE_SMC_SEC_CAP_RPC_ARG		BIT(6)

#define OPTEE_SMC_FUNCID_EXCHANGE_CAPABILITIES	U(9)
#define OPTEE_SMC_EXCHANGE_CAPABILITIES \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_EXCHANGE_CAPABILITIES)

/*
 * Disable and empties cache of shared memory objects
 *
 * Secure world can cache frequently used shared memory objects, for
 * example objects used as RPC arguments. When secure world is idle this
 * function returns one shared memory reference to free. To disable the
 * cache and free all cached objects this function has to be called until
 * it returns OPTEE_SMC_RETURN_ENOTAVAIL.
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC_DISABLE_SHM_CACHE
 * a1-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1	Upper 32 bits of a 64-bit Shared memory cookie
 * a2	Lower 32 bits of a 64-bit Shared memory cookie
 * a3-7	Preserved
 *
 * Cache empty return register usage:
 * a0	OPTEE_SMC_RETURN_ENOTAVAIL
 * a1-7	Preserved
 *
 * Not idle return register usage:
 * a0	OPTEE_SMC_RETURN_EBUSY
 * a1-7	Preserved
 */
#define OPTEE_SMC_FUNCID_DISABLE_SHM_CACHE	U(10)
#define OPTEE_SMC_DISABLE_SHM_CACHE \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_DISABLE_SHM_CACHE)

/*
 * Enable cache of shared memory objects
 *
 * Secure world can cache frequently used shared memory objects, for
 * example objects used as RPC arguments. When secure world is idle this
 * function returns OPTEE_SMC_RETURN_OK and the cache is enabled. If
 * secure world isn't idle OPTEE_SMC_RETURN_EBUSY is returned.
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC_ENABLE_SHM_CACHE
 * a1-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1-7	Preserved
 *
 * Not idle return register usage:
 * a0	OPTEE_SMC_RETURN_EBUSY
 * a1-7	Preserved
 */
#define OPTEE_SMC_FUNCID_ENABLE_SHM_CACHE	U(11)
#define OPTEE_SMC_ENABLE_SHM_CACHE \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_ENABLE_SHM_CACHE)

/*
 * Release of secondary cores
 *
 * OP-TEE in secure world is in charge of the release process of secondary
 * cores. The Rich OS issue the this request to ask OP-TEE to boot up the
 * secondary cores, go through the OP-TEE per-core initialization, and then
 * switch to the Non-seCure world with the Rich OS provided entry address.
 * The secondary cores enter Non-Secure world in SVC mode, with Thumb, FIQ,
 * IRQ and Abort bits disabled.
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC_BOOT_SECONDARY
 * a1	Index of secondary core to boot
 * a2	Upper 32 bits of a 64-bit Non-Secure world entry physical address
 * a3	Lower 32 bits of a 64-bit Non-Secure world entry physical address
 * a4-7	Not used
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1-7	Preserved
 *
 * Error return:
 * a0	OPTEE_SMC_RETURN_EBADCMD		Core index out of range
 * a1-7	Preserved
 *
 * Not idle return register usage:
 * a0	OPTEE_SMC_RETURN_EBUSY
 * a1-7	Preserved
 */
#define OPTEE_SMC_FUNCID_BOOT_SECONDARY  U(12)
#define OPTEE_SMC_BOOT_SECONDARY \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_BOOT_SECONDARY)

/*
 * Inform OP-TEE about a new virtual machine
 *
 * Hypervisor issues this call during virtual machine (guest) creation.
 * OP-TEE records client id of new virtual machine and prepares
 * to receive requests from it. This call is available only if OP-TEE
 * was built with virtualization support.
 *
 * Call requests usage:
 * a0	SMC Function ID, OPTEE_SMC_VM_CREATED
 * a1	Hypervisor Client ID of newly created virtual machine
 * a2-6 Not used
 * a7	Hypervisor Client ID register. Must be 0, because only hypervisor
 *      can issue this call
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1-7	Preserved
 *
 * Error return:
 * a0	OPTEE_SMC_RETURN_ENOTAVAIL	OP-TEE have no resources for
 *					another VM
 * a1-7	Preserved
 *
 */
#define OPTEE_SMC_FUNCID_VM_CREATED	U(13)
#define OPTEE_SMC_VM_CREATED \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_VM_CREATED)

/*
 * Inform OP-TEE about shutdown of a virtual machine
 *
 * Hypervisor issues this call during virtual machine (guest) destruction.
 * OP-TEE will clean up all resources associated with this VM. This call is
 * available only if OP-TEE was built with virtualization support.
 *
 * Call requests usage:
 * a0	SMC Function ID, OPTEE_SMC_VM_DESTROYED
 * a1	Hypervisor Client ID of virtual machine being shut down
 * a2-6 Not used
 * a7	Hypervisor Client ID register. Must be 0, because only hypervisor
 *      can issue this call
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1-7	Preserved
 *
 */
#define OPTEE_SMC_FUNCID_VM_DESTROYED	U(14)
#define OPTEE_SMC_VM_DESTROYED \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_VM_DESTROYED)

/*
 * Query OP-TEE about number of supported threads
 *
 * Normal World OS or Hypervisor issues this call to find out how many
 * threads OP-TEE supports. That is how many standard calls can be issued
 * in parallel before OP-TEE will return OPTEE_SMC_RETURN_ETHREAD_LIMIT.
 *
 * Call requests usage:
 * a0	SMC Function ID, OPTEE_SMC_GET_THREAD_COUNT
 * a1-6 Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1	Number of threads
 * a2-7 Preserved
 *
 * Error return:
 * a0	OPTEE_SMC_RETURN_UNKNOWN_FUNCTION   Requested call is not implemented
 * a1-7	Preserved
 */
#define OPTEE_SMC_FUNCID_GET_THREAD_COUNT	U(15)
#define OPTEE_SMC_GET_THREAD_COUNT \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_THREAD_COUNT)

/*
 * Inform OP-TEE that normal world is able to receive asynchronous
 * notifications.
 *
 * Call requests usage:
 * a0	SMC Function ID, OPTEE_SMC_ENABLE_ASYNC_NOTIF
 * a1-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1-7	Preserved
 *
 * Not supported return register usage:
 * a0	OPTEE_SMC_RETURN_ENOTAVAIL
 * a1-7	Preserved
 */
#define OPTEE_SMC_FUNCID_ENABLE_ASYNC_NOTIF	16
#define OPTEE_SMC_ENABLE_ASYNC_NOTIF \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_ENABLE_ASYNC_NOTIF)

/*
 * Retrieve a value of notifications pending since the last call of this
 * function.
 *
 * OP-TEE keeps a record of all posted values. When an interrupt is
 * received which indicates that there are posted values this function
 * should be called until all pended values have been retrieved. When a
 * value is retrieved, it's cleared from the record in secure world.
 *
 * It is expected that this function is called from an interrupt handler
 * in normal world.
 *
 * Call requests usage:
 * a0	SMC Function ID, OPTEE_SMC_GET_ASYNC_NOTIF_VALUE
 * a1-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	OPTEE_SMC_RETURN_OK
 * a1	value
 * a2	Bit[0]: OPTEE_SMC_ASYNC_NOTIF_VALUE_VALID if the value in a1 is
 *		valid, else 0 if no values were pending
 * a2	Bit[1]: OPTEE_SMC_ASYNC_NOTIF_VALUE_PENDING if another value is
 *		pending, else 0.
 *	Bit[31:2]: MBZ
 * a3-7	Preserved
 *
 * Not supported return register usage:
 * a0	OPTEE_SMC_RETURN_ENOTAVAIL
 * a1-7	Preserved
 */
#define OPTEE_SMC_ASYNC_NOTIF_VALID	BIT(0)
#define OPTEE_SMC_ASYNC_NOTIF_PENDING	BIT(1)

/*
 * Notification that OP-TEE expects a yielding call to do some bottom half
 * work in a driver.
 */
#define OPTEE_SMC_ASYNC_NOTIF_VALUE_DO_BOTTOM_HALF	0

#define OPTEE_SMC_FUNCID_GET_ASYNC_NOTIF_VALUE	17
#define OPTEE_SMC_GET_ASYNC_NOTIF_VALUE \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_ASYNC_NOTIF_VALUE)

/* See OPTEE_SMC_CALL_WITH_RPC_ARG above */
#define OPTEE_SMC_FUNCID_CALL_WITH_RPC_ARG	U(18)

/* See OPTEE_SMC_CALL_WITH_REGD_ARG above */
#define OPTEE_SMC_FUNCID_CALL_WITH_REGD_ARG	U(19)

/*
 * Resume from RPC (for example after processing a foreign interrupt)
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC
 * a1-3	Value of a1-3 when OPTEE_SMC_CALL_WITH_ARG returned
 *	OPTEE_SMC_RETURN_RPC in a0
 *
 * Return register usage is the same as for OPTEE_SMC_*CALL_WITH_ARG above.
 *
 * Possible return values
 * OPTEE_SMC_RETURN_UNKNOWN_FUNCTION	Trusted OS does not recognize this
 *					function.
 * OPTEE_SMC_RETURN_OK			Original call completed, result
 *					updated in the previously supplied.
 *					struct optee_msg_arg
 * OPTEE_SMC_RETURN_RPC			Call suspended by RPC call to normal
 *					world.
 * OPTEE_SMC_RETURN_ERESUME		Resume failed, the opaque resume
 *					information was corrupt.
 */
#define OPTEE_SMC_FUNCID_RETURN_FROM_RPC	U(3)
#define OPTEE_SMC_CALL_RETURN_FROM_RPC \
	OPTEE_SMC_STD_CALL_VAL(OPTEE_SMC_FUNCID_RETURN_FROM_RPC)

#define OPTEE_SMC_RETURN_RPC_PREFIX_MASK	U(0xFFFF0000)
#define OPTEE_SMC_RETURN_RPC_PREFIX		U(0xFFFF0000)
#define OPTEE_SMC_RETURN_RPC_FUNC_MASK		U(0x0000FFFF)

#define OPTEE_SMC_RETURN_GET_RPC_FUNC(ret) \
	((ret) & OPTEE_SMC_RETURN_RPC_FUNC_MASK)

#define OPTEE_SMC_RPC_VAL(func)		((func) | OPTEE_SMC_RETURN_RPC_PREFIX)

/*
 * Allocate memory for RPC parameter passing. The memory is used to hold a
 * struct optee_msg_arg.
 *
 * "Call" register usage:
 * a0	This value, OPTEE_SMC_RETURN_RPC_ALLOC
 * a1	Size in bytes of required argument memory
 * a2	Not used
 * a3	Resume information, must be preserved
 * a4-5	Not used
 * a6-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC.
 * a1	Upper 32 bits of 64-bit physical pointer to allocated
 *	memory, (a1 == 0 && a2 == 0) if size was 0 or if memory can't
 *	be allocated.
 * a2	Lower 32 bits of 64-bit physical pointer to allocated
 *	memory, (a1 == 0 && a2 == 0) if size was 0 or if memory can't
 *	be allocated
 * a3	Preserved
 * a4	Upper 32 bits of 64-bit Shared memory cookie used when freeing
 *	the memory or doing an RPC
 * a5	Lower 32 bits of 64-bit Shared memory cookie used when freeing
 *	the memory or doing an RPC
 * a6-7	Preserved
 */
#define OPTEE_SMC_RPC_FUNC_ALLOC	U(0)
#define OPTEE_SMC_RETURN_RPC_ALLOC \
	OPTEE_SMC_RPC_VAL(OPTEE_SMC_RPC_FUNC_ALLOC)

/*
 * Free memory previously allocated by OPTEE_SMC_RETURN_RPC_ALLOC
 *
 * "Call" register usage:
 * a0	This value, OPTEE_SMC_RETURN_RPC_FREE
 * a1	Upper 32 bits of 64-bit shared memory cookie belonging to this
 *	argument memory
 * a2	Lower 32 bits of 64-bit shared memory cookie belonging to this
 *	argument memory
 * a3-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC.
 * a1-2	Not used
 * a3-7	Preserved
 */
#define OPTEE_SMC_RPC_FUNC_FREE		U(2)
#define OPTEE_SMC_RETURN_RPC_FREE \
	OPTEE_SMC_RPC_VAL(OPTEE_SMC_RPC_FUNC_FREE)

/*
 * Deliver a foreign interrupt in normal world.
 *
 * "Call" register usage:
 * a0	OPTEE_SMC_RETURN_RPC_FOREIGN_INTR
 * a1-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC.
 * a1-7	Preserved
 */
#define OPTEE_SMC_RPC_FUNC_FOREIGN_INTR	U(4)
#define OPTEE_SMC_RETURN_RPC_FOREIGN_INTR \
	OPTEE_SMC_RPC_VAL(OPTEE_SMC_RPC_FUNC_FOREIGN_INTR)

/*
 * Do an RPC request. The supplied struct optee_msg_arg tells which
 * request to do and the parameters for the request. The following fields
 * are used (the rest are unused):
 * - cmd		the Request ID
 * - ret		return value of the request, filled in by normal world
 * - num_params		number of parameters for the request
 * - params		the parameters
 * - param_attrs	attributes of the parameters
 *
 * "Call" register usage:
 * a0	OPTEE_SMC_RETURN_RPC_CMD
 * a1	Upper 32 bits of a 64-bit Shared memory cookie holding a
 *	struct optee_msg_arg, must be preserved, only the data should
 *	be updated
 * a2	Lower 32 bits of a 64-bit Shared memory cookie holding a
 *	struct optee_msg_arg, must be preserved, only the data should
 *	be updated
 * a3-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC.
 * a1-2	Not used
 * a3-7	Preserved
 */
#define OPTEE_SMC_RPC_FUNC_CMD		U(5)
#define OPTEE_SMC_RETURN_RPC_CMD \
	OPTEE_SMC_RPC_VAL(OPTEE_SMC_RPC_FUNC_CMD)

/* Returned in a0 */
#define OPTEE_SMC_RETURN_UNKNOWN_FUNCTION U(0xFFFFFFFF)

/* Returned in a0 only from Trusted OS functions */
#define OPTEE_SMC_RETURN_OK		U(0x0)
#define OPTEE_SMC_RETURN_ETHREAD_LIMIT	U(0x1)
#define OPTEE_SMC_RETURN_EBUSY		U(0x2)
#define OPTEE_SMC_RETURN_ERESUME	U(0x3)
#define OPTEE_SMC_RETURN_EBADADDR	U(0x4)
#define OPTEE_SMC_RETURN_EBADCMD	U(0x5)
#define OPTEE_SMC_RETURN_ENOMEM		U(0x6)
#define OPTEE_SMC_RETURN_ENOTAVAIL	U(0x7)
#define OPTEE_SMC_RETURN_IS_RPC(ret) \
	(((ret) != OPTEE_SMC_RETURN_UNKNOWN_FUNCTION) && \
	((((ret) & OPTEE_SMC_RETURN_RPC_PREFIX_MASK) == \
		OPTEE_SMC_RETURN_RPC_PREFIX)))

#endif /* OPTEE_SMC_H */
