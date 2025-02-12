/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2015-2020, Linaro Limited
 */
#ifndef _OPTEE_MSG_H
#define _OPTEE_MSG_H

/*
 * TODO: Zephyr has similar macros defined in stdint.h and called UINT32_C,
 * UINT64_C etc. This should be refactored to use macro from Zephyr.
 */
#define U(v) v ## U

/*
 * This file defines the OP-TEE message protocol used to communicate
 * with an instance of OP-TEE running in secure world.
 */

/*****************************************************************************
 * Part 1 - formatting of messages
 *****************************************************************************/

#define OPTEE_MSG_ATTR_TYPE_NONE		U(0x0)
#define OPTEE_MSG_ATTR_TYPE_VALUE_INPUT		U(0x1)
#define OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT	U(0x2)
#define OPTEE_MSG_ATTR_TYPE_VALUE_INOUT		U(0x3)
#define OPTEE_MSG_ATTR_TYPE_RMEM_INPUT		U(0x5)
#define OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT		U(0x6)
#define OPTEE_MSG_ATTR_TYPE_RMEM_INOUT		U(0x7)
#define OPTEE_MSG_ATTR_TYPE_FMEM_INPUT		OPTEE_MSG_ATTR_TYPE_RMEM_INPUT
#define OPTEE_MSG_ATTR_TYPE_FMEM_OUTPUT		OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT
#define OPTEE_MSG_ATTR_TYPE_FMEM_INOUT		OPTEE_MSG_ATTR_TYPE_RMEM_INOUT
#define OPTEE_MSG_ATTR_TYPE_TMEM_INPUT		U(0x9)
#define OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT		U(0xa)
#define OPTEE_MSG_ATTR_TYPE_TMEM_INOUT		U(0xb)

#define OPTEE_MSG_ATTR_TYPE_MASK		GENMASK(7, 0)

/*
 * Meta parameter to be absorbed by the Secure OS and not passed
 * to the Trusted Application.
 *
 * Currently only used with OPTEE_MSG_CMD_OPEN_SESSION.
 */
#define OPTEE_MSG_ATTR_META			BIT(8)

/*
 * Pointer to a list of pages used to register user-defined SHM buffer.
 * Used with OPTEE_MSG_ATTR_TYPE_TMEM_*.
 * buf_ptr should point to the beginning of the buffer. Buffer will contain
 * list of page addresses. OP-TEE core can reconstruct contiguous buffer from
 * that page addresses list. Page addresses are stored as 64 bit values.
 * Last entry on a page should point to the next page of buffer.
 * Every entry in buffer should point to a 4k page beginning (12 least
 * significant bits must be equal to zero).
 *
 * 12 least significant bits of optee_msg_param.u.tmem.buf_ptr should hold
 * page offset of user buffer.
 *
 * So, entries should be placed like members of this structure:
 *
 * struct page_data {
 *   uint64_t pages_array[OPTEE_MSG_NONCONTIG_PAGE_SIZE/sizeof(uint64_t) - 1];
 *   uint64_t next_page_data;
 * };
 *
 * Structure is designed to exactly fit into the page size
 * OPTEE_MSG_NONCONTIG_PAGE_SIZE which is a standard 4KB page.
 *
 * The size of 4KB is chosen because this is the smallest page size for ARM
 * architectures. If REE uses larger pages, it should divide them to 4KB ones.
 */
#define OPTEE_MSG_ATTR_NONCONTIG		BIT(9)

/*
 * Memory attributes for caching passed with temp memrefs. The actual value
 * used is defined outside the message protocol with the exception of
 * OPTEE_MSG_ATTR_CACHE_PREDEFINED which means the attributes already
 * defined for the memory range should be used. If optee_smc.h is used as
 * bearer of this protocol OPTEE_SMC_SHM_* is used for values.
 */
#define OPTEE_MSG_ATTR_CACHE_SHIFT		U(16)
#define OPTEE_MSG_ATTR_CACHE_MASK		GENMASK(2, 0)
#define OPTEE_MSG_ATTR_CACHE_PREDEFINED		U(0)

