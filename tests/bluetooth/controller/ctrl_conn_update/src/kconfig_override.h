/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Override common Kconfig settings
 */

#ifdef CONFIG_BT_CTLR_CONN_PARAM_REQ
#undef CONFIG_BT_CTLR_CONN_PARAM_REQ
#endif
