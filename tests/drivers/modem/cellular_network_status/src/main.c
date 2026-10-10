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

/* A vendor measures neighbours when it has a measurement script; the request
 * also needs the periodic script that starts it.
 */
static const struct modem_chat_script placeholder_script;

static const struct modem_cellular_vendor_config reporting_vendor = {
	.scripts = {
		.periodic = &placeholder_script,
		.neighbor_scan = &placeholder_script,
	},
};

static const struct modem_cellular_config reporting_config = {
	.vendor = &reporting_vendor,
};

static const struct modem_cellular_vendor_config silent_vendor;

static const struct modem_cellular_config silent_config = {
	.vendor = &silent_vendor,
};

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

	cell.access_tech = CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN;
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
		.config = &reporting_config,
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

/* The accumulator writes straight into the buffer the requesting thread owns. */
ZTEST(cellular_network_status, test_neighbor_accumulate)
{
	struct modem_cellular_data data;
	struct cellular_neighbor_cell a = make_neighbor(100, 10, -90, -10);
	struct cellular_neighbor_cell b = make_neighbor(200, 20, -95, -12);
	struct cellular_neighbor_cell out[4];

	init_data(&data);
	data.neighbor_cells = out;
	data.neighbor_cell_capacity = ARRAY_SIZE(out);

	modem_cellular_add_neighbor_cell(&data, &a);
	modem_cellular_add_neighbor_cell(&data, &b);

	zassert_equal(data.neighbor_cell_count, 2, "both neighbours must be written");
	zassert_false(data.neighbor_cells_truncated, "a buffer that fits must not truncate");
	zassert_equal(out[0].cell.lte.earfcn, 100);
	zassert_equal(out[0].access_tech, CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN,
		      "the access technology must select the reported union member");
	zassert_equal(out[1].cell.lte.phys_cell_id, 20);
	zassert_equal(out[1].cell.lte.rsrp, -95);
}

/* A buffer smaller than the measurement is filled to capacity and the overflow
 * reported, so a caller that sized low still reads what fits.
 */
ZTEST(cellular_network_status, test_neighbor_truncates)
{
	struct modem_cellular_data data;
	struct cellular_neighbor_cell a = make_neighbor(100, 10, -90, -10);
	struct cellular_neighbor_cell b = make_neighbor(200, 20, -95, -12);
	struct cellular_neighbor_cell out[1];

	init_data(&data);
	data.neighbor_cells = out;
	data.neighbor_cell_capacity = ARRAY_SIZE(out);

	modem_cellular_add_neighbor_cell(&data, &a);
	modem_cellular_add_neighbor_cell(&data, &b);

	zassert_equal(data.neighbor_cell_count, 1, "only what fits must be written");
	zassert_true(data.neighbor_cells_truncated, "the overflow must be reported");
	zassert_equal(out[0].cell.lte.earfcn, 100, "the entries that fit must be written");
}

/* A report arriving outside a measurement has nowhere to go. */
ZTEST(cellular_network_status, test_neighbor_unsolicited_dropped)
{
	struct modem_cellular_data data;
	struct cellular_neighbor_cell a = make_neighbor(100, 10, -90, -10);

	init_data(&data);

	modem_cellular_add_neighbor_cell(&data, &a);

	zassert_equal(data.neighbor_cell_count, 0, "a report with no buffer must be dropped");
	zassert_false(data.neighbor_cells_truncated, "dropping must not report truncation");
}

/* Without a measurement script there is nothing to ask for. */
ZTEST(cellular_network_status, test_neighbor_unsupported_vendor)
{
	struct modem_cellular_data data;
	const struct device dev = {
		.data = &data,
		.config = &silent_config,
		.api = &modem_cellular_api,
	};
	struct cellular_neighbor_cell out[4];
	uint8_t count = ARRAY_SIZE(out);

	init_data(&data);

	zassert_equal(cellular_scan_neighbor_cells(&dev, out, &count), -ENOSYS,
		      "a vendor with no measurement script must return -ENOSYS");
}

ZTEST_SUITE(cellular_network_status, NULL, NULL, NULL, NULL, NULL);
