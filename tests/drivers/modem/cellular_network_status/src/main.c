/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/modem/modem_cellular.h>

static int emit_count;
static struct cellular_evt_network_status last_payload;

static void status_cb(const struct device *dev, enum cellular_event event, const void *payload,
		      void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (event == CELLULAR_EVENT_NETWORK_STATUS_CHANGED) {
		emit_count++;
		last_payload = *(const struct cellular_evt_network_status *)payload;
	}
}

static void init_data(struct modem_cellular_data *data)
{
	memset(data, 0, sizeof(*data));
	k_mutex_init(&data->api_lock);
	data->cb.fn = status_cb;
	data->cb.mask = CELLULAR_EVENT_NETWORK_STATUS_CHANGED;
}

static struct cellular_evt_network_status make_lte(uint32_t gci, int16_t rsrp)
{
	struct cellular_evt_network_status status = {
		.status = CELLULAR_REGISTRATION_REGISTERED_HOME,
		.access_tech = CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN,
	};

	status.cell.lte.mcc = 310;
	status.cell.lte.mnc = 260;
	status.cell.lte.gci = gci;
	status.cell.lte.rsrp = rsrp;

	return status;
}

static struct cellular_neighbor_cell make_neighbor(uint32_t earfcn, uint16_t pci, int16_t rsrp,
						   int8_t rsrq)
{
	struct cellular_neighbor_cell cell = {0};

	cell.cell.lte.earfcn = earfcn;
	cell.cell.lte.phys_cell_id = pci;
	cell.cell.lte.rsrp = rsrp;
	cell.cell.lte.rsrq = rsrq;

	return cell;
}

/* modem_cellular_emit_network_status() must raise the event on the first report
 * and on any changed field, but suppress a report identical to the last one.
 */
ZTEST(cellular_network_status, test_emit_on_change_only)
{
	struct modem_cellular_data data;
	struct cellular_evt_network_status status = make_lte(0x1234567, -95);

	init_data(&data);
	emit_count = 0;

	modem_cellular_emit_network_status(&data, &status);
	zassert_equal(emit_count, 1, "first report must emit");
	zassert_true(data.network_status_valid, "cache must be valid after emit");
	zassert_equal(data.network_status.cell.lte.gci, 0x1234567);

	modem_cellular_emit_network_status(&data, &status);
	zassert_equal(emit_count, 1, "identical report must be suppressed");

	/* Signal-only change refreshes the cache but is not a status change. */
	status.cell.lte.rsrp = -100;
	modem_cellular_emit_network_status(&data, &status);
	zassert_equal(emit_count, 1, "signal-only change must not emit");
	zassert_equal(data.network_status.cell.lte.rsrp, -100, "cache must still refresh signal");

	/* Cell identity change emits. */
	status.cell.lte.gci = 0x7654321;
	modem_cellular_emit_network_status(&data, &status);
	zassert_equal(emit_count, 2, "cell change must emit");
}

/* cellular_get_network_status() returns -ENODATA until a status has been cached,
 * then the cached snapshot, and -ENODATA again once the cache is invalidated.
 */
ZTEST(cellular_network_status, test_getter_states)
{
	struct modem_cellular_data data;
	const struct device dev = {
		.data = &data,
		.api = &modem_cellular_api,
	};
	struct cellular_evt_network_status status = make_lte(0x1234567, -95);
	struct cellular_evt_network_status out;

	init_data(&data);

	zassert_equal(cellular_get_network_status(&dev, &out), -ENODATA,
		      "getter must report no data before the first report");

	modem_cellular_emit_network_status(&data, &status);
	zassert_ok(cellular_get_network_status(&dev, &out), "getter must succeed once cached");
	zassert_equal(out.cell.lte.gci, 0x1234567);
	zassert_equal(out.cell.lte.rsrp, -95);

	/* Invalidation (as done on a registration change) restores the no-data state. */
	data.network_status_valid = false;
	zassert_equal(cellular_get_network_status(&dev, &out), -ENODATA,
		      "getter must report no data after invalidation");
}

/* The accumulator publishes the neighbour list only on commit; the getter must
 * report -ENODATA while a scan is in progress and the full list afterwards.
 */
ZTEST(cellular_network_status, test_neighbor_accumulate_and_get)
{
	struct modem_cellular_data data;
	const struct device dev = {
		.data = &data,
		.api = &modem_cellular_api,
	};
	struct cellular_neighbor_cell a = make_neighbor(100, 10, -90, -10);
	struct cellular_neighbor_cell b = make_neighbor(200, 20, -95, -12);
	struct cellular_neighbor_cell out[4];
	uint8_t count;

	init_data(&data);

	count = ARRAY_SIZE(out);
	zassert_equal(cellular_get_neighbor_cells(&dev, out, &count), -ENODATA,
		      "getter must report no data before the first scan");

	modem_cellular_add_neighbor_cell(&data, &a);

	count = ARRAY_SIZE(out);
	zassert_equal(cellular_get_neighbor_cells(&dev, out, &count), -ENODATA,
		      "staged cells must not be visible before commit");

	modem_cellular_add_neighbor_cell(&data, &b);
	modem_cellular_commit_neighbor_cells(&data);

	count = ARRAY_SIZE(out);
	zassert_ok(cellular_get_neighbor_cells(&dev, out, &count));
	zassert_equal(count, 2, "both neighbours must be returned");
	zassert_equal(out[0].cell.lte.earfcn, 100);
	zassert_equal(out[1].cell.lte.phys_cell_id, 20);
	zassert_equal(out[1].cell.lte.rsrp, -95);
}

