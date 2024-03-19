/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright (c) 2024 Laczen
 */

/**
 * @brief Non-volatile circular buffer (NVCB)
 * @defgroup nvcb Non-volatile circular buffer (NVCB)
 * @since
 * @version 0.0.1
 * @ingroup storage_apis
 * @{
 *
 * Generic non volatile circular buffer to store and retrieve data on
 * different kind of memory devices e.g. RAM, FLASH, EEPROM, ...
 *
 * nvcb stores data as sequential cbor items in blocks that have a configurable
 * size. The cbor encoding/decoding is limited to reduce code space, the
 * following limitations apply:
 *
 * Each block starts with the tag 55799 (0xD9 0xD9 0xF7) stating that the data
 * is cbor. This tag is followed by a 4 byte cbor byte string (cbor tag 0x44)
 * indicating that this is a nvcb block (string "NVCB"). Then a 1 byte integer
 * is used as a pass indicator (this allows to determine the newest block). The
 * pass indicator is toggled from 0x00 to 0x01 (or vice versa) each time the
 * backend memory wraps around.
 * Each block thus starts with the block header:
 *      0xD9 0xD9 0xF7 0x44 0x4E 0x56 0x43 0x42 0x00/0x01.
 * The block header is immediately followed by a first item that is represented
 * by a cbor byte string (with 2 byte length): 0x59 0xYY 0xYY where 0xYY 0xYY
 * is the item length in big endian format.
 * The item is aligned to the write block size by padding with 0xF7 (cbor
 * undefined).
 *
 * Any binary data found after an item that is not 0x59 or 0xF7 is treated as
 * the end of the block.
 *
 * Adding entries is done by calling `nvcb_append()` or `nvcb_write()`.
 * When there is insufficient space in the current block to add the data
 * `nvcb_append()/nvcb_write()` returns -NVCB_ENOSPC. In that case the user
 * should advance the nvcb using `nvcb_advance()`.
 *
 * Before writing a cbor header nvcb verifies that the data that is found at the
 * next writing location is a valid entry type. If a valid entry type is found a
 * break character (0xFF) is written to the next writing location. This method
 * enables support for memory devices that do not have erase functionality.
 *
 * The configurable block size limits the maximum size of an entry as it needs
 * to fit within one block. The block size is not limited to an erase block size
 * of the memory device, this allows using memory devices with non constant
 * erase block sizes.
 *
 * @}
 */

#ifndef NVCB_H_
#define NVCB_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NVCB_FILLCHAR  0xF7   /* cbor undefined */
#define NVCB_BREAKCHAR 0xFF   /* cbor atom break */
#define NVCB_ECBORHDR  0x59   /* entry header: cbor byte string 2 byte length */
#define NVCB_EHDRSIZE  3      /* entry header size */
#define NVCB_EMAXLEN   0xFFFF /* entry max length: 2 byte */
#define NVCB_BCBORHDR  "\xD9\xD9\xF7\x44NVCB"
#define NVCB_BHDRSIZE  sizeof(NVCB_BCBORHDR)
#define NVCB_BUFSIZE   16

/**
 * @brief Non-volatile circular buffer Return Codes
 * @defgroup nvcb_return_codes Non-volatile circular buffer Return Codes
 * @ingroup nvcb
 * @{
 */

enum nvcb_rc {
	NVCB_SKIP = 2,         /**< Skip block while walking */
	NVCB_DONE = 1,         /**< Stop walking */
	NVCB_OK = 0,           /**< No error, */
	NVCB_ENOENT = -ENOENT, /**< No such entry */
	NVCB_EAGAIN = -EAGAIN, /**< No more contexts */
	NVCB_EINVAL = -EINVAL, /**< Invalid argument */
	NVCB_ENOSPC = -ENOSPC, /**< No space left on device */
	NVCB_ECORRUPT = -84,   /**< Corrupt block on device */
};

/**
 * @}
 */

/**
 * @brief Non-volatile circular buffer Data Structures
 * @defgroup nvcb_data_structures Non-volatile circular buffer Data Structures
 * @ingroup nvcb
 * @{
 */

struct nvcb_store;

/**
 * @brief nvcb entry structure
 *
 */
struct nvcb_ent {
	const struct nvcb_store *store; /**< pointer to the nvcb */
	uint32_t blck;                  /**< block the entry belongs to */
	uint32_t dpos;                  /**< data start position in block */
	uint32_t dsz;                   /**< data size */
};

/**
 * @brief nvcb store configuration definition
 *
 */
