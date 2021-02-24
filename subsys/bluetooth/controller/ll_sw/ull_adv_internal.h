/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ULL_ADV_RANDOM_DELAY HAL_TICKER_US_TO_TICKS(10000)

/* Bitmask value returned by ull_adv_is_enabled() */
#define ULL_ADV_ENABLED_BITMASK_ENABLED  BIT(0)

/* Helper functions to initialise and reset ull_adv module */
int ull_adv_init(void);
int ull_adv_reset(void);
int ull_adv_reset_finalize(void);

/* Return ll_adv_set context (unconditional) */
struct ll_adv_set *ull_adv_set_get(uint8_t handle);

/* Return the adv set handle given the adv set instance */
uint16_t ull_adv_handle_get(struct ll_adv_set *adv);

/* Return ll_adv_set context if enabled */
struct ll_adv_set *ull_adv_is_enabled_get(uint8_t handle);

/* Return enabled status of a set */
int ull_adv_is_enabled(uint8_t handle);

/* Return filter policy used */
uint32_t ull_adv_filter_pol_get(uint8_t handle);

/* Return ll_adv_set context if created */
struct ll_adv_set *ull_adv_is_created_get(uint8_t handle);

/* Helper function to construct AD data */
uint8_t ull_adv_data_set(struct ll_adv_set *adv, uint8_t len,
			 uint8_t const *const data);

/* Helper function to construct SR data */
uint8_t ull_scan_rsp_set(struct ll_adv_set *adv, uint8_t len,
			 uint8_t const *const data);

/* Update AdvA and TgtA (if application) in advertising PDU */
const uint8_t *ull_adv_pdu_update_addrs(struct ll_adv_set *adv,
					struct pdu_adv *pdu);

#if defined(CONFIG_BT_CTLR_ADV_EXT)

#define ULL_ADV_PDU_HDR_FIELD_ADVA      BIT(0)
#define ULL_ADV_PDU_HDR_FIELD_CTE_INFO  BIT(2)
#define ULL_ADV_PDU_HDR_FIELD_AUX_PTR   BIT(4)
#define ULL_ADV_PDU_HDR_FIELD_SYNC_INFO BIT(5)
#define ULL_ADV_PDU_HDR_FIELD_AD_DATA   BIT(8)

/* Helper type to store data for extended advertising
 * header fields and extra data.
 */
struct adv_pdu_field_data {
	uint8_t *field_data;

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	void *extra_data;
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
};

/* helper function to handle adv done events */
void ull_adv_done(struct node_rx_event_done *done);

/* Helper functions to initialise and reset ull_adv_aux module */
int ull_adv_aux_init(void);
int ull_adv_aux_reset(void);

/* Return the aux set handle given the aux set instance */
uint8_t ull_adv_aux_handle_get(struct ll_adv_aux_set *aux);

/* Helper to read back random address */
uint8_t const *ll_adv_aux_random_addr_get(struct ll_adv_set const *const adv,
				       uint8_t *const addr);

/* helper function to initialize event timings */
uint32_t ull_adv_aux_evt_init(struct ll_adv_aux_set *aux);

/* helper function to start auxiliary advertising */
uint32_t ull_adv_aux_start(struct ll_adv_aux_set *aux, uint32_t ticks_anchor,
			   uint32_t ticks_slot_overhead);

/* helper function to stop auxiliary advertising */
uint8_t ull_adv_aux_stop(struct ll_adv_aux_set *aux);

/* helper function to acquire and initialize auxiliary advertising instance */
struct ll_adv_aux_set *ull_adv_aux_acquire(struct lll_adv *lll);

/* helper function to release auxiliary advertising instance */
void ull_adv_aux_release(struct ll_adv_aux_set *aux);

/* helper function to schedule a mayfly to get aux offset */
void ull_adv_aux_offset_get(struct ll_adv_set *adv);

/* helper function to set/clear common extended header format fields */
uint8_t ull_adv_aux_hdr_set_clear(struct ll_adv_set *adv,
				  uint16_t sec_hdr_add_fields,
				  uint16_t sec_hdr_rem_fields,
				  void *value,
				  struct pdu_adv_adi *adi,
				  uint8_t *pri_idx);

/* helper function to release periodic advertising instance */
void ull_adv_sync_release(struct ll_adv_sync_set *sync);

/* helper function to set/clear common extended header format fields
 * for AUX_SYNC_IND PDU.
 */
uint8_t ull_adv_sync_pdu_set_clear(struct ll_adv_set *adv,
				   uint16_t hdr_add_fields,
				   uint16_t hdr_rem_fields,
				   struct adv_pdu_field_data *data,
				   uint8_t *ter_idx);

/* helper function to calculate common ext adv payload header length and
 * adjust the data pointer.
 * NOTE: This function reverts the header data pointer if there is no
 *       header fields flags set, and hence no header fields have been
 *       populated.
 */
static inline uint8_t
ull_adv_aux_hdr_len_calc(struct pdu_adv_com_ext_adv *com_hdr, uint8_t **dptr)
{
	uint8_t len;

	len = *dptr - (uint8_t *)com_hdr;
	if (len <= (PDU_AC_EXT_HEADER_SIZE_MIN +
		    sizeof(struct pdu_adv_ext_hdr))) {
		len = PDU_AC_EXT_HEADER_SIZE_MIN;
		*dptr = (uint8_t *)com_hdr + len;
	}

	return len;
}

/* helper function to fill common ext adv payload header length */
static inline void
ull_adv_aux_hdr_len_fill(struct pdu_adv_com_ext_adv *com_hdr, uint8_t len)
{
	com_hdr->ext_hdr_len = len - PDU_AC_EXT_HEADER_SIZE_MIN;

}

/* helper function to fill the aux ptr structure in common ext adv payload */
void ull_adv_aux_ptr_fill(uint8_t **dptr, uint8_t phy_s);

int ull_adv_sync_init(void);
int ull_adv_sync_reset(void);

/* helper function to start periodic advertising */
uint32_t ull_adv_sync_start(struct ll_adv_set *adv,
			    struct ll_adv_sync_set *sync,
			    uint32_t ticks_anchor);

/* helper function to update periodic advertising event length */
void ull_adv_sync_update(struct ll_adv_sync_set *sync, uint32_t slot_plus_us,
			 uint32_t slot_minus_us);

/* helper function to schedule a mayfly to get sync offset */
void ull_adv_sync_offset_get(struct ll_adv_set *adv);

int ull_adv_iso_init(void);
int ull_adv_iso_reset(void);

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* helper function to release unused DF configuration memory */
void ull_df_adv_cfg_release(struct lll_df_adv_cfg *df_adv_cfg);
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#endif /* CONFIG_BT_CTLR_ADV_EXT */
