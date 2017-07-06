#ifndef _SDMMC_H__
#define _SDMMC_H__

#include "global_macros.h"

#define SDMMC_TEST_ENABLE 1

/* =============================================================================
 * TYPES
 * =============================================================================
 */

typedef struct {
	uint8_t mid;
	uint32_t oid;
	uint32_t pnm1;
	uint8_t pnm2;
	uint8_t prv;
	uint32_t psn;
	uint32_t mdt;
	uint8_t crc;
} mcd_cid_format_t;

/* =============================================================================
 * mcd_card_state_t
 * -----------------------------------------------------------------------------
 * The state of the card when receiving the command. If the command execution
 * causes a state change, it will be visible to the host in the response to
 * the next command. The four bits are interpreted as a binary coded number
 * between 0 and 15.
 * =============================================================================
 */
typedef enum {
	MCD_CARD_STATE_IDLE = 0,
	MCD_CARD_STATE_READY = 1,
	MCD_CARD_STATE_IDENT = 2,
	MCD_CARD_STATE_STBY = 3,
	MCD_CARD_STATE_TRAN = 4,
	MCD_CARD_STATE_DATA = 5,
	MCD_CARD_STATE_RCV = 6,
	MCD_CARD_STATE_PRG = 7,
	MCD_CARD_STATE_DIS = 8
} mcd_card_state_t;

/* =============================================================================
 * MCD_CARD_STATUS_T
 * -----------------------------------------------------------------------------
 * Card status as returned by R1 reponses (spec V2 pdf p.)
 * =============================================================================
 */
typedef union {
	uint32_t reg;
	struct {
		uint32_t:3;
		uint32_t ake_seq_error:1;
		 uint32_t:1;
		uint32_t app_cmd:1;
		 uint32_t:2;
		uint32_t ready_for_data:1;
		mcd_card_state_t current_state:4;
		uint32_t erase_reset:1;
		uint32_t card_ecc_disabled:1;
		uint32_t wp_erase_skip:1;
		uint32_t csd_over_write:1;
		 uint32_t:2;
		uint32_t error:1;
		uint32_t cc_error:1;
		uint32_t card_ecc_failed:1;
		uint32_t illegal_command:1;
		uint32_t com_crc_error:1;
		uint32_t lock_unlock_fail:1;
		uint32_t card_is_locked:1;
		uint32_t wp_violation:1;
		uint32_t erase_param:1;
		uint32_t erase_seq_error:1;
		uint32_t block_len_error:1;
		uint32_t address_error:1;
		uint32_t out_of_range:1;
	} fields;
} mcd_card_status_t;

/* =============================================================================
 * MCD_CSD_T
 * -----------------------------------------------------------------------------
 * This structure contains the fields of the MMC chip's register.
 * For more details, please refer to your MMC specification.
 * =============================================================================
 */
struct mcd_csd_reg {			/* ver 2. | ver 1.0 (if different) */
	uint8_t csd_structure;		/* 127:126 */
	uint8_t spec_vers;		/* 125:122 */
	uint8_t taac;			/* 119:112 */
	uint8_t nsac;			/* 111:104 */
	uint8_t tran_speed;		/* 103:96 */
	uint16_t ccc;			/* 95:84 */
	uint8_t read_bl_len;		/* 83:80 */
	uint8_t read_bl_partial;	/* 79:79 */
	uint8_t write_blk_misalign;	/* 78:78 */
	uint8_t read_blk_misalign;	/* 77:77 */
	uint8_t dsr_imp;		/* 76:76 */
	uint16_t c_size;		/* 69:48 | 73:62 */
	uint8_t vdd_r_curr_min;		/* 61:59 */
	uint8_t vdd_r_curr_max;		/* 58:56 */
	uint8_t vdd_w_curr_min;		/* 55:53 */
	uint8_t vdd_w_curr_max;		/* 52:50 */
	uint8_t c_size_mult;		/* 49:47 */