/* A completed scan with no LTE neighbours is valid, distinct from -ENODATA. */
ZTEST(cellular_network_status, test_neighbor_empty_scan)
{
	struct modem_cellular_data data;
	const struct device dev = {
		.data = &data,
		.api = &modem_cellular_api,
	};
	struct cellular_neighbor_cell out[4];
	uint8_t count;

	init_data(&data);

	modem_cellular_commit_neighbor_cells(&data);

	count = ARRAY_SIZE(out);
	zassert_ok(cellular_get_neighbor_cells(&dev, out, &count),
		   "a completed scan with no neighbours must succeed");
	zassert_equal(count, 0, "no neighbours reported");
}

/* A caller buffer smaller than the cached list is rejected, and count reports
 * the number available so the caller can size a second call.
 */
ZTEST(cellular_network_status, test_neighbor_buffer_too_small)
{
	struct modem_cellular_data data;
	const struct device dev = {
		.data = &data,
		.api = &modem_cellular_api,
	};
	struct cellular_neighbor_cell a = make_neighbor(100, 10, -90, -10);
	struct cellular_neighbor_cell b = make_neighbor(200, 20, -95, -12);
	struct cellular_neighbor_cell out[1];
	uint8_t count;

	init_data(&data);

	modem_cellular_add_neighbor_cell(&data, &a);
	modem_cellular_add_neighbor_cell(&data, &b);
	modem_cellular_commit_neighbor_cells(&data);

	count = ARRAY_SIZE(out);
	zassert_equal(cellular_get_neighbor_cells(&dev, out, &count), -ENOMEM,
		      "a too-small buffer must be rejected");
	zassert_equal(count, 2, "count must report the number available");
}

/* Neighbours reported beyond the staging cap are dropped, not overflowed. */
ZTEST(cellular_network_status, test_neighbor_overflow_dropped)
{
	struct modem_cellular_data data;
	const struct device dev = {
		.data = &data,
		.api = &modem_cellular_api,
	};
	struct cellular_neighbor_cell c;
	struct cellular_neighbor_cell out[CONFIG_MODEM_CELLULAR_MAX_NEIGHBOR_CELLS + 4];
	uint8_t count;
	int i;

	init_data(&data);

	for (i = 0; i < CONFIG_MODEM_CELLULAR_MAX_NEIGHBOR_CELLS + 2; i++) {
		c = make_neighbor(i, i, -80, -8);
		modem_cellular_add_neighbor_cell(&data, &c);
	}
	modem_cellular_commit_neighbor_cells(&data);

	count = ARRAY_SIZE(out);
	zassert_ok(cellular_get_neighbor_cells(&dev, out, &count));
	zassert_equal(count, CONFIG_MODEM_CELLULAR_MAX_NEIGHBOR_CELLS,
		      "excess neighbours beyond the cap must be dropped");
}

/* Each commit fully replaces the previous list; the staging buffer self-clears,
 * so a later, shorter scan does not leak stale neighbours from an earlier one.
 */
ZTEST(cellular_network_status, test_neighbor_rescan_replaces)
{
	struct modem_cellular_data data;
	const struct device dev = {
		.data = &data,
		.api = &modem_cellular_api,
	};
	struct cellular_neighbor_cell a = make_neighbor(100, 10, -90, -10);
	struct cellular_neighbor_cell b = make_neighbor(200, 20, -95, -12);
	struct cellular_neighbor_cell c = make_neighbor(300, 30, -85, -9);
	struct cellular_neighbor_cell out[4];
	uint8_t count;

	init_data(&data);

	modem_cellular_add_neighbor_cell(&data, &a);
	modem_cellular_add_neighbor_cell(&data, &b);
	modem_cellular_commit_neighbor_cells(&data);

	modem_cellular_add_neighbor_cell(&data, &c);
	modem_cellular_commit_neighbor_cells(&data);

	count = ARRAY_SIZE(out);
	zassert_ok(cellular_get_neighbor_cells(&dev, out, &count));
	zassert_equal(count, 1, "second scan must replace, not append to, the first");
	zassert_equal(out[0].cell.lte.earfcn, 300);
}

ZTEST_SUITE(cellular_network_status, NULL, NULL, NULL, NULL, NULL);
