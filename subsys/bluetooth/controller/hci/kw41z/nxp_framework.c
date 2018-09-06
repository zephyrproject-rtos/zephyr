/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2018 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME BLE_KW41Z
#define LOG_LEVEL CONFIG_LOG_BT_HCI_DRIVER_LEVEL

#include <logging/log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME, LOG_LEVEL);


#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr.h>
#include <arch/cpu.h>
#include <device.h>

#include "fsl_os_abstraction.h"
#include "nxp_private.h"

/*
 * ---------------------------------------------------------------------
 *
 * Coexstence support
 *
 * The KW41Z link layer library uses this API to determine if usage of
 * the radio is allowed. Additional framework is required to more fully
 * realize coexistence on Zephyr & KW41Z.
 *
 * @return  uint8_t !CONFIG_KW41_COEX_RF_DENY_ACTIVE_STATE - RF Deny Inactive,
 *                   CONFIG_KW41_COEX_RF_DENY_ACTIVE_STATE - RF Deny Active
 *
 * ---------------------------------------------------------------------
 */
uint8_t MWS_CoexistenceDenyState(void)
{
	uint8_t retVal = !CONFIG_KW41_COEX_RF_DENY_ACTIVE_STATE;

	LOG_DBG("Entry");

#if gMWS_UseCoexistence_d
	if (rf_deny) {
		if (GpioReadPinInput(rf_deny) ==
		    CONFIG_KW41_COEX_RF_DENY_ACTIVE_STATE) {
			retVal = CONFIG_KW41_COEX_RF_DENY_ACTIVE_STATE;
		}
	}
#endif
	return retVal;
}

/*
 * ---------------------------------------------------------------------
 *
 * memcpy, memset, and memcmp abstractions
 *
 * Controller library uses these abstractions to perform the memory
 * functions.
 *
 * ---------------------------------------------------------------------
 */

void FLib_MemCpy(void *pDst, void *pSrc, uint32_t cBytes)
{
	(void)memcpy(pDst, pSrc, cBytes);
}

void FLib_MemSet(void *pData, uint8_t value, uint32_t cBytes)
{
	(void)memset(pData, value, cBytes);
}

bool_t FLib_MemCmp(void *pData1, void *pData2, uint32_t cBytes)
{
	bool_t status = TRUE;

	if (memcmp(pData1, pData2, cBytes)) {
		status = FALSE;
	}
	return status;
}

/*
 * ---------------------------------------------------------------------
 *
 * Buffer management services abstractions
 *
 * The NXP connectivity stack uses a series of buffer pools that align
 * to the different size requests the system instead of a single pool
 * that gets carved up. It helps avoid fragmenting memory. Below is
 * an emulation of the same thing using k_mem_pool. An alternative is to use
 * a single buffer pool that is sized according to the needs of the application
 * and how it influences the memory needs of the link layer. Could potentially
 * be subject to fragmentation but either path requires some realtime tuning
 * and understanding of the application. What might be better is to just create
 * a buffer pool out of the "rest of memory" not currently allocated by the
 * application at compile time.
 *
 * Need to profile additional demo applications to better understand the buffer
 * needs of the controller. Not crazy with the breakout of this right now.
 *
 * ---------------------------------------------------------------------
 */

/*
 * k_mem_pool defines the smallest and largest buffer block available
 * in the pool. What is not clearly documented is that there is header
 * overhead of 4 bytes to manage the buffers. Result is that if you
 * define a pool of 32byte blocks an allocation request for 29 bytes
 * will fail. The ADJ_POOL_SIZE() macro "fixes" up the size to account
 * for this. Also, the block size needs to be a power of 2 hence the +3
 * and shifts.
 */
#define ADJ_POOL_SIZE(n) (((((n) + 4) + 3) >> 2) << 2)
#define MEM_MANAGER_POOL(_size, _cnt)				 \
	K_MEM_POOL_DEFINE(mm_pool_##_size, ADJ_POOL_SIZE(_size), \
			  ADJ_POOL_SIZE(_size), _cnt, 4)

