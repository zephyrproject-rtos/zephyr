/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Synthetic I3C peripheral emulator used by the i3c_emul ztest. Implements
 * the full struct i3c_emul_api surface that M2-M5 wire up: private xfers,
 * direct/broadcast CCC responders (GETPID/GETBCR/GETDCR/GETSTATUS/GETMXDS,
 * SETMRL/GETMRL/SETMWL/GETMWL, DEFTGTS capture, SETDASA/SETNEWDA/RSTDAA
 * acks, ENEC/DISEC event-mask handling) and a backend trigger API for tests.
 */

#define DT_DRV_COMPAT zephyr_i3c_emul_test_target

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <string.h>

#include "test_target_emul.h"

#define TEST_TARGET_REG_SIZE 32U

/* Capacity for a captured DEFTGTS payload: 1 byte count + active-controller
 * descriptor + a handful of target descriptors. The real subsys CCC bytes
 * are tightly packed, this just needs to be a generous upper bound for
 * reasonable test bus topologies.
 */
#define TEST_TARGET_DEFTGTS_BUF_SIZE 128U

struct test_target_cfg {
	uint64_t pid;
	uint8_t bcr;
	uint8_t dcr;
	bool supports_setaasa;
	bool supports_hdr_ddr;
};

struct test_target_data {
	uint8_t regs[TEST_TARGET_REG_SIZE];
	uint8_t cursor;

	/* HDR-DDR private xfer count. Bumped by test_target_xfers when
	 * a msg has the I3C_MSG_HDR flag with hdr_mode == HDR_DDR, so
	 * tests can verify the bus emulator gates HDR correctly without
	 * stripping the flag before delivery to the peripheral.
	 */
	uint32_t hdr_ddr_xfer_count;

	/* MRL / MWL state set via SETMRL / SETMWL, returned via GETMRL / GETMWL. */
	uint16_t mrl;
	uint16_t mwl;
	uint8_t mrl_ibi_len;

	/* GETSTATUS Format 1 value (test-poke-able for status tests). */
	uint16_t status_fmt1;

	/* Last DEFTGTS broadcast we observed. Captured for test inspection. */
	uint8_t deftgts_buf[TEST_TARGET_DEFTGTS_BUF_SIZE];
	size_t deftgts_len;
	bool deftgts_seen;
};

static int test_target_xfers(const struct emul *target, struct i3c_msg *msgs, uint8_t num_msgs)
{
	struct test_target_data *data = target->data;

	for (uint8_t i = 0; i < num_msgs; i++) {
		struct i3c_msg *m = &msgs[i];

		/*
		 * Tests assert on this counter to verify that HDR-DDR
		 * private xfers reach the peripheral via the same xfers
		 * callback as SDR (with I3C_MSG_HDR set in the flags).
		 */
		if ((m->flags & I3C_MSG_HDR) != 0U &&
		    m->hdr_mode == I3C_MSG_HDR_DDR) {
			data->hdr_ddr_xfer_count++;
		}

		if ((m->flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE) {
			if (m->len == 0U) {
				continue;
			}
			data->cursor = m->buf[0] % TEST_TARGET_REG_SIZE;
			for (uint32_t j = 1; j < m->len; j++) {
				data->regs[data->cursor] = m->buf[j];
				data->cursor = (data->cursor + 1U) % TEST_TARGET_REG_SIZE;
			}
			m->num_xfer = m->len;
		} else {
			for (uint32_t j = 0; j < m->len; j++) {
				m->buf[j] = data->regs[data->cursor];
				data->cursor = (data->cursor + 1U) % TEST_TARGET_REG_SIZE;
			}
			m->num_xfer = m->len;
		}
	}

	return 0;
}

/*
 * Find the target payload addressed to this peripheral in a direct CCC.
 * SETDASA uses static_addr (no dyn_addr yet); all others use dynamic_addr.
 * Matching on either lets the peripheral intercept all of its own direct CCCs
 * regardless of which lookup mode the bus emulator chose.
 */
static struct i3c_ccc_target_payload *
test_target_find_own_tp(const struct emul *target, const struct i3c_ccc_payload *payload)
{
	uint8_t my_static = target->bus.i3c->static_addr;
	uint8_t my_dynamic = target->bus.i3c->dynamic_addr;

	for (size_t i = 0; i < payload->targets.num_targets; i++) {
		uint8_t addr = payload->targets.payloads[i].addr;

		if (addr == my_dynamic || (my_static != 0U && addr == my_static)) {
			return &payload->targets.payloads[i];
		}
	}
	return NULL;
}

static int test_target_ccc_getpid(const struct test_target_cfg *cfg,
				   struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 6U) {
		sys_put_be48(cfg->pid, tp->data);
		tp->num_xfer = 6U;
	}
	return 0;
}

