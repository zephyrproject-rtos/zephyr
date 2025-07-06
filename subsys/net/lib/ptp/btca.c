/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ptp_btca, CONFIG_PTP_LOG_LEVEL);

#include <string.h>

#include "btca.h"
#include "clock.h"
#include "port.h"

#define A_BETTER	  (1)
#define A_BETTER_TOPOLOGY (2)
#define B_BETTER	  (-1)
#define B_BETTER_TOPOLOGY (-2)

static int btca_port_id_cmp(const struct ptp_port_id *p1, const struct ptp_port_id *p2)
{
	int diff = memcmp(&p1->clk_id, &p2->clk_id, sizeof(p1->clk_id));

	if (diff == 0) {
		diff = p1->port_number - p2->port_number;
	}

	return diff;
}

static int btca_ds_cmp2(const struct ptp_dataset *a, const struct ptp_dataset *b)
{
	int diff;

	if (b->steps_rm + 1 < a->steps_rm) {
		return B_BETTER;
	}
	if (a->steps_rm + 1 < b->steps_rm) {
		return A_BETTER;
	}
	if (a->steps_rm > b->steps_rm) {
		diff = btca_port_id_cmp(&a->receiver, &a->sender);
		if (diff > 0) {
			return B_BETTER_TOPOLOGY;
		}
		if (diff < 0) {
			return B_BETTER;
		}
		/* error-1 */
		return 0;
	}
	if (a->steps_rm < b->steps_rm) {
		diff = btca_port_id_cmp(&b->receiver, &b->sender);
		if (diff > 0) {
			return A_BETTER_TOPOLOGY;
		}
		if (diff < 0) {
			return A_BETTER;
		}
		/* error-1 */
		return 0;
	}

	diff = btca_port_id_cmp(&a->sender, &b->sender);
	if (diff > 0) {
		return B_BETTER_TOPOLOGY;
	}
	if (diff < 0) {
		return A_BETTER_TOPOLOGY;
	}

	if (a->receiver.port_number > b->receiver.port_number) {
		return B_BETTER_TOPOLOGY;
	}
	if (a->receiver.port_number > b->receiver.port_number) {
		return A_BETTER_TOPOLOGY;
	}
	/* error-2 */
	return 0;
}

int ptp_btca_ds_cmp(const struct ptp_dataset *a, const struct ptp_dataset *b)
{
	if (a == b) {
		return 0;
	}
	if (a && !b) {
		return A_BETTER;
	}
	if (!a && b) {
		return B_BETTER;
	}

	int id_diff = memcmp(&a->clk_id, &b->clk_id, sizeof(a->clk_id));

	if (id_diff == 0) {
		return btca_ds_cmp2(a, b);
	}
	if (a->priority1 > b->priority1) {
		return B_BETTER;
	}
	if (a->clk_quality.cls > b->clk_quality.cls) {
		return B_BETTER;
	}
	if (a->clk_quality.accuracy > b->clk_quality.accuracy) {
		return B_BETTER;
	}
	if (a->clk_quality.offset_scaled_log_variance > b->clk_quality.offset_scaled_log_variance) {
		return B_BETTER;
	}
	if (a->priority2 > b->priority2) {
		return B_BETTER;
	}

	return id_diff < 0 ? A_BETTER : B_BETTER;
}

enum ptp_port_state ptp_btca_state_decision(struct ptp_port *port)
{
	const struct ptp_foreign_tt_clock *clk_best_foreign = ptp_clock_best_time_transmitter();
	const struct ptp_dataset *clk_default, *clk_best, *port_best;

	clk_default = ptp_clock_ds();
	clk_best = ptp_clock_best_foreign_ds();
	port_best = ptp_port_best_foreign_ds(port);

	if (!port_best && ptp_port_state(port) == PTP_PS_LISTENING) {
		return PTP_PS_LISTENING;
	}

	if (clk_default->clk_quality.cls <= 127) {
		if (ptp_btca_ds_cmp(clk_default, port_best) > 0) {
			/* M1 */
			return PTP_PS_GRAND_MASTER;
		}
		/* P1 */
		return PTP_PS_PASSIVE;
	}

	if (ptp_btca_ds_cmp(clk_default, clk_best) > 0) {
		/* M2 */
		return PTP_PS_GRAND_MASTER;
	}

	if (clk_best_foreign->port == port) {
		/* S1 */
		return PTP_PS_TIME_RECEIVER;
	}

	if (ptp_btca_ds_cmp(clk_best, port_best) == A_BETTER_TOPOLOGY) {
		/* P2 */
		return PTP_PS_PASSIVE;
	}
	/* M3 */
	return PTP_PS_TIME_TRANSMITTER;
}