#if (CONFIG_KW41_POOL_32_CNT > 0)
MEM_MANAGER_POOL(32, CONFIG_KW41_POOL_32_CNT);
#endif
#if (CONFIG_KW41_POOL_64_CNT > 0)
MEM_MANAGER_POOL(64, CONFIG_KW41_POOL_64_CNT);
#endif
#if (CONFIG_KW41_POOL_128_CNT > 0)
MEM_MANAGER_POOL(128, CONFIG_KW41_POOL_128_CNT);
#endif
#if (CONFIG_KW41_POOL_256_CNT > 0)
MEM_MANAGER_POOL(256, CONFIG_KW41_POOL_256_CNT);
#endif
#if (CONFIG_KW41_POOL_512_CNT > 0)
MEM_MANAGER_POOL(512, CONFIG_KW41_POOL_512_CNT);
#endif
#if (CONFIG_KW41_POOL_1024_CNT > 0)
MEM_MANAGER_POOL(1024, CONFIG_KW41_POOL_1024_CNT);
#endif

struct mem_manager_pools {
	struct k_mem_pool *pool;
	uint32_t size;  /* Buffer size */
	uint32_t cnt;   /* Total avail */
	uint32_t curr;  /* Current usage */
	uint32_t high;  /* High water count for number of allocations */
	uint32_t max;   /* Maximum allocation request for pool */
};

struct mem_manager_pools mm_pools[] = {
#if (CONFIG_KW41_POOL_32_CNT > 0)
	{ &mm_pool_32, 32, CONFIG_KW41_POOL_32_CNT, 0, 0, 0 },
#endif
#if (CONFIG_KW41_POOL_64_CNT > 0)
	{ &mm_pool_64, 64, CONFIG_KW41_POOL_64_CNT, 0, 0, 0 },
#endif
#if (CONFIG_KW41_POOL_128_CNT > 0)
	{ &mm_pool_128, 128, CONFIG_KW41_POOL_128_CNT, 0, 0, 0 },
#endif
#if (CONFIG_KW41_POOL_256_CNT > 0)
	{ &mm_pool_256, 256, CONFIG_KW41_POOL_256_CNT, 0, 0, 0 },
#endif
#if (CONFIG_KW41_POOL_512_CNT > 0)
	{ &mm_pool_512, 512, CONFIG_KW41_POOL_512_CNT, 0, 0, 0 },
#endif
#if (CONFIG_KW41_POOL_1024_CNT > 0)
	{ &mm_pool_1024, 1024, CONFIG_KW41_POOL_1024_CNT, 0, 0, 0 },
#endif
};

#define MM_POOLS_CNT  ARRAY_SIZE(mm_pools)

enum memStatus {
	MEM_SUCCESS_c = 0,              /* No error occurred */
	MEM_INIT_ERROR_c,               /* Memory initialization error */
	MEM_ALLOC_ERROR_c,              /* Memory allocation error */
	MEM_FREE_ERROR_c,               /* Memory free error */
	MEM_UNKNOWN_ERROR_c             /* something bad has happened */
};

#ifdef CONFIG_KW41_MEM_USAGE_STATS

extern struct k_mem_pool _k_mem_pool_list_start[];

void MEM_PrintInfo(void)
{
	printk("MEM Pools Statistics:\n");
	for (int i = 0; i < MM_POOLS_CNT; i++) {
		printk("   pool: %p, size: %ld, cur: %ld, max: %ld, high: %ld\n",
		       mm_pools[i].pool,
		       mm_pools[i].size,
		       mm_pools[i].curr,
		       mm_pools[i].max,
		       mm_pools[i].high);
	}
}

