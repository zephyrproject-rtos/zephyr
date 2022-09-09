/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "hal/ccm.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_conn_internal.h"

#include "ll_feat.h"
#include "ll_settings.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ll_feat
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_SET_HOST_FEATURE)
static uint64_t host_features;

uint8_t ll_set_host_feature(uint8_t bit_number, uint8_t bit_value)
{
	uint64_t feature;

	/* Check if Bit_Number is not controlled by the Host */
	feature = BIT64(bit_number);
	if (!(feature & LL_FEAT_HOST_BIT_MASK)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

#if defined(CONFIG_BT_CONN)
	/* Check if the Controller has an established ACL */
	uint16_t conn_free_count = ll_conn_free_count_get();

	/* Check if any connection contexts where allocated */
	if (conn_free_count != CONFIG_BT_MAX_CONN) {
		uint16_t handle;

		/* Check if there are established connections */
		for (handle = 0U; handle < CONFIG_BT_MAX_CONN; handle++) {
			if (ll_connected_get(handle)) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		}
	}
#endif /* CONFIG_BT_CONN */

	/* Set or Clear the Host feature bit */
	if (bit_value) {
		host_features |= feature;
	} else {
		host_features &= ~feature;
	}

	return BT_HCI_ERR_SUCCESS;
}

uint64_t ll_feat_get(void)
{
	return LL_FEAT | (host_features & LL_FEAT_HOST_BIT_MASK);
}

#else /* !CONFIG_BT_CTLR_SET_HOST_FEATURE */
uint64_t ll_feat_get(void)
{
	return LL_FEAT;
}

#endif /* !CONFIG_BT_CTLR_SET_HOST_FEATURE */