/*
 * Same values as TEE_LOGIN_* from TEE Internal API
 */
#define OPTEE_MSG_LOGIN_PUBLIC			U(0x00000000)
#define OPTEE_MSG_LOGIN_USER			U(0x00000001)
#define OPTEE_MSG_LOGIN_GROUP			U(0x00000002)
#define OPTEE_MSG_LOGIN_APPLICATION		U(0x00000004)
#define OPTEE_MSG_LOGIN_APPLICATION_USER	U(0x00000005)
#define OPTEE_MSG_LOGIN_APPLICATION_GROUP	U(0x00000006)

/*
 * Page size used in non-contiguous buffer entries
 */
#define OPTEE_MSG_NONCONTIG_PAGE_SIZE		U(4096)

#define OPTEE_MSG_FMEM_INVALID_GLOBAL_ID	0xffffffffffffffff

#ifndef __ASSEMBLER__
/**
 * struct optee_msg_param_tmem - temporary memory reference parameter
 * @buf_ptr:	Address of the buffer
 * @size:	Size of the buffer
 * @shm_ref:	Temporary shared memory reference, pointer to a struct tee_shm
 *
 * Secure and normal world communicates pointers as physical address
 * instead of the virtual address. This is because secure and normal world
 * have completely independent memory mapping. Normal world can even have a
 * hypervisor which need to translate the guest physical address (AKA IPA
 * in ARM documentation) to a real physical address before passing the
 * structure to secure world.
 */
struct optee_msg_param_tmem {
	uint64_t buf_ptr;
	uint64_t size;
	uint64_t shm_ref;
};

/**
 * struct optee_msg_param_rmem - registered memory reference parameter
 * @offs:	Offset into shared memory reference
 * @size:	Size of the buffer
 * @shm_ref:	Shared memory reference, pointer to a struct tee_shm
 */
struct optee_msg_param_rmem {
	uint64_t offs;
	uint64_t size;
	uint64_t shm_ref;
};

/**
 * struct optee_msg_param_fmem - FF-A memory reference parameter
 * @offs_lower:	   Lower bits of offset into shared memory reference
 * @offs_upper:	   Upper bits of offset into shared memory reference
 * @internal_offs: Internal offset into the first page of shared memory
 *		   reference
 * @size:	   Size of the buffer
 * @global_id:	   Global identifier of the shared memory
 */
struct optee_msg_param_fmem {
	uint32_t offs_low;
	uint16_t offs_high;
	uint16_t internal_offs;
	uint64_t size;
	uint64_t global_id;
};

/**
 * struct optee_msg_param_value - opaque value parameter
 *
 * Value parameters are passed unchecked between normal and secure world.
 */
struct optee_msg_param_value {
	uint64_t a;
	uint64_t b;
	uint64_t c;
};

/**
 * struct optee_msg_param - parameter used together with struct optee_msg_arg
 * @attr:	attributes
 * @tmem:	parameter by temporary memory reference
 * @rmem:	parameter by registered memory reference
 * @fmem:	parameter by FF-A registered memory reference
 * @value:	parameter by opaque value
 *
 * @attr & OPTEE_MSG_ATTR_TYPE_MASK indicates if tmem, rmem or value is used in
 * the union. OPTEE_MSG_ATTR_TYPE_VALUE_* indicates value,
 * OPTEE_MSG_ATTR_TYPE_TMEM_* indicates @tmem and
 * OPTEE_MSG_ATTR_TYPE_RMEM_* or the alias PTEE_MSG_ATTR_TYPE_FMEM_* indicates
 * @rmem or @fmem depending on the conduit.
 * OPTEE_MSG_ATTR_TYPE_NONE indicates that none of the members are used.
 */
struct optee_msg_param {
	uint64_t attr;
	union {
		struct optee_msg_param_tmem tmem;
		struct optee_msg_param_rmem rmem;
		struct optee_msg_param_fmem fmem;
		struct optee_msg_param_value value;
	} u;
};