void MEM_StatFree(void *buffer)
{
	struct k_mem_block_id *id =
		buffer - sizeof(struct k_mem_block_id);
	struct k_mem_pool *pool = &_k_mem_pool_list_start[id->pool];

	for (int i = 0; i < MM_POOLS_CNT; i++) {
		if (mm_pools[i].pool == pool) {
			mm_pools[i].curr--;
		}
	}
}

void MEM_StatAlloc(struct mem_manager_pools *pool, uint32_t numBytes)
{
	pool->curr++;
	if (pool->curr > pool->high) {
		pool->high = pool->curr;
	}
	if (pool->max < numBytes) {
		pool->max = numBytes;
	}
}

#else

#define MEM_PrintInfo()
#define MEM_StatFree(x)
#define MEM_StatAlloc(x, y)

#endif

/*
 * Allocate a block from the memory pools. The function uses the
 * numBytes argument to look up a pool with adequate block sizes.
 *
 * @param[in] numBytes - Size of buffer to allocate.
 * @param[in] poolId - The ID of the pool where to search for a free
 *                     buffer.
 * @param[in] pCaller - pointer to the caller function (Debug purpose)
 *
 * @return Pointer to the allocated buffer, NULL if failed.
 *
 * @pre Memory manager must be previously initialized.
 *
 */
void *MEM_BufferAllocWithId(
	uint32_t numBytes,
	uint8_t poolId,
	void *pCaller)
{
	struct k_mem_pool *pool = NULL;
	void *rslt = NULL;
	unsigned int i;

	(void)poolId;

	/* Find pool */
	for (i = 0; i < MM_POOLS_CNT; i++) {
		if (mm_pools[i].size >= numBytes) {
			pool = mm_pools[i].pool;
			break;
		}
	}

	if (pool) {
		rslt = k_mem_pool_malloc(pool, numBytes);
	}

	if (rslt) {
		MEM_StatAlloc(&mm_pools[i], numBytes);
	} else {
		LOG_ERR("ERROR! Alloc request failure!, pool: %d, numBytes: %"
			PRIu32 "", i, numBytes);
		MEM_PrintInfo();
	}

	return rslt;
}

/*
 * Deallocate a memory block by putting it in the corresponding pool
 * of free blocks.
 *
 * @param[in] buffer - Pointer to buffer to deallocate.
 *
 * @return MEM_SUCCESS_c if deallocation was successful,
 *         MEM_FREE_ERROR_c if not.
 *
 * @pre Memory manager must be previously initialized.
 *
 * @remarks Never deallocate the same buffer twice.
 *
 */
enum memStatus MEM_BufferFree(void *buffer)
{
	MEM_StatFree(buffer);

	k_free(buffer);

	MEM_PrintInfo();

	return MEM_SUCCESS_c;
}

/*
 * ---------------------------------------------------------------------
 *
 * Timer services abstractions
 *
 * NXP IP utilizes a Timer Manager framework to allocate, start, stop,
 * and free timers. This provides the expected API functions tied to the
 * Zephyr API functions.
 *
 * ---------------------------------------------------------------------
 */

#define gTmrSingleShotTimer_c   0x01
#define gTmrIntervalTimer_c     0x02

typedef enum tmrErrCode_tag {
    gTmrSuccess_c,
    gTmrInvalidId_c,
    gTmrOutOfRange_c
} tmrErrCode_t;

typedef uint32_t tmrTimeInMilliseconds_t;
typedef uint8_t tmrTimerID_t;
typedef uint8_t tmrTimerType_t;

/*
 * Timer callback function
 */
typedef void (*pfTmrCallBack_t)(void *param);


struct tmrTimerTableEntry {
	struct k_timer timer;
	bool allocated;
	pfTmrCallBack_t callback;
	void            *param;
};

static void tmr_handler(struct k_timer *timer);

/*
 * The KW41Z Link Layer library require one timer. A Kconfig value is used
 * in case this changes in the future.
 */
struct tmrTimerTableEntry tmr_table[CONFIG_KW41_NUM_TIMERS];