	uint8_t erase_blk_enable;
	uint8_t erase_grp_size;		/* 46:42 */
	uint8_t sector_size;
	uint8_t erase_grp_mult;		/* 41:37 */

	uint8_t wp_grp_size;		/* 38:32 */
	uint8_t wp_grp_enable;		/* 31:31 */
	uint8_t default_ecc;		/* 30:29 */
	uint8_t r2w_factor;		/* 28:26 */
	uint8_t write_bl_len;		/* 25:22 */
	uint8_t write_bl_partial;	/* 21:21 */
	uint8_t content_prot_app;	/* 16:16 */
	uint8_t file_format_grp;	/* 15:15 */
	uint8_t copy;			/* 14:14 */
	uint8_t perm_write_protect;	/* 13:13 */
	uint8_t tmp_write_protect;	/* 12:12 */
	uint8_t file_format;		/* 11:10 */
	uint8_t ecc;			/* 9:8 */
	uint8_t crc;			/* 7:1 */
	/* this field is not from the csd register. */
	/* this is the actual block number. */
	uint32_t block_number;
};

#undef bool

/* =============================================================================
 * mcd_err_t
 * -----------------------------------------------------------------------------
 * Type used to describe the error status of the MMC driver.
 * =============================================================================
 */
typedef enum {
	MCD_ERR_NO = 0,
	MCD_ERR_CARD_TIMEOUT = 1,
	MCD_ERR_DMA_BUSY = 3,
	MCD_ERR_CSD = 4,
	MCD_ERR_SPI_BUSY = 5,
	MCD_ERR_BLOCK_LEN = 6,
	MCD_ERR_CARD_NO_RESPONSE,
	MCD_ERR_CARD_RESPONSE_BAD_CRC,
	MCD_ERR_CMD,
	MCD_ERR_UNUSABLE_CARD,
	MCD_ERR_NO_CARD,
	MCD_ERR_NO_HOTPLUG,
	MCD_ERR,
} mcd_err_t;

/* =============================================================================
 * MCD_STATUS_T
 * -----------------------------------------------------------------------------
 * Status of card
 * =============================================================================
 */
typedef enum {
	/* Card present and mcd is open */
	MCD_STATUS_OPEN,
	/* Card present and mcd is not open */
	MCD_STATUS_NOTOPEN_PRESENT,
	/* Card not present */
	MCD_STATUS_NOTPRESENT,
	/* Card removed, still open (please close !) */
	MCD_STATUS_OPEN_NOTPRESENT
} mcd_status_t;

/* =============================================================================
 * mcd_card_size_t
 * -----------------------------------------------------------------------------
 * Card size
 * =============================================================================
 */
typedef struct {
	uint32_t nbBlock;
	uint32_t blockLen;
} mcd_card_size_t;

/* =============================================================================
 * MCD_CARD_VER
 * -----------------------------------------------------------------------------
 * Card version
 * =============================================================================
 */

typedef enum {
	MCD_CARD_V1,
	MCD_CARD_V2
} mcd_card_ver_t;

/* =============================================================================
 * mcd_card_id_t
 * -----------------------------------------------------------------------------
 * Card version
 * =============================================================================
 */

typedef enum {
	MCD_CARD_ID_0,
	MCD_CARD_ID_1,
	MCD_CARD_ID_NO,
} mcd_card_id_t;

#ifdef __cplusplus
extern "C" {
#endif

	void rda_sdmmc_init(unsigned int d0);
	int rda_sdmmc_open(void);
	int rda_sdmmc_read_blocks(uint8_t * buffer, uint32_t block_start,
				  uint32_t block_num);
	int rda_sdmmc_write_blocks(const uint8_t * buffer, uint32_t block_start,
				   uint32_t block_num);
	int rda_sdmmc_get_csdinfo(uint32_t * block_size);
#if SDMMC_TEST_ENABLE
	void rda_sdmmc_test(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* _SDMMC_H_ */