static int test_target_ccc_getbcr(const struct test_target_cfg *cfg,
				   struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		tp->data[0] = cfg->bcr;
		tp->num_xfer = 1U;
	}
	return 0;
}

static int test_target_ccc_getdcr(const struct test_target_cfg *cfg,
				   struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		tp->data[0] = cfg->dcr;
		tp->num_xfer = 1U;
	}
	return 0;
}

static int test_target_ccc_getmrl(const struct test_target_data *data,
				   struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 2U) {
		sys_put_be16(data->mrl, tp->data);
		tp->num_xfer = 2U;
		if (tp->data_len >= 3U) {
			tp->data[2] = data->mrl_ibi_len;
			tp->num_xfer = 3U;
		}
	}
	return 0;
}

static int test_target_ccc_getmwl(const struct test_target_data *data,
				   struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 2U) {
		sys_put_be16(data->mwl, tp->data);
		tp->num_xfer = 2U;
	}
	return 0;
}

static int test_target_ccc_getstatus(const struct test_target_data *data,
				     struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 2U) {
		sys_put_be16(data->status_fmt1, tp->data);
		tp->num_xfer = 2U;
	}
	return 0;
}

static int test_target_ccc_getcaps(const struct test_target_cfg *cfg,
				    struct i3c_ccc_target_payload *tp)
{
	/*
	 * GETCAPS Format 1 response: 1-4 bytes of capability flags.
	 * GETCAPS1 byte 0 carries HDR-mode bits; we set HDR-DDR per
	 * the supports-hdr-ddr DT property. Returning a single byte
	 * is spec-legal — the framework helper zero-pads the rest.
	 */
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		tp->data[0] = cfg->supports_hdr_ddr ? I3C_CCC_GETCAPS1_HDR_DDR : 0U;
		tp->num_xfer = 1U;
	}
	return 0;
}

static int test_target_ccc_getacccr(const struct emul *target,
				     struct i3c_ccc_target_payload *tp)
{
	/*
	 * GETACCCR response: dyn_addr left-shifted by 1 with bit[0] set to
	 * odd parity. The controller verifies both. See i3c_bus_getacccr().
	 */
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		uint8_t da = target->bus.i3c->dynamic_addr;

		tp->data[0] = (da << 1) | i3c_odd_parity(da);
		tp->num_xfer = 1U;
	}
	return 0;
}

static int test_target_ccc_setmrl_direct(struct test_target_data *data,
					  struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 2U) {
		data->mrl = sys_get_be16(tp->data);
		if (tp->data_len >= 3U) {
			data->mrl_ibi_len = tp->data[2];
		}
		tp->num_xfer = tp->data_len;
	}
	return 0;
}

static int test_target_ccc_setmwl_direct(struct test_target_data *data,
					  struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 2U) {
		data->mwl = sys_get_be16(tp->data);
		tp->num_xfer = 2U;
	}
	return 0;
}

static int test_target_ccc_setmrl_bc(struct test_target_data *data,
				      const struct i3c_ccc_payload *payload)
{
	if (payload->ccc.data != NULL && payload->ccc.data_len >= 2U) {
		data->mrl = sys_get_be16(payload->ccc.data);
		if (payload->ccc.data_len >= 3U) {
			data->mrl_ibi_len = payload->ccc.data[2];
		}
	}
	return 0;
}

static int test_target_ccc_setmwl_bc(struct test_target_data *data,
				      const struct i3c_ccc_payload *payload)
{
	if (payload->ccc.data != NULL && payload->ccc.data_len >= 2U) {
		data->mwl = sys_get_be16(payload->ccc.data);
	}
	return 0;
}

static int test_target_ccc_deftgts(struct test_target_data *data,
				    const struct i3c_ccc_payload *payload)
{
	if (payload->ccc.data != NULL && payload->ccc.data_len > 0U) {
		size_t copy = MIN(payload->ccc.data_len, sizeof(data->deftgts_buf));

		memcpy(data->deftgts_buf, payload->ccc.data, copy);
		data->deftgts_len = copy;
		data->deftgts_seen = true;
	}
	return 0;
}