static void tmr_handler(struct k_timer *timer)
{
	int timer_num;

	for (timer_num = 0;
	     timer_num < ARRAY_SIZE(tmr_table);
	     timer_num++) {
		if (&tmr_table[timer_num].timer == timer) {
			break;
		}
	}

	if (timer_num == ARRAY_SIZE(tmr_table)) {
		LOG_ERR("WRONG TIMER???");
		return;
	}

	tmr_table[timer_num].callback(k_timer_user_data_get(timer));
}

/*
 * Allocate a timer
 *
 * return  timer ID
 */
tmrTimerID_t TMR_AllocateTimer(void)
{
	tmrTimerID_t id = 0xFF;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(tmr_table); i++) {
		if (!tmr_table[i].allocated) {
			id = i;
			tmr_table[id].allocated = TRUE;
			k_timer_init(&tmr_table[id].timer,
				     tmr_handler, NULL);
			break;
		}
	}
	LOG_DBG("ID: %d", id);
	return id;
}

/*
 * Stop a timer
 *
 * Associated timer callback function is not called, even if the timer
 * expires. Does not frees the timer. Safe to call anytime, regardless
 * of the state of the timer.
 *
 * @param timerID - the ID of the timer
 *
 */
tmrErrCode_t TMR_StopTimer(tmrTimerID_t timerID)
{
	tmrErrCode_t err = gTmrSuccess_c;

	LOG_DBG("ID: %d", timerID);
	/* check if timer is not allocated or if it has an invalid ID */
	if ((timerID >= ARRAY_SIZE(tmr_table)) ||
	    (!tmr_table[timerID].allocated)) {
		err = gTmrInvalidId_c;
	} else {
		k_timer_stop(&tmr_table[timerID].timer);
		tmr_table[timerID].callback = NULL;
		tmr_table[timerID].param = (void *)0xDEADBEEF;
	}

	return err;
}

/*
 * Free a timer
 *
 * Safe to call even if the timer is running. Harmless if the timer is
 * already free.
 *
 * @param[in] timerID - the ID of the timer
 */
tmrErrCode_t TMR_FreeTimer(tmrTimerID_t timerID)
{
	tmrErrCode_t status;

	LOG_DBG("ID: %d", timerID);
	status = TMR_StopTimer(timerID);

	if (status == gTmrSuccess_c) {
		tmr_table[timerID].allocated = FALSE;
	}

	return gTmrSuccess_c;
}

/*
 * Start a specified timer
 *
 * When the timer expires, the callback function is called in
 * non-interrupt context. If the timer is already running when this
 * function is called, it will be stopped and restarted.
 *
 * @param[in] timerId - the ID of the timer
 * @param[in] timerType - the type of the timer
 * @param[in] timeInMilliseconds - time expressed in millisecond units
 * @param[in] pfTmrCallBack - callback function
 * @param[in] param - parameter to callback function
 *
 */
tmrErrCode_t TMR_StartTimer(
	tmrTimerID_t timerID,
	tmrTimerType_t timerType,
	tmrTimeInMilliseconds_t timeInMilliseconds,
	pfTmrCallBack_t callback,
	void *param
	)
{
	tmrErrCode_t status;

	LOG_DBG("ID: %d, ms: %d", timerID, timeInMilliseconds);

	/* Stopping an already stopped timer is harmless. */
	status = TMR_StopTimer(timerID);

	if (status == gTmrSuccess_c) {
		tmr_table[timerID].callback = callback;
		k_timer_user_data_set(&tmr_table[timerID].timer, param);
		k_timer_start(&tmr_table[timerID].timer,
			      timeInMilliseconds,
			      (timerType == gTmrIntervalTimer_c)
			      ? timeInMilliseconds : 0);
	}

	return status;
}

tmrErrCode_t TMR_StartLowPowerTimer(
    tmrTimerID_t timerID, tmrTimerType_t timerType, uint32_t timeInMilliseconds, pfTmrCallBack_t callback, void *param)
{
    return (tmrErrCode_t)TMR_StartTimer(timerID, timerType, timeInMilliseconds, callback, (void *)param);
}