struct nvcb_store_cfg {
	const void *ctx;     /**< opaque context pointer */
	const size_t psz;    /**< prog buffer size (byte), power of 2! */
			     /**< the write alignment */
	const size_t bsz;    /**< block size (byte), power of 2! */
	const uint32_t bcnt; /**< block count */

	/**
	 * @brief read from memory device
	 *
	 * @param[in] ctx pointer to memory context
	 * @param[in] start starting address
	 * @param[in] data pointer to buffer to place read data
	 * @param[in] len number of bytes
	 *
	 * @return 0 on success, negative error code on error. -NVCB_ECORRUPT
	 * can be used to signal a bad block.
	 */
	int (*read)(const void *ctx, size_t start, void *data, size_t len);

	/**
	 * @brief program memory device (optional)
	 *
	 * @param[in] ctx pointer to memory context
	 * @param[in] start starting address
	 * @param[in] data pointer to data to be written
	 * @param[in] len number of bytes
	 *
	 * @return 0 on success, negative error code on error. -NVCB_ECORRUPT
	 * can be used to signal a bad block.
	 */
	int (*prog)(const void *ctx, size_t start, const void *data, size_t len);

	/**
	 * @brief prepare memory device (optional)
	 *
	 * Prepare memory device for writing at specified location. This can be
	 * used to erase a memory device (when required). The memory should be
	 * prepared for at least len byte (but can be larger).
	 *
	 * @param[in] ctx pointer to memory context
	 * @param[in] start starting address
	 * @param[in] len number of bytes
	 *
	 * @return 0 on success, negative error code on error. -NVCB_ECORRUPT
	 * can be used to signal a bad block.
	 */
	int (*prep)(const void *ctx, size_t start, size_t len);

	/**
	 * @brief memory device sync (optional)
	 *
	 * @param[in] ctx pointer to memory context
	 *
	 * @return 0 on success, error is propagated to user
	 */
	int (*sync)(const void *ctx);

	/**
	 * @brief memory device init function
	 *
	 * @param[in] ctx pointer to memory context
	 *
	 * @return 0 on success, error is propagated to user
	 */
	int (*init)(const void *ctx);

	/**
	 * @brief memory device release function
	 *
	 * @param[in] ctx pointer to memory context
	 *
	 * @return 0 on success, error is propagated to user
	 */
	int (*release)(const void *ctx);

	/**
	 * @brief os provided lock function
	 *
	 * @param[in] ctx pointer to memory context
	 *
	 * @return 0 on success, error is propagated to user
	 */
	int (*lock)(const void *ctx);

	/**
	 * @brief os provided unlock function
	 *
	 * @param[in] ctx pointer to memory context
	 *
	 * @return 0 on success, error is ignored
	 */
	int (*unlock)(const void *ctx);
};

/**
 * @brief nvcb store data structure
 *
 */
struct nvcb_store_data {
	uint32_t cblck;              /**< current memory (write) block */
	uint32_t cpos;               /**< current memory (write) position */
	bool ready;                  /**< state */
	uint8_t pass;		     /**< current pass */
};

/**
 * @brief nvcb structure
 *
 */
struct nvcb_store {
	const struct nvcb_store_cfg *cfg;
	struct nvcb_store_data *data;
};

/**
 * @}
 */

/**
 * @brief Non-volatile circular buffer Macros
 * @defgroup nvcb_macros Non-volatile circular buffer Macros
 * @ingroup nvcb
 * @{
 */

/**
 * @brief Helper macro to define a nvcb
 *
 */
#define DEFINE_NVCB(_name, _ctx, _psz, _bsz, _bcnt, _read, _prog, _prep, _sync, \
		    _init, _release, _lock, _unlock)                            \
	BUILD_ASSERT(_psz != 0, "incorrect program size");                      \
	BUILD_ASSERT((_psz & (_psz - 1)) == 0, "incorrect program size");       \
	BUILD_ASSERT((_bsz % _psz) == 0,                                        \
		     "block size not a multiple of program size");		\
	BUILD_ASSERT((NVCB_BUFSIZE % _psz) == 0,                                \
		     "NVCB_BUFSIZE not a multiple of program size");            \
	static struct nvcb_store_cfg nvcb_store_cfg_##_name = {                 \
		.ctx = _ctx,                                                    \
		.psz = _psz,                                                    \
		.bsz = _bsz,                                                    \
		.bcnt = _bcnt,                                                  \
		.read = _read,                                                  \
		.prog = _prog,                                                  \
		.prep = _prep,                                                  \
		.sync = _sync,                                                  \
		.init = _init,                                                  \
		.release = _release,                                            \
		.lock = _lock,                                                  \
		.unlock = _unlock,                                              \
	};                                                                      \
	static struct nvcb_store_data nvcb_store_data_##_name;                  \
	static struct nvcb_store nvcb_store_##_name = {                         \
		.cfg = &nvcb_store_cfg_##_name,                                 \
		.data = &nvcb_store_data_##_name,                               \
	}