static int test_target_ccc_setdasa(const struct emul *target, struct i3c_ccc_target_payload *tp)
{
	/*
	 * Wire-level address-mutating CCCs: the peripheral updates its own
	 * dyn_addr from the wire payload, the same way real silicon would.
	 *
	 * SETDASA: only legal when the target has a static_addr and no dyn_addr.
	 */
	if (target->bus.i3c->static_addr == 0U || target->bus.i3c->dynamic_addr != 0U) {
		return -EACCES;
	}
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		target->bus.i3c->dynamic_addr = tp->data[0] >> 1;
	}
	return 0;
}

static int test_target_ccc_setaasa(const struct emul *target, const struct test_target_cfg *cfg)
{
	/*
	 * SETAASA: broadcast, self-assign static_addr as dynamic_addr.
	 * Targets without supports-setaasa, no static_addr, or an existing
	 * dyn_addr must ignore it.
	 */
	if (!cfg->supports_setaasa || target->bus.i3c->static_addr == 0U ||
	    target->bus.i3c->dynamic_addr != 0U) {
		return 0;
	}
	target->bus.i3c->dynamic_addr = target->bus.i3c->static_addr;
	return 0;
}

static int test_target_ccc_setnewda(const struct emul *target, struct i3c_ccc_target_payload *tp)
{
	/* SETNEWDA: change an existing dyn_addr — only legal when one exists. */
	if (target->bus.i3c->dynamic_addr == 0U) {
		return -EACCES;
	}
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		target->bus.i3c->dynamic_addr = tp->data[0] >> 1;
	}
	return 0;
}

static int test_target_do_daa(const struct emul *target, uint8_t dynamic_addr, uint64_t *pid,
			      uint8_t *bcr, uint8_t *dcr)
{
	const struct test_target_cfg *cfg = target->cfg;
	struct i3c_emul *self = target->bus.i3c;

	if (self->dynamic_addr != 0U) {
		return -EALREADY;
	}

	self->dynamic_addr = dynamic_addr;

	if (pid != NULL) {
		*pid = cfg->pid;
	}
	if (bcr != NULL) {
		*bcr = cfg->bcr;
	}
	if (dcr != NULL) {
		*dcr = cfg->dcr;
	}
	return 0;
}

static int test_target_ccc_enec_bc(const struct emul *target, const struct i3c_ccc_payload *payload)
{
	if (payload->ccc.data != NULL && payload->ccc.data_len >= 1U) {
		target->bus.i3c->enabled_events |= payload->ccc.data[0] & I3C_CCC_EVT_ALL;
	}
	return 0;
}

static int test_target_ccc_enec_direct(const struct emul *target,
					struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		target->bus.i3c->enabled_events |= tp->data[0] & I3C_CCC_EVT_ALL;
	}
	return 0;
}

static int test_target_ccc_disec_bc(const struct emul *target,
				     const struct i3c_ccc_payload *payload)
{
	if (payload->ccc.data != NULL && payload->ccc.data_len >= 1U) {
		target->bus.i3c->enabled_events &= ~(payload->ccc.data[0] & I3C_CCC_EVT_ALL);
	}
	return 0;
}

static int test_target_ccc_disec_direct(const struct emul *target,
					 struct i3c_ccc_target_payload *tp)
{
	if (tp != NULL && tp->data != NULL && tp->data_len >= 1U) {
		target->bus.i3c->enabled_events &= ~(tp->data[0] & I3C_CCC_EVT_ALL);
	}
	return 0;
}