/*
 * -----------------------------------------------------------------------------
 *
 * Crypto abstractions
 *
 * -----------------------------------------------------------------------------
 */

/*
 * N.B.:
 * There is a separate PR being worked to abstract the crypto security to allow
 * using tinycrypt or mbedTLS. For now, we will use tinycrypt and switch over
 * when the other PR is completed.
 */
#include <tinycrypt/ccm_mode.h>
#include <tinycrypt/ecc_dh.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>


/*
 * Generate a random number
 *
 * @param[out] pRandomNo - storage location for randome number
 *
 */
void RNG_GetRandomNo(uint32_t *pRandomNo)
{
	if (pRandomNo != NULL) {
		uint32_t n = sys_rand32_get();

		FLib_MemCpy(pRandomNo, &n, sizeof(uint32_t));
	}
}


/* CCM operation */
#define gSecLib_CCM_Encrypt_c 0
#define gSecLib_CCM_Decrypt_c 1

union ecdhPrivateKey {
	uint8_t raw_8bit[32];
	uint32_t raw_32bit[8];
};

PACKED_UNION ecdhPoint {
	uint8_t raw[64];

	PACKED_STRUCT {
		uint8_t x[32];
		uint8_t y[32];
	} components_8bit;
	PACKED_STRUCT {
		uint32_t x[8];
		uint32_t y[8];
	} components_32bit;
};

typedef PACKED_UNION ecdhPoint ecdhPublicKey_t;
typedef PACKED_UNION ecdhPoint ecdhDhKey_t;

enum secResultType {
	gSecSuccess_c,
	gSecAllocError_c,
	gSecError_c
};

/*
 * This function performs AES-128 encryption on a 16-byte block.
 *
 * @param[in]  pInput Pointer to the location of the 16-byte plain text block.
 * @param[in]  pKey Pointer to the location of the 128-bit key.
 * @param[out] pOutput Pointer to the location to store the 16-byte ciphered
 *             output.
 *
 * @pre All Input/Output pointers must refer to a memory address aligned to 4
 *      bytes!
 *
 */
void AES_128_Encrypt(
	const uint8_t *pInput,
	const uint8_t *pKey,
	uint8_t *pOutput)
{
	struct tc_aes_key_sched_struct s;

	LOG_DBG("");
	LOG_HEXDUMP_DBG(pKey, 16, "key:");
	LOG_HEXDUMP_DBG(pInput, 16, "plaintext: ");

	if (tc_aes128_set_encrypt_key(&s, pKey) == TC_CRYPTO_FAIL) {
		LOG_ERR("error setting encryption key???");
		return;
	}

	if (tc_aes_encrypt(pOutput, pInput, &s) == TC_CRYPTO_FAIL) {
		LOG_ERR("error encrypting???");
		return;
	}

	LOG_HEXDUMP_DBG(pOutput, 16, "pOutput: ");
}

/*
 * This function performs AES-128-CCM on a message block.
 *
 * @param[in]  pInput       Pointer to the location of the input message
 *                          (plaintext or cyphertext).
 * @param[in]  inputLen     Length of the input plaintext in bytes when
 *                          encrypting.
 *                          Length of the input cypertext without the
 *                          MAC length when decrypting.
 * @param[in]  pAuthData    Pointer to the additional authentication data.
 * @param[in]  authDataLen  Length of additional authentication data.
 * @param[in]  pNonce       Pointer to the Nonce.
 * @param[in]  nonceSize    The size of the nonce (7-13).
 * @param[in]  pKey         Pointer to the location of the 128-bit key.
 * @param[out]  pOutput     Pointer to the location to store the
 *                          plaintext data when decrypting.
 *                          Pointer to the location to store the
 *                          cyphertext data when encrypting.
 * @param[out]  pCbcMac     Pointer to the location to store the Message
 *                          Authentication Code (MAC) when encrypting.
 *                          Pointer to the location where the received
 *                          MAC can be found when decrypting.
 * @param[out]  macSize     The size of the MAC.
 * @param[out]  flags       Select encrypt/decrypt operations
 *                          (gSecLib_CCM_Encrypt_c, gSecLib_CCM_Decrypt_c)
 *
 * @return 0 if encryption/decryption was successful; otherwise, error
 *           code for failed encryption/decryption
 *
 * @remarks At decryption, MIC fail is also signaled by returning a
 *          non-zero value.
 */
