/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ULL_ADV_RANDOM_DELAY HAL_TICKER_US_TO_TICKS(10000)

/* Bitmask value returned by ull_adv_is_enabled() */
#define ULL_ADV_ENABLED_BITMASK_ENABLED  BIT(0)

#if defined(CONFIG_BT_CTLR_ADV_SET)
#define BT_CTLR_ADV_SET CONFIG_BT_CTLR_ADV_SET
#else /* CONFIG_BT_CTLR_ADV_SET */
#define BT_CTLR_ADV_SET 1
#endif /* CONFIG_BT_CTLR_ADV_SET */

/* Helper functions to initialise and reset ull_adv module */
int ull_adv_init(void);
int ull_adv_reset(void);

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


#if defined(CONFIG_BT_CTLR_ADV_EXT)

#define ULL_ADV_PDU_HDR_FIELD_ADVA      BIT(0)
#define ULL_ADV_PDU_HDR_FIELD_SYNC_INFO BIT(5)
#define ULL_ADV_PDU_HDR_FIELD_AD_DATA   BIT(8)

/* helper function to handle adv done events */
void ull_adv_done(struct node_rx_event_done *done);

/* Helper functions to initialise and reset ull_adv_aux module */
int ull_adv_aux_init(void);
int ull_adv_aux_reset(void);

/* Helper to read back random address */
uint8_t const *ll_adv_aux_random_addr_get(struct ll_adv_set const *const adv,
				       uint8_t *const addr);

/* helper function to initialize event timings */
uint32_t ull_adv_aux_evt_init(struct ll_adv_aux_set *aux);

/* helper function to start auxiliary advertising */
uint32_t ull_adv_aux_start(struct ll_adv_aux_set *aux, uint32_t ticks_anchor,
			   uint32_t ticks_slot_overhead,
			   uint32_t volatile *ret_cb);

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
				  void *value, struct pdu_adv_adi *adi);

/* helper function to calculate common ext adv payload header length */
static inline uint8_t
ull_adv_aux_hdr_len_get(struct pdu_adv_com_ext_adv *com_hdr, uint8_t *dptr)
{
	uint8_t len;

	len = dptr - (uint8_t *)com_hdr;
	if (len <= (offsetof(struct pdu_adv_com_ext_adv, ext_hdr_adi_adv_data) +
		    sizeof(struct pdu_adv_hdr))) {
		len = offsetof(struct pdu_adv_com_ext_adv,
			       ext_hdr_adi_adv_data);
	}

	return len;
}

/* helper function to fill common ext adv payload header length */
static inline void
ull_adv_aux_hdr_len_fill(struct pdu_adv_com_ext_adv *com_hdr, uint8_t len)
{
	com_hdr->ext_hdr_len = len - offsetof(struct pdu_adv_com_ext_adv,
					      ext_hdr_adi_adv_data);

}

/* helper function to fill the aux ptr structure in common ext adv payload */
void ull_adv_aux_ptr_fill(uint8_t **dptr, uint8_t phy_s);

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
int ull_adv_sync_init(void);
int ull_adv_sync_reset(void);

/* helper function to start periodic advertising */
uint32_t ull_adv_sync_start(struct ll_adv_sync_set *sync, uint32_t ticks_anchor,
			 uint32_t volatile *ret_cb);

/* helper function to schedule a mayfly to get sync offset */
void ull_adv_sync_offset_get(struct ll_adv_set *adv);
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