static int test_target_do_ccc(const struct emul *target, struct i3c_ccc_payload *payload)
{
	struct test_target_data *data = target->data;
	const struct test_target_cfg *cfg = target->cfg;
	struct i3c_ccc_target_payload *tp = NULL;

	if (!i3c_ccc_is_payload_broadcast(payload)) {
		tp = test_target_find_own_tp(target, payload);
		if (tp == NULL) {
			return 0;
		}
	}

	switch (payload->ccc.id) {
	case I3C_CCC_GETPID:
		return test_target_ccc_getpid(cfg, tp);
	case I3C_CCC_GETBCR:
		return test_target_ccc_getbcr(cfg, tp);
	case I3C_CCC_GETDCR:
		return test_target_ccc_getdcr(cfg, tp);
	case I3C_CCC_GETMRL:
		return test_target_ccc_getmrl(data, tp);
	case I3C_CCC_GETMWL:
		return test_target_ccc_getmwl(data, tp);
	case I3C_CCC_GETSTATUS:
		return test_target_ccc_getstatus(data, tp);
	case I3C_CCC_GETCAPS:
		return test_target_ccc_getcaps(cfg, tp);
	case I3C_CCC_GETACCCR:
		return test_target_ccc_getacccr(target, tp);
	case I3C_CCC_SETMRL(false):
		return test_target_ccc_setmrl_direct(data, tp);
	case I3C_CCC_SETMWL(false):
		return test_target_ccc_setmwl_direct(data, tp);
	case I3C_CCC_SETMRL(true):
		return test_target_ccc_setmrl_bc(data, payload);
	case I3C_CCC_SETMWL(true):
		return test_target_ccc_setmwl_bc(data, payload);
	case I3C_CCC_DEFTGTS:
		return test_target_ccc_deftgts(data, payload);
	case I3C_CCC_SETDASA:
		return test_target_ccc_setdasa(target, tp);
	case I3C_CCC_SETAASA:
		return test_target_ccc_setaasa(target, cfg);
	case I3C_CCC_SETNEWDA:
		return test_target_ccc_setnewda(target, tp);
	case I3C_CCC_RSTDAA:
		target->bus.i3c->dynamic_addr = 0;
		return 0;
	case I3C_CCC_ENEC(true):
		return test_target_ccc_enec_bc(target, payload);
	case I3C_CCC_ENEC(false):
		return test_target_ccc_enec_direct(target, tp);
	case I3C_CCC_DISEC(true):
		return test_target_ccc_disec_bc(target, payload);
	case I3C_CCC_DISEC(false):
		return test_target_ccc_disec_direct(target, tp);
	default:
		return -ENOTSUP;
	}
}

static const struct i3c_emul_api test_target_api = {
	.xfers = test_target_xfers,
	.do_ccc = test_target_do_ccc,
	.do_daa = test_target_do_daa,
};

uint8_t test_target_get_reg(const struct emul *target, uint8_t idx)
{
	struct test_target_data *data = target->data;

	return data->regs[idx % TEST_TARGET_REG_SIZE];
}

void test_target_set_reg(const struct emul *target, uint8_t idx, uint8_t value)
{
	struct test_target_data *data = target->data;

	data->regs[idx % TEST_TARGET_REG_SIZE] = value;
}

uint8_t test_target_get_dynamic_addr(const struct emul *target)
{
	return target->bus.i3c->dynamic_addr;
}

void test_target_install_mock(const struct emul *target, struct i3c_emul_api *mock)
{
	target->bus.i3c->mock_api = mock;
}

int test_target_trigger_ibi(const struct emul *target, uint8_t *payload, uint8_t len)
{
	return i3c_emul_target_raise_ibi(target, payload, len);
}

int test_target_trigger_hj(const struct emul *target)
{
	return i3c_emul_target_raise_hj(target);
}

int test_target_trigger_crr(const struct emul *target)
{
	return i3c_emul_target_raise_crr(target);
}

void test_target_set_status_fmt1(const struct emul *target, uint16_t status)
{
	struct test_target_data *data = target->data;

	data->status_fmt1 = status;
}

uint16_t test_target_get_mrl(const struct emul *target)
{
	struct test_target_data *data = target->data;

	return data->mrl;
}

uint16_t test_target_get_mwl(const struct emul *target)
{
	struct test_target_data *data = target->data;

	return data->mwl;
}

bool test_target_deftgts_was_seen(const struct emul *target)
{
	struct test_target_data *data = target->data;

	return data->deftgts_seen;
}

size_t test_target_get_deftgts(const struct emul *target, uint8_t *out, size_t out_len)
{
	struct test_target_data *data = target->data;
	size_t copy = MIN(out_len, data->deftgts_len);

	if (copy > 0U) {
		memcpy(out, data->deftgts_buf, copy);
	}
	return copy;
}

void test_target_clear_deftgts(const struct emul *target)
{
	struct test_target_data *data = target->data;

	data->deftgts_len = 0;
	data->deftgts_seen = false;
}

