/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(NO_ENC)
#undef CONFIG_BT_CTLR_LE_ENC
#endif /* NO_ENC */

#if defined(NO_PER_FEAT_EXCH)
#undef CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG
#endif /* NO_PER_FEAT_EXCH */

#if defined(NO_CPR)
#undef CONFIG_BT_CTLR_CONN_PARAM_REQ
#endif /* NO_CPR */

#if defined(NO_PHY)
#undef CONFIG_BT_CTLR_PHY
#endif /* NO_PHY */