/**
 * @brief Helper macro to get a pointer to a nvcb structure
 *
 */
#define nvcb_store_get(_name) &nvcb_store_##_name

/**
 * @}
 */

/**
 * @brief Non-volatile circular buffer APIs
 * @defgroup nvcb_api Non-volatile circular buffer APIs
 * @ingroup nvcb
 * @{
 */

/**
 * @brief mount the nvcb store
 *
 * @param[in] store pointer to the nvcb
 *
 * @return 0 on success, negative errorcode on error
 */
int nvcb_mount(const struct nvcb_store *store);

/**
 * @brief unmount the nvcb store and releases the memory backend
 *
 * @param[in] store pointer to the nvcb
 *
 * @return 0 on success, negative errorcode on error
 */
int nvcb_unmount(const struct nvcb_store *store);

/**
 * @brief advance the nvcb, this starts a new sector for appending.
 *
 * @param[in] store pointer to the nvcb
 *
 * @return 0 on success, negative errorcode on error
 */
int nvcb_advance(const struct nvcb_store *store);

/**
 * @brief append data to the nvcb store
 *
 * @param[in] store nvcb store
 * @param[in] read data read callback routine
 * @param[in] rdctx data read callbac context
 * @param[in] len data size
 *
 * @return 0 on success, negative errorcode on error. Errorcode -NVCB_ENOSPC or
 * -NVCB_ECORRUPT indicates that `nvcb_advance(const struct nvcb_store *store)`
 * should be used to start using a next block.
 */
int nvcb_append(const struct nvcb_store *store,
		int (*read)(const void *rdctx, uint32_t off, void *data,
			    size_t len),
		const void *rdctx, size_t len);

/**
 * @brief write data to the nvcb store
 *
 * @param[in] store nvcb store
 * @param[in] data
 * @param[in] len data length (bytes)
 *
 * @return 0 on success, negative errorcode on error. Errorcode -NVCB_ENOSPC or
 * -NVCB_ECORRUPT indicates that `nvcb_advance(const struct nvcb_store *store)`
 * should be used to start using a next block.
 */
int nvcb_write(const struct nvcb_store *store, const void *data, size_t len);

/**
 * @brief walk forward over blocks of the nvcb and for each entry in a block
 * issue a callback. Walking is stopped when the callback returns NVCB_DONE or
 * an errorcode. Walking skips to the next block when the callback returns
 * NVCB_SKIP.
 *
 * @param[in] store nvcb store
 * @param[in] cb callback routine
 * @param[in] cb_arg callback routine argument
 *
 * @return 0 on success, negative errorcode on error
 */
int nvcb_walk_forward(const struct nvcb_store *store,
		      int (*cb)(const struct nvcb_ent *ent, void *cb_arg),
		      void *cb_arg);

/**
 * @brief get the start entry for iterating with nvcb_get_next(), the
 * parameter skip can be used to advance blocks at the start.
 *
 * @param[out] ent nvcb entry
 * @param[in] store nvcb store
 * @param[in] bskip number of blocks to skip from nvcb store start
 *
 * @return 0 on success, negative errorcode on error
 */
int nvcb_get_start(struct nvcb_ent *ent, const struct nvcb_store *store,
		   uint32_t bskip);

/**
 * @brief get the next entry in a nvcb store, the parameter blimit can be used
 * to limit the amount of blocks to iterate.
 *
 * @param[in,out] ent nvcb entry
 * @param[in] blimit limit on the number of blocks
 *
 * @return 0 on success, NVCB_ENOENT is returned when the end is reached,
 * negative errorcode on error
 */
int nvcb_get_next(struct nvcb_ent *ent, uint32_t *blimit);

/**
 * @brief read data from an entry at offset
 *
 * @param[in] ent pointer to the entry
 * @param[in] off offset
 * @param[out] data
 * @param[in] len value length (bytes)
 *
 * @return 0 on success, negative errorcode on error
 */
int nvcb_entry_read(const struct nvcb_ent *ent, uint32_t off, void *data,
		    size_t len);

/**
 * @brief secure wipe the nvcb, should be called on a unmounted nvcb store.
 * Overwrites the memory backend with a preset value (NVCB_FILLCHAR) so that it
 * is sure that no data is left in the memory backend.
 *
 * @param[in] store pointer to the nvcb
 *
 * @return 0 on success, negative errorcode on error
 */
int nvcb_secure_wipe(const struct nvcb_store *store);
/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NVCB_H_ */