bool test_target_event_enabled(const struct emul *target, uint8_t event_mask)
{
	return (target->bus.i3c->enabled_events & event_mask) == event_mask;
}

uint32_t test_target_get_hdr_ddr_xfer_count(const struct emul *target)
{
	const struct test_target_data *data = target->data;

	return data->hdr_ddr_xfer_count;
}

static const struct test_target_backend_api test_target_backend = {
	.get_reg = test_target_get_reg,
	.set_reg = test_target_set_reg,
	.get_dynamic_addr = test_target_get_dynamic_addr,
	.install_mock = test_target_install_mock,
	.trigger_ibi = test_target_trigger_ibi,
	.trigger_hj = test_target_trigger_hj,
	.trigger_crr = test_target_trigger_crr,
	.set_status_fmt1 = test_target_set_status_fmt1,
	.get_mrl = test_target_get_mrl,
	.get_mwl = test_target_get_mwl,
	.deftgts_was_seen = test_target_deftgts_was_seen,
	.get_deftgts = test_target_get_deftgts,
	.clear_deftgts = test_target_clear_deftgts,
	.event_enabled = test_target_event_enabled,
};

static int test_target_init(const struct emul *target, const struct device *parent)
{
	const struct test_target_cfg *cfg = target->cfg;

	ARG_UNUSED(parent);

	target->bus.i3c->pid = cfg->pid;
	target->bus.i3c->bcr = cfg->bcr;
	target->bus.i3c->dcr = cfg->dcr;
	target->bus.i3c->getcaps1 = cfg->supports_hdr_ddr ? I3C_CCC_GETCAPS1_HDR_DDR : 0U;

	/*
	 * Powered-up baseline per the I3C spec: targets come up with INTR
	 * and CR raise capability. HJ stays off until the controller broadcasts
	 * ENEC(HJ) — which i3c_bus_init does at the end of initialization.
	 */
	target->bus.i3c->enabled_events = I3C_CCC_EVT_INTR | I3C_CCC_EVT_CR;
	return 0;
}

#define TEST_TARGET_INIT(n)                                                                        \
	static struct test_target_data test_target_data_##n;                                       \
	static const struct test_target_cfg test_target_cfg_##n = {                                \
		.pid = ((uint64_t)DT_INST_PROP_BY_IDX(n, reg, 1) << 32) |                          \
		       DT_INST_PROP_BY_IDX(n, reg, 2),                                             \
		.bcr = DT_INST_PROP(n, bcr),                                                       \
		.dcr = DT_INST_PROP(n, dcr),                                                       \
		.supports_setaasa = DT_INST_PROP(n, supports_setaasa),                             \
		.supports_hdr_ddr = DT_INST_PROP(n, supports_hdr_ddr),                             \
	};                                                                                         \
	I3C_DEVICE_DT_INST_DEFINE(n, NULL, NULL, &test_target_data_##n, &test_target_cfg_##n,      \
				  POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, NULL);            \
	EMUL_DT_INST_DEFINE(n, test_target_init, &test_target_data_##n, &test_target_cfg_##n,      \
			    &test_target_api, &test_target_backend);

DT_INST_FOREACH_STATUS_OKAY(TEST_TARGET_INIT)

int test_target_bus_known_state(const struct device *bus, uint64_t pid_a, uint8_t static_a,
				uint64_t pid_b, uint8_t init_dyn_b)
{
	struct i3c_device_id id_a = { .pid = pid_a };
	struct i3c_device_id id_b = { .pid = pid_b };
	struct i3c_device_desc *desc_a = i3c_device_find(bus, &id_a);
	struct i3c_device_desc *desc_b = i3c_device_find(bus, &id_b);
	int rc;

	if (desc_a == NULL || desc_b == NULL) {
		return -ENODEV;
	}

	if (desc_a->dynamic_addr == static_a && desc_b->dynamic_addr == init_dyn_b) {
		return 0;
	}

	rc = i3c_bus_rstdaa_all(bus);
	if (rc != 0 && rc != -ENOTSUP) {
		return rc;
	}

	rc = i3c_bus_setdasa(desc_a, static_a);
	if (rc != 0) {
		return rc;
	}

	rc = i3c_do_daa(bus);
	if (rc != 0) {
		return rc;
	}

	if (desc_a->dynamic_addr != static_a || desc_b->dynamic_addr != init_dyn_b) {
		return -EIO;
	}

	return 0;
}