/**
 * struct optee_msg_arg - call argument
 * @cmd: Command, one of OPTEE_MSG_CMD_* or OPTEE_MSG_RPC_CMD_*
 * @func: Trusted Application function, specific to the Trusted Application,
 *	     used if cmd == OPTEE_MSG_CMD_INVOKE_COMMAND
 * @session: In parameter for all OPTEE_MSG_CMD_* except
 *	     OPTEE_MSG_CMD_OPEN_SESSION where it's an output parameter instead
 * @cancel_id: Cancellation id, a unique value to identify this request
 * @ret: return value
 * @ret_origin: origin of the return value
 * @num_params: number of parameters supplied to the OS Command
 * @params: the parameters supplied to the OS Command
 *
 * All normal calls to Trusted OS uses this struct. If cmd requires further
 * information than what these fields hold it can be passed as a parameter
 * tagged as meta (setting the OPTEE_MSG_ATTR_META bit in corresponding
 * attrs field). All parameters tagged as meta have to come first.
 */
struct optee_msg_arg {
	uint32_t cmd;
	uint32_t func;
	uint32_t session;
	uint32_t cancel_id;
	uint32_t pad;
	uint32_t ret;
	uint32_t ret_origin;
	uint32_t num_params;

	/* num_params tells the actual number of element in params */
	struct optee_msg_param params[];
};

/**
 * OPTEE_MSG_GET_ARG_SIZE - return size of struct optee_msg_arg
 *
 * @num_params: Number of parameters embedded in the struct optee_msg_arg
 *
 * Returns the size of the struct optee_msg_arg together with the number
 * of embedded parameters.
 */
#define OPTEE_MSG_GET_ARG_SIZE(num_params) \
	(sizeof(struct optee_msg_arg) + \
	 sizeof(struct optee_msg_param) * (num_params))

/*
 * Defines the maximum value of @num_params that can be passed to
 * OPTEE_MSG_GET_ARG_SIZE without a risk of crossing page boundary.
 */
#define OPTEE_MSG_MAX_NUM_PARAMS	\
	((OPTEE_MSG_NONCONTIG_PAGE_SIZE - sizeof(struct optee_msg_arg)) / \
	 sizeof(struct optee_msg_param))

#endif /*__ASSEMBLER__*/

/*****************************************************************************
 * Part 2 - requests from normal world
 *****************************************************************************/

/*
 * Return the following UID if using API specified in this file without
 * further extensions:
 * 384fb3e0-e7f8-11e3-af63-0002a5d5c51b.
 * Represented in 4 32-bit words in OPTEE_MSG_UID_0, OPTEE_MSG_UID_1,
 * OPTEE_MSG_UID_2, OPTEE_MSG_UID_3.
 */
#define OPTEE_MSG_UID_0			U(0x384fb3e0)
#define OPTEE_MSG_UID_1			U(0xe7f811e3)
#define OPTEE_MSG_UID_2			U(0xaf630002)
#define OPTEE_MSG_UID_3			U(0xa5d5c51b)
#define OPTEE_MSG_FUNCID_CALLS_UID	U(0xFF01)

/*
 * Returns 2.0 if using API specified in this file without further
 * extensions. Represented in 2 32-bit words in OPTEE_MSG_REVISION_MAJOR
 * and OPTEE_MSG_REVISION_MINOR
 */
#define OPTEE_MSG_REVISION_MAJOR	U(2)
#define OPTEE_MSG_REVISION_MINOR	U(0)
#define OPTEE_MSG_FUNCID_CALLS_REVISION	U(0xFF03)

/*
 * Get UUID of Trusted OS.
 *
 * Used by non-secure world to figure out which Trusted OS is installed.
 * Note that returned UUID is the UUID of the Trusted OS, not of the API.
 *
 * Returns UUID in 4 32-bit words in the same way as
 * OPTEE_MSG_FUNCID_CALLS_UID described above.
 */