uint8_t AES_128_CCM(uint8_t *pInput,
		    uint16_t inputLen,
		    uint8_t *pAuthData,
		    uint16_t authDataLen,
		    uint8_t *pNonce,
		    uint8_t nonceSize,
		    uint8_t *pKey,
		    uint8_t *pOutput,
		    uint8_t *pCbcMac,
		    uint8_t macSize,
		    uint32_t flags)
{
	LOG_DBG("Entry(%x)", (unsigned int)flags);
	LOG_HEXDUMP_DBG(pKey, 16, "key:");

	/*
	 * N.B: A bit of a cheat in knowledge here but the underlying usage of
	 * this function puts the MAC (pCbcMac) at the end of the buffer) so we
	 * can ignore this paramenter for now.
	 */

	struct tc_ccm_mode_struct c;
	struct tc_aes_key_sched_struct sched;
	int result;


	tc_aes128_set_encrypt_key(&sched, pKey);
	result = tc_ccm_config(&c, &sched, pNonce, nonceSize, macSize);
	if (result == TC_CRYPTO_FAIL) {
		LOG_ERR("TC internal error during CCM config OP");
	}

	/* TESTPOINT: Check CCM config */
	/* zassert_true(result, "CCM config failed"); */

	if (flags & gSecLib_CCM_Decrypt_c) {
		result = tc_ccm_decryption_verification(
			pOutput,
			inputLen,
			pAuthData,
			authDataLen,
			pInput,
			inputLen + macSize,
			&c);
		if (result == TC_CRYPTO_FAIL) {
			LOG_ERR("TC internal error during CCM decryption OP");
		}
	} else {
		/*
		 * N.B.: out buffer should be at least (plen + c->mlen) bytes
		 * long... The tiny crypt library appends the mac to the end of
		 * the encrypted data.
		 */
		result = tc_ccm_generation_encryption(
			pOutput,
			inputLen + macSize,
			pAuthData,
			authDataLen,
			pInput,
			inputLen,
			&c);

		if (result == TC_CRYPTO_FAIL) {
			LOG_ERR("TC internal error during CCM encryption OP");
		}
	}
	LOG_DBG("Exit: %d", result);
	/* Tinycrypt returns 1 on success while the SecLib returns 0. */
	return (result == TC_CRYPTO_FAIL) ? 1 : 0;

}

enum secResultType ECDH_P256_GenerateKeys(
	ecdhPublicKey_t *pOutPublicKey,
	union ecdhPrivateKey *pOutPrivateKey
	)
{
	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		if (uECC_make_key((uint8_t *)pOutPublicKey,
				  (uint8_t *)pOutPrivateKey,
				  &curve_secp256r1)) {
			return gSecSuccess_c;
		}
	}

	LOG_ERR("error?!?!?");
	return gSecError_c;
}

enum secResultType ECDH_P256_ComputeDhKey(
	union ecdhPrivateKey *pPrivateKey,
	ecdhPublicKey_t *pPeerPublicKey,
	ecdhDhKey_t *pOutDhKey
	)
{
	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		if (uECC_shared_secret((const uint8_t *)pPeerPublicKey,
				       (const uint8_t *)pPrivateKey,
				       (uint8_t *)pOutDhKey,
				       &curve_secp256r1)) {
			return gSecSuccess_c;
		}
	}

	LOG_ERR("error?!?!?");
	return gSecError_c;
}
