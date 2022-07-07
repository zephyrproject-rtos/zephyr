/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline void feature_unmask_features(struct ll_conn *conn, uint64_t ll_feat_mask)
{
	conn->llcp.fex.features_used &= ~ll_feat_mask;
}

static inline bool feature_le_encryption(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_LE_ENC)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_ENC;
#else
	return 0;
#endif
}

static inline bool feature_conn_param_req(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_CONN_PARAM_REQ;
#else
	return 0;
#endif
}

static inline bool feature_ext_rej_ind(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_EXT_REJ_IND)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_EXT_REJ_IND;
#else
	return 0;
#endif
}

static inline bool feature_periph_feat_req(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_PER_INIT_FEAT_XCHG;
#else
	return 0;
#endif
}

static inline bool feature_le_ping(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_LE_PING)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_PING;
#else
	return 0;
#endif
}

static inline bool feature_dle(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_DLE;
#else
	return 0;
#endif
}

static inline bool feature_privacy(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_PRIVACY;
#else
	return 0;
#endif
}

static inline bool feature_ext_scan(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_EXT_SCAN;
#else
	return 0;
#endif
}

static inline bool feature_chan_sel_2(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_CHAN_SEL_2;
#else
	return 0;
#endif
}

static inline bool feature_min_used_chan(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_MIN_USED_CHAN;
#else
	return 0;
#endif
}

static inline bool feature_phy_2m(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_PHY_2M)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_PHY_2M;
#else
	return 0;
#endif
}

static inline bool feature_phy_coded(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_PHY_CODED;
#else
	return 0;
#endif
}

static inline bool feature_cte_req(struct ll_conn *conn)
{
#if defined(CONFIG_BT_CTLR_DF) && defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	return conn->llcp.fex.features_used & LL_FEAT_BIT_CONNECTION_CTE_REQ;
#else
	return 0;
#endif
}

/*
 * for asymmetric features we can check either if we support it
 * or if the peer supports it
 */
static inline bool feature_smi_rx(struct ll_conn *conn)
{
	return LL_FEAT_BIT_SMI_RX;
}

static inline bool feature_peer_smi_rx(struct ll_conn *conn)
{
	return conn->llcp.fex.features_peer & BIT64(BT_LE_FEAT_BIT_SMI_RX);
}

static inline bool feature_smi_tx(struct ll_conn *conn)
{
	return LL_FEAT_BIT_SMI_TX;
}

static inline bool feature_peer_smi_tx(struct ll_conn *conn)
{
	return conn->llcp.fex.features_peer & BIT64(BT_LE_FEAT_BIT_SMI_TX);
}

/*
 * The following features are not yet defined in KConfig and do
 * not have a bitfield defined in ll_feat.h
 * ext_adv
 * per_adv
 * pwr_class1
 * min_chann
 * ant_sw_CTE_tx
 * ant_sw_CTE_rx
 * tone_ext
 * per_adv_sync_tx
 * per_adv_sync_rx
 * sleep_upd
 * rpk_valid
 * iso_central
 * iso_periph
 * iso_broadcast
 * iso_receiver
 * iso_channels
 * le_pwr_req
 * le_pwr_ind
 * le_path_loss
 */