#define OPTEE_MSG_OS_OPTEE_UUID_0	U(0x486178e0)
#define OPTEE_MSG_OS_OPTEE_UUID_1	U(0xe7f811e3)
#define OPTEE_MSG_OS_OPTEE_UUID_2	U(0xbc5e0002)
#define OPTEE_MSG_OS_OPTEE_UUID_3	U(0xa5d5c51b)
#define OPTEE_MSG_FUNCID_GET_OS_UUID	U(0x0000)

/*
 * Get revision of Trusted OS.
 *
 * Used by non-secure world to figure out which version of the Trusted OS
 * is installed. Note that the returned revision is the revision of the
 * Trusted OS, not of the API.
 *
 * Returns revision in 2 32-bit words in the same way as
 * OPTEE_MSG_CALLS_REVISION described above.
 */
#define OPTEE_MSG_FUNCID_GET_OS_REVISION	U(0x0001)

/*
 * Do a secure call with struct optee_msg_arg as argument
 * The OPTEE_MSG_CMD_* below defines what goes in struct optee_msg_arg::cmd
 *
 * OPTEE_MSG_CMD_OPEN_SESSION opens a session to a Trusted Application.
 * The first two parameters are tagged as meta, holding two value
 * parameters to pass the following information:
 * param[0].u.value.a-b uuid of Trusted Application
 * param[1].u.value.a-b uuid of Client
 * param[1].u.value.c Login class of client OPTEE_MSG_LOGIN_*
 *
 * OPTEE_MSG_CMD_INVOKE_COMMAND invokes a command a previously opened
 * session to a Trusted Application.  struct optee_msg_arg::func is Trusted
 * Application function, specific to the Trusted Application.
 *
 * OPTEE_MSG_CMD_CLOSE_SESSION closes a previously opened session to
 * Trusted Application.
 *
 * OPTEE_MSG_CMD_CANCEL cancels a currently invoked command.
 *
 * OPTEE_MSG_CMD_REGISTER_SHM registers a shared memory reference. The
 * information is passed as:
 * [in] param[0].attr			OPTEE_MSG_ATTR_TYPE_TMEM_INPUT
 *					[| OPTEE_MSG_ATTR_NONCONTIG]
 * [in] param[0].u.tmem.buf_ptr		physical address (of first fragment)
 * [in] param[0].u.tmem.size		size (of first fragment)
 * [in] param[0].u.tmem.shm_ref		holds shared memory reference
 *
 * OPTEE_MSG_CMD_UNREGISTER_SHM unregisteres a previously registered shared
 * memory reference. The information is passed as:
 * [in] param[0].attr			OPTEE_MSG_ATTR_TYPE_RMEM_INPUT
 * [in] param[0].u.rmem.shm_ref		holds shared memory reference
 * [in] param[0].u.rmem.offs		0
 * [in] param[0].u.rmem.size		0
 *
 * OPTEE_MSG_CMD_DO_BOTTOM_HALF does the scheduled bottom half processing
 * of a driver.
 *
 * OPTEE_MSG_CMD_STOP_ASYNC_NOTIF informs secure world that from now is
 * normal world unable to process asynchronous notifications. Typically
 * used when the driver is shut down.
 */
#define OPTEE_MSG_CMD_OPEN_SESSION	U(0)
#define OPTEE_MSG_CMD_INVOKE_COMMAND	U(1)
#define OPTEE_MSG_CMD_CLOSE_SESSION	U(2)
#define OPTEE_MSG_CMD_CANCEL		U(3)
#define OPTEE_MSG_CMD_REGISTER_SHM	U(4)
#define OPTEE_MSG_CMD_UNREGISTER_SHM	U(5)
#define OPTEE_MSG_CMD_DO_BOTTOM_HALF	U(6)
#define OPTEE_MSG_CMD_STOP_ASYNC_NOTIF	U(7)
#define OPTEE_MSG_FUNCID_CALL_WITH_ARG	U(0x0004)

#endif /* _OPTEE_MSG_H */
