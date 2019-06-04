/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_USER_EXT)

/* The test is made to show that ull.c and other code can compile when
 * certain Kconfig options are set. This includes functions that are
 * used for vendor specific behavior, as such the function implementations
 * in this file are simple stubs without functional behavior.
 */

static inline int ull_user_init(void)
{
	return 0;
}

static inline int rx_demux_rx_proprietary(memq_link_t *link,
					  struct node_rx_hdr *rx,
					  memq_link_t *tail,
					  memq_link_t **head)
{
	return 0;
}

static inline void ull_proprietary_done(struct node_rx_event_done *evdone)
{
	/* Nothing to do */
}

#endif /* CONFIG_BT_CTLR_USER_EXT */
