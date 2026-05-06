/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulated I3C bus controller. Mirrors the role of drivers/i2c/i2c_emul.c but
 * covers the I3C controller API: private SDR transfers, broadcast/direct CCCs,
 * dynamic-address assignment, and (in later milestones) IBI, async, RTIO,
 * target-mode and controller-handoff.
 */

#define DT_DRV_COMPAT zephyr_i3c_emul

#define LOG_LEVEL CONFIG_I3C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_emul_ctlr);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/addresses.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c/ibi.h>
#include <zephyr/drivers/i3c/target_device.h>
#include <zephyr/drivers/i3c_emul.h>
#include <zephyr/sys/__assert.h>

#include <string.h>

struct i3c_emul_data {
	struct i3c_driver_data common;
	/*
	 * HDR-mode-entry state. Set by ENTHDR0 broadcast, cleared by
	 * the next SDR-mode operation (per spec, the bus drops out of
	 * HDR mode after a stop). Bus emul gates HDR-DDR private xfers
	 * on this flag.
	 */
	uint8_t hdr_mode_entered;
#ifdef CONFIG_I3C_USE_IBI
	bool ibi_hj_ack;
	bool ibi_crr_ack;
#endif
#ifdef CONFIG_I3C_TARGET
	struct i3c_target_config *target_cfg;
	struct i3c_config_target target_config;
	bool handoff_accept;
#if CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE > 0
	uint8_t tx_fifo[CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE];
	size_t tx_fifo_len;
#endif
#endif
};

struct i3c_emul_config {
	struct i3c_driver_config common;
	struct emul_list_for_bus emul_list;
	uint32_t i3c_scl_hz;
	uint32_t i2c_scl_hz;
};

static struct i3c_emul *i3c_emul_for_desc(const struct i3c_device_desc *desc)
{
	if (desc == NULL) {
		return NULL;
	}
	return (struct i3c_emul *)desc->controller_priv;
}

/*
 * PID is the constant identifier across DAA / SETDASA / SETNEWDA, so it
 * is the only stable correlator between a controller-side desc and a
 * peripheral. Walk every peripheral emul in the binary, find the one
 * bound to this bus whose PID matches.
 */
static struct i3c_emul *i3c_emul_lookup_by_pid(const struct device *bus, uint64_t pid)
{
	if (pid == 0U) {
		return NULL;
	}
	STRUCT_SECTION_FOREACH(emul, e) {
		if (e->bus_type != EMUL_BUS_TYPE_I3C) {
			continue;
		}
		if (e->bus.i3c == NULL || e->bus.i3c->bus != bus) {
			continue;
		}
		if (e->bus.i3c->pid == pid) {
			return e->bus.i3c;
		}
	}
	return NULL;
}

static struct i2c_emul *i3c_emul_lookup_i2c_by_addr(const struct device *bus, uint16_t addr)
{
	const struct i3c_emul_config *cfg = bus->config;
	const struct emul_link_for_bus *const end =
		cfg->emul_list.children + cfg->emul_list.num_children;

	for (const struct emul_link_for_bus *elp = cfg->emul_list.children; elp < end; elp++) {
		const struct emul *e = emul_get_binding(elp->dev->name);

		if (e == NULL || e->bus_type != EMUL_BUS_TYPE_I2C) {
			continue;
		}
		if (e->bus.i2c != NULL && e->bus.i2c->addr == addr) {
			return e->bus.i2c;
		}
	}
	return NULL;
}

/*
 * Wire-level dynamic-address lookup. Used when a CCC arrives addressed
 * at a dynamic address but the desc carrying the call has no PID linkage
 * yet — concretely the temp_desc i3c_sec_get_basic_info attaches before
 * issuing GETPID after a controller handoff.
 */
static struct i3c_emul *i3c_emul_lookup_by_dyn_addr(const struct device *bus, uint8_t addr)
{
	if (addr == 0U) {
		return NULL;
	}
	STRUCT_SECTION_FOREACH(emul, e) {
		if (e->bus_type != EMUL_BUS_TYPE_I3C) {
			continue;
		}
		if (e->bus.i3c == NULL || e->bus.i3c->bus != bus) {
			continue;
		}
		if (e->bus.i3c->dynamic_addr == addr) {
			return e->bus.i3c;
		}
	}
	return NULL;
}

static struct i3c_emul *i3c_emul_find_by_addr(const struct device *dev, uint8_t addr,
					      bool is_setdasa)
{
	struct i3c_device_desc *desc;
	struct i3c_emul *target_emul;

	if (addr == 0U) {
		return NULL;
	}

	desc = is_setdasa ? i3c_dev_list_i3c_static_addr_find(dev, addr)
			  : i3c_dev_list_i3c_addr_find(dev, addr);
	target_emul = i3c_emul_for_desc(desc);
	if (target_emul != NULL) {
		return target_emul;
	}

	/*
	 * The desc may exist but lack a PID-derived controller_priv link
	 * (i3c_sec_get_basic_info attaches a temp_desc with bus + dyn_addr
	 * before issuing GETPID). Fall back to a wire-level dynamic-address
	 * lookup against peripheral emul state.
	 */
	if (!is_setdasa) {
		return i3c_emul_lookup_by_dyn_addr(dev, addr);
	}
	return NULL;
}

static int i3c_emul_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct i3c_emul_data *data = dev->data;

	switch (type) {
	case I3C_CONFIG_CONTROLLER:
		(void)memcpy(&data->common.ctrl_config, config,
			     sizeof(data->common.ctrl_config));
		return 0;
#ifdef CONFIG_I3C_TARGET
	case I3C_CONFIG_TARGET:
		(void)memcpy(&data->target_config, config, sizeof(data->target_config));
		return 0;
#endif
	default:
		return -ENOTSUP;
	}
}

static int i3c_emul_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct i3c_emul_data *data = dev->data;

	switch (type) {
	case I3C_CONFIG_CONTROLLER:
		(void)memcpy(config, &data->common.ctrl_config,
			     sizeof(data->common.ctrl_config));
		return 0;
#ifdef CONFIG_I3C_TARGET
	case I3C_CONFIG_TARGET:
		(void)memcpy(config, &data->target_config, sizeof(data->target_config));
		return 0;
#endif
	default:
		return -ENOTSUP;
	}
}

static int i3c_emul_recover_bus(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int i3c_emul_attach_i3c_device(const struct device *dev, struct i3c_device_desc *target)
{
	target->controller_priv = i3c_emul_lookup_by_pid(dev, target->pid);
	return 0;
}

static int i3c_emul_detach_i3c_device(const struct device *dev, struct i3c_device_desc *target)
{
	ARG_UNUSED(dev);

	/*
	 * detach is a controller-side bookkeeping operation: the I3C
	 * subsystem is removing this desc from its attached-devices list
	 * and freeing the address slot. It does NOT touch the peripheral
	 * on the wire — the only CCC that resets a peripheral's dynamic
	 * address is RSTDAA, which the bus emulator handles in its
	 * broadcast do_ccc path.
	 *
	 * Clear desc->controller_priv so a subsequent re-attach (e.g. via
	 * i3c_sec_handoffed reading DEFTGTS) gets a clean slate and can
	 * re-link the peripheral by walking on dynamic address.
	 */
	target->controller_priv = NULL;
	return 0;
}

static int i3c_emul_attach_i2c_device(const struct device *dev,
				      struct i3c_i2c_device_desc *target)
{
	target->controller_priv = i3c_emul_lookup_i2c_by_addr(dev, target->addr);
	return 0;
}

static int i3c_emul_detach_i2c_device(const struct device *dev,
				      struct i3c_i2c_device_desc *target)
{
	ARG_UNUSED(dev);
	target->controller_priv = NULL;
	return 0;
}

static int i3c_emul_assign_dynamic_addr(const struct device *dev, struct i3c_device_desc *desc,
					struct i3c_emul *target_emul)
{
	struct i3c_emul_data *data = dev->data;
	struct i3c_addr_slots *slots = &data->common.attached_dev.addr_slots;
	uint8_t addr;
	uint64_t pid = 0;
	uint8_t bcr = 0;
	uint8_t dcr = 0;
	int rc;

	if (desc->dynamic_addr != 0U) {
		return 0;
	}

	if (target_emul->api == NULL || target_emul->api->do_daa == NULL) {
		LOG_ERR("%s: %s cannot DAA: no do_daa handler", dev->name, desc->dev->name);
		return -ENOTSUP;
	}

	addr = desc->init_dynamic_addr;
	if (addr != 0U && !i3c_addr_slots_is_free(slots, addr)) {
		LOG_WRN("%s: requested dynamic addr 0x%02x for %s not free", dev->name, addr,
			desc->dev->name);
		addr = 0U;
	}

	if (addr == 0U) {
		addr = i3c_addr_slots_next_free_find(slots, 0U);
		if (addr == 0U) {
			LOG_ERR("%s: no free dynamic addresses", dev->name);
			return -ENOSPC;
		}
	}

	i3c_addr_slots_mark_i3c(slots, addr);

	/*
	 * Model the ENTDAA exchange: offer the assigned address; the peripheral
	 * takes ownership of its own dynamic address and reports the PID/BCR/DCR
	 * it would have shifted out during DAA. Mirror those into the descriptor,
	 * exactly as real silicon captures them from the DAA data — the subsystem
	 * skips GETBCR/GETDCR for static-less targets, relying on this.
	 */
	rc = target_emul->api->do_daa(target_emul->target, addr, &pid, &bcr, &dcr);
	if (rc == -EALREADY) {
		LOG_INF("%s: %s already had a dynamic addr; skipping", dev->name, desc->dev->name);
		i3c_addr_slots_mark_free(slots, addr);
		return 0;
	}
	if (rc != 0) {
		LOG_ERR("%s: %s rejected dynamic addr 0x%02x (%d)", dev->name, desc->dev->name,
			addr, rc);
		i3c_addr_slots_mark_free(slots, addr);
		return rc;
	}

	desc->dynamic_addr = target_emul->dynamic_addr;
	desc->pid = pid;
	desc->bcr = bcr;
	desc->dcr = dcr;

	LOG_DBG("%s: assigned dynamic addr 0x%02x to %s (bcr 0x%02x dcr 0x%02x)", dev->name,
		desc->dynamic_addr, desc->dev->name, bcr, dcr);
	return 0;
}

static int i3c_emul_do_daa(const struct device *dev)
{
	struct i3c_device_desc *desc;
	int ret = 0;

	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		struct i3c_emul *target_emul;

		if (desc->dynamic_addr != 0U) {
			continue;
		}

		target_emul = i3c_emul_for_desc(desc);
		if (target_emul == NULL) {
			continue;
		}

		ret = i3c_emul_assign_dynamic_addr(dev, desc, target_emul);
		if (ret != 0) {
			break;
		}
	}

	return ret;
}

static int i3c_emul_do_ccc_one(struct i3c_emul *target_emul, struct i3c_ccc_payload *payload)
{
	if (target_emul->mock_api != NULL && target_emul->mock_api->do_ccc != NULL) {
		int rc = target_emul->mock_api->do_ccc(target_emul->target, payload);

		if (rc != -ENOSYS) {
			return rc;
		}
	}

	if (target_emul->api == NULL || target_emul->api->do_ccc == NULL) {
		return -ENOTSUP;
	}

	return target_emul->api->do_ccc(target_emul->target, payload);
}

static int i3c_emul_do_ccc_broadcast(const struct device *dev, struct i3c_ccc_payload *payload)
{
	struct i3c_emul_data *data = dev->data;
	struct i3c_device_desc *desc;
	int ret = 0;

	/*
	 * ENTHDR0 puts the bus in HDR-DDR mode. Track that here so HDR-DDR
	 * private xfers can be gated on a prior ENTHDR0 broadcast (real
	 * controllers/targets reject HDR transfers issued in SDR mode).
	 * Broadcast doesn't need to reach peripherals — the mode entry is
	 * a bus-state transition observed by everyone implicitly.
	 */
	if (payload->ccc.id == I3C_CCC_ENTHDR0) {
		data->hdr_mode_entered = I3C_MSG_HDR_DDR;
	}

	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		struct i3c_emul *target_emul = i3c_emul_for_desc(desc);
		int rc;

		if (target_emul == NULL || target_emul->api == NULL) {
			continue;
		}

		rc = i3c_emul_do_ccc_one(target_emul, payload);
		if (rc != 0 && rc != -ENOTSUP && ret == 0) {
			ret = rc;
		}
	}

#ifdef CONFIG_I3C_TARGET
	/*
	 * If a target_cfg is registered, mirror the wire-level "DEFTGTS
	 * received as a target" path that real silicon exposes through an
	 * IRQ: deep-copy the payload into data->common.deftgts and arm
	 * deftgts_refreshed, so a subsequent controller-handoff completion
	 * can let i3c_sec_handoffed walk it.
	 *
	 * Per spec, the wire DEFTGTS payload (which payload->ccc.data is
	 * verbatim from i3c_ccc_do_deftgts_all) carries each address in
	 * the upper 7 bits of an 8-bit field. Real hardware SDCT registers
	 * de-shift on the way in, and dw / cdns / npcx all store the
	 * 7-bit form into data->common.deftgts. i3c_sec_handoffed assumes
	 * that 7-bit form. The emulator has no hardware to de-shift for
	 * us, so do it here when we mirror the payload.
	 */
	if (payload->ccc.id == I3C_CCC_DEFTGTS &&
	    data->target_cfg != NULL && payload->ccc.data != NULL &&
	    payload->ccc.data_len >= sizeof(struct i3c_ccc_deftgts)) {
		const struct i3c_ccc_deftgts *src =
			(const struct i3c_ccc_deftgts *)payload->ccc.data;
		size_t required = sizeof(struct i3c_ccc_deftgts) +
				  (size_t)src->count * sizeof(struct i3c_ccc_deftgts_target);

		/*
		 * A malformed payload that advertises more targets than it
		 * actually carries must not drive the de-shift loop past the
		 * end of the copied buffer. Reject it and leave the stale
		 * deftgts (if any) unarmed.
		 */
		if (payload->ccc.data_len < required) {
			LOG_ERR("%s: DEFTGTS payload too short (%zu < %zu)", dev->name,
				payload->ccc.data_len, required);
			data->common.deftgts_refreshed = false;
		} else {
			struct i3c_ccc_deftgts *defs;

			if (data->common.deftgts != NULL) {
				k_free(data->common.deftgts);
			}
			data->common.deftgts = k_malloc(payload->ccc.data_len);
			if (data->common.deftgts != NULL) {
				memcpy(data->common.deftgts, payload->ccc.data,
				       payload->ccc.data_len);
				defs = data->common.deftgts;
				defs->active_controller.addr >>= 1;
				defs->active_controller.static_addr >>= 1;
				for (uint8_t i = 0; i < defs->count; i++) {
					defs->targets[i].addr >>= 1;
					defs->targets[i].static_addr >>= 1;
				}
				data->common.deftgts_refreshed = true;
			} else {
				LOG_ERR("%s: DEFTGTS k_malloc(%zu) failed", dev->name,
					payload->ccc.data_len);
				data->common.deftgts_refreshed = false;
			}
		}
	}
#endif /* CONFIG_I3C_TARGET */

	return ret;
}

#ifdef CONFIG_I3C_TARGET
/*
 * Handle a GETACCCR direct CCC addressed to the locally-registered target.
 * Returns 0 on success (caller should continue to next target), or a negative
 * error code to propagate immediately.
 */
static int i3c_emul_handle_getacccr(const struct device *dev, struct i3c_emul_data *data,
				    struct i3c_ccc_target_payload *tp)
{
	if (tp->data == NULL || tp->data_len < 1U) {
		return -EINVAL;
	}
	if (!data->handoff_accept) {
		/* NACK: leave the byte at zero so the controller-side parity check fails. */
		tp->data[0] = 0;
		tp->num_xfer = 1U;
		return -ENOTCONN;
	}

	tp->data[0] = (tp->addr << 1) | i3c_odd_parity(tp->addr);
	tp->num_xfer = 1U;

	/* Wire-level handoff completed: this device just became the active
	 * controller and is no longer behaving as a target.
	 */
	data->target_config.enabled = false;

	if (data->target_cfg->callbacks != NULL &&
	    data->target_cfg->callbacks->controller_handoff_cb != NULL) {
		(void)data->target_cfg->callbacks->controller_handoff_cb(data->target_cfg);
	}

#if defined(CONFIG_I3C_USE_IBI) && defined(CONFIG_I3C_IBI_WORKQUEUE)
	/*
	 * Mirror real drivers (i3c_dw.c, i3c_cdns.c): after a completed
	 * handoff, schedule i3c_sec_handoffed via the IBI workqueue. It
	 * walks data->common.deftgts (populated when the prior active
	 * controller broadcast DEFTGTS) and re-attaches the bus topology
	 * under this newly-active controller.
	 */
	(void)i3c_ibi_work_enqueue_cb(dev, i3c_sec_handoffed);
#else
	ARG_UNUSED(dev);
#endif
	return 0;
}
#endif /* CONFIG_I3C_TARGET */

static int i3c_emul_do_ccc_direct(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const bool is_setdasa = (payload->ccc.id == I3C_CCC_SETDASA);
	int ret = 0;

	for (size_t i = 0; i < payload->targets.num_targets; i++) {
		struct i3c_ccc_target_payload *tp = &payload->targets.payloads[i];
		struct i3c_emul *target_emul;
		int rc;

#ifdef CONFIG_I3C_TARGET
		/*
		 * If the application has registered as a target at this address
		 * and the CCC is GETACCCR, handle it locally instead of routing
		 * to a peripheral emul. Other direct CCCs to the registered-target
		 * address fall through to peripheral lookup (likely -ENODEV).
		 */
		{
			struct i3c_emul_data *data = dev->data;

			if (data->target_cfg != NULL &&
			    data->target_config.dynamic_addr == tp->addr &&
			    payload->ccc.id == I3C_CCC_GETACCCR) {
				rc = i3c_emul_handle_getacccr(dev, data, tp);
				if (rc != 0) {
					return rc;
				}
				continue;
			}
		}
#endif /* CONFIG_I3C_TARGET */

		target_emul = i3c_emul_find_by_addr(dev, tp->addr, is_setdasa);
		if (target_emul == NULL) {
			ret = ret ? ret : -ENODEV;
			continue;
		}

		/* Default tp->num_xfer to 0 so callers don't read uninitialized
		 * memory if the peripheral does not touch it.
		 */
		tp->num_xfer = 0;

		rc = i3c_emul_do_ccc_one(target_emul, payload);
		if (rc != 0 && ret == 0) {
			ret = rc;
		}
	}

	return ret;
}

static int i3c_emul_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	if (payload == NULL) {
		return -EINVAL;
	}

	if (i3c_ccc_is_payload_broadcast(payload)) {
		return i3c_emul_do_ccc_broadcast(dev, payload);
	}

	return i3c_emul_do_ccc_direct(dev, payload);
}

#ifdef CONFIG_I3C_TARGET

static int i3c_emul_target_read_byte(struct i3c_target_config *cfg, uint32_t off, uint8_t *byte)
{
	const struct i3c_target_callbacks *cb = cfg->callbacks;

	if (off == 0) {
		return cb->read_requested_cb != NULL
			       ? cb->read_requested_cb(cfg, byte)
			       : -ENOSYS;
	}
	return cb->read_processed_cb != NULL ? cb->read_processed_cb(cfg, byte) : -ENOSYS;
}

#if CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE > 0
static uint32_t i3c_emul_target_drain_tx_fifo(struct i3c_emul_data *data, uint8_t *buf,
					      uint32_t len)
{
	size_t take = MIN(data->tx_fifo_len, (size_t)len);

	__ASSERT_NO_MSG(data->tx_fifo_len <= sizeof(data->tx_fifo));
	memcpy(buf, data->tx_fifo, take);
	if (take < data->tx_fifo_len) {
		memmove(data->tx_fifo, &data->tx_fifo[take], data->tx_fifo_len - take);
	}
	data->tx_fifo_len -= take;
	return (uint32_t)take;
}
#endif /* CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE > 0 */

static int i3c_emul_target_read_msg(struct i3c_emul_data *data, struct i3c_msg *m)
{
	uint32_t off = 0;
	int rc;

#if CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE > 0
	if (data->tx_fifo_len > 0U) {
		off = i3c_emul_target_drain_tx_fifo(data, m->buf, m->len);
	}
#endif
	for (; off < m->len; off++) {
		rc = i3c_emul_target_read_byte(data->target_cfg, off, &m->buf[off]);
		if (rc != 0) {
			return rc;
		}
	}
	m->num_xfer = m->len;
	return 0;
}

static int i3c_emul_target_write_msg(struct i3c_emul_data *data, struct i3c_msg *m)
{
	const struct i3c_target_callbacks *cb = data->target_cfg->callbacks;
	int rc;

	for (uint32_t j = 0; j < m->len; j++) {
		if (j == 0 && cb->write_requested_cb != NULL) {
			rc = cb->write_requested_cb(data->target_cfg);
			if (rc != 0) {
				return rc;
			}
		}
		if (cb->write_received_cb != NULL) {
			rc = cb->write_received_cb(data->target_cfg, m->buf[j]);
			if (rc != 0) {
				return rc;
			}
		}
	}
	m->num_xfer = m->len;
	return 0;
}

static int i3c_emul_target_xfer(struct i3c_emul_data *data, struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct i3c_target_callbacks *cb = data->target_cfg->callbacks;
	int rc;

	for (uint8_t i = 0; i < num_msgs; i++) {
		struct i3c_msg *m = &msgs[i];

		if ((m->flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
			rc = i3c_emul_target_read_msg(data, m);
		} else {
			rc = i3c_emul_target_write_msg(data, m);
		}
		if (rc != 0) {
			return rc;
		}
		if ((m->flags & I3C_MSG_STOP) && cb->stop_cb != NULL) {
			(void)cb->stop_cb(data->target_cfg);
		}
	}

	return 0;
}
#endif /* CONFIG_I3C_TARGET */

static int i3c_emul_xfers(const struct device *dev, struct i3c_device_desc *target,
			  struct i3c_msg *msgs, uint8_t num_msgs)
{
	struct i3c_emul *target_emul;

	if (target == NULL || msgs == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_I3C_TARGET
	{
		struct i3c_emul_data *data = dev->data;

		if (data->target_cfg != NULL &&
		    data->target_config.dynamic_addr == target->dynamic_addr) {
			return i3c_emul_target_xfer(data, msgs, num_msgs);
		}
	}
#endif

	target_emul = i3c_emul_find_by_addr(dev, target->dynamic_addr, false);
	if (target_emul == NULL) {
		LOG_DBG("%s: no emul for dynamic addr 0x%02x", dev->name, target->dynamic_addr);
		return -ENODEV;
	}

	if (target_emul->mock_api != NULL && target_emul->mock_api->xfers != NULL) {
		int rc = target_emul->mock_api->xfers(target_emul->target, msgs, num_msgs);

		if (rc != -ENOSYS) {
			return rc;
		}
	}

	/*
	 * HDR-mode private xfer prerequisites:
	 *   1. Bus must be in HDR mode (controller broadcast ENTHDR0 for
	 *      HDR-DDR). Real silicon rejects HDR ops issued in SDR mode.
	 *   2. Target must advertise HDR support (BCR bit 5 = Advanced
	 *      Capabilities). Controllers gate HDR on BCR before issuing.
	 *
	 * HDR mode is consumed by the next xfer regardless of outcome:
	 * the bus exits HDR via the HDR-EXIT pattern after each transfer,
	 * so a subsequent HDR-DDR xfer needs another ENTHDR0 broadcast.
	 *
	 * Once gated, the msgs (with their HDR flags set) flow through
	 * the same @ref xfers callback as SDR. Peripherals that care about
	 * HDR dispatch on @c msgs[i].flags & I3C_MSG_HDR themselves.
	 */
	{
		struct i3c_emul_data *data = dev->data;
		uint8_t entered = data->hdr_mode_entered;
		bool any_hdr_ddr = false;
		bool any_non_hdr_ddr_hdr = false;

		data->hdr_mode_entered = 0;

		for (uint8_t i = 0; i < num_msgs; i++) {
			if ((msgs[i].flags & I3C_MSG_HDR) == 0U) {
				continue;
			}
			if (msgs[i].hdr_mode == I3C_MSG_HDR_DDR) {
				any_hdr_ddr = true;
			} else {
				any_non_hdr_ddr_hdr = true;
			}
		}

		if (any_hdr_ddr && any_non_hdr_ddr_hdr) {
			return -EINVAL;
		}

		if (any_hdr_ddr) {
			if (entered != I3C_MSG_HDR_DDR) {
				return -EACCES;
			}
			/*
			 * Gate on the controller-side descriptor, which the
			 * controller learned over the bus: BCR/DCR via DAA
			 * (i3c_emul_api::do_daa) and the GETCAPS byte via the
			 * subsystem's post-assignment i3c_device_adv_info_get.
			 */
			if ((target->bcr & I3C_BCR_ADV_CAPABILITIES) == 0U) {
				return -ENOTSUP;
			}
			if ((target->getcaps.getcap1 & I3C_CCC_GETCAPS1_HDR_DDR) == 0U) {
				return -ENOTSUP;
			}
		}
	}

	if (target_emul->api == NULL || target_emul->api->xfers == NULL) {
		return -ENOSYS;
	}

	return target_emul->api->xfers(target_emul->target, msgs, num_msgs);
}

static struct i3c_device_desc *i3c_emul_device_find(const struct device *dev,
						    const struct i3c_device_id *id)
{
	const struct i3c_emul_config *cfg = dev->config;

	return i3c_dev_list_find(&cfg->common.dev_list, id);
}

#ifdef CONFIG_I3C_CALLBACK
static int i3c_emul_do_ccc_cb(const struct device *dev, struct i3c_ccc_payload *payload,
			      i3c_callback_t cb, void *userdata)
{
	int ret = i3c_emul_do_ccc(dev, payload);

	if (cb != NULL) {
		cb(dev, ret, userdata);
	}

	return ret;
}

static int i3c_emul_xfers_cb(const struct device *dev, struct i3c_device_desc *target,
			     struct i3c_msg *msgs, uint8_t num_msgs, i3c_callback_t cb,
			     void *userdata)
{
	int ret = i3c_emul_xfers(dev, target, msgs, num_msgs);

	if (cb != NULL) {
		cb(dev, ret, userdata);
	}

	return ret;
}
#endif /* CONFIG_I3C_CALLBACK */

#ifdef CONFIG_I3C_USE_IBI
static int i3c_emul_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_INTR };

	if (target == NULL) {
		return -EINVAL;
	}

	/*
	 * Real silicon enables a target's IBI by sending ENEC(INTR) at it
	 * on the wire. Mirror that here: the peripheral's ENEC handler
	 * will set its INTR bit, and i3c_emul_target_raise_ibi gates on
	 * that bit via the test_target wrapper.
	 */
	return i3c_ccc_do_events_set(target, true, &events);
}

static int i3c_emul_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_INTR };

	ARG_UNUSED(dev);
	if (target == NULL) {
		return -EINVAL;
	}
	return i3c_ccc_do_events_set(target, false, &events);
}

static int i3c_emul_ibi_hj_response(const struct device *dev, bool ack)
{
	struct i3c_emul_data *data = dev->data;

	data->ibi_hj_ack = ack;
	return 0;
}

static int i3c_emul_ibi_crr_response(struct i3c_device_desc *target, bool ack)
{
	struct i3c_emul_data *data;

	if (target == NULL || target->bus == NULL) {
		return -EINVAL;
	}

	data = target->bus->data;
	data->ibi_crr_ack = ack;
	return 0;
}
#endif /* CONFIG_I3C_USE_IBI */

#ifdef CONFIG_I3C_TARGET
static int i3c_emul_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	struct i3c_emul_data *data = dev->data;

	if (cfg == NULL) {
		return -EINVAL;
	}

	data->target_cfg = cfg;
	data->target_config.enabled = true;
	/* dynamic_addr is populated later by the controller's DAA / SETDASA
	 * / SETNEWDA path (i3c_target_config no longer carries an address
	 * field as of upstream commit 54ebe6f2ae5).
	 */
#if CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE > 0
	data->tx_fifo_len = 0;
#endif
	return 0;
}

static int i3c_emul_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	struct i3c_emul_data *data = dev->data;

	if (data->target_cfg != cfg) {
		return -EINVAL;
	}

	data->target_cfg = NULL;
	data->target_config.enabled = false;
	data->handoff_accept = false;
#if CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE > 0
	data->tx_fifo_len = 0;
#endif
	return 0;
}

static int i3c_emul_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len,
				    uint8_t hdr_mode)
{
	ARG_UNUSED(hdr_mode);
#if CONFIG_I3C_EMUL_TARGET_TX_FIFO_SIZE > 0
	struct i3c_emul_data *data = dev->data;
	size_t free_space;
	size_t take;

	if (data->target_cfg == NULL) {
		return -ENOTSUP;
	}
	if (len == 0U) {
		return 0;
	}
	if (buf == NULL) {
		return -EINVAL;
	}

	free_space = sizeof(data->tx_fifo) - data->tx_fifo_len;
	take = MIN(free_space, (size_t)len);
	if (take == 0U) {
		return -ENOSPC;
	}

	memcpy(&data->tx_fifo[data->tx_fifo_len], buf, take);
	data->tx_fifo_len += take;

	return (int)take;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -ENOTSUP;
#endif
}

static int i3c_emul_target_controller_handoff(const struct device *dev, bool accept)
{
	struct i3c_emul_data *data = dev->data;

	/*
	 * Per i3c_dw.c / i3c_cdns.c, controller_handoff_cb fires only when
	 * the wire-level handoff actually completes (a hardware IRQ telling
	 * us we now own the bus), not when the application opts in. This
	 * call is just the application announcing whether it would accept
	 * a future handoff offer. Stash the preference; the bus emulator
	 * fires the callback and flips target_config.enabled when it
	 * dispatches a GETACCCR direct CCC to this device's address.
	 */
	data->handoff_accept = accept;
	return 0;
}
#endif /* CONFIG_I3C_TARGET */

static int i3c_emul_i2c_api_configure(const struct device *dev, uint32_t dev_config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_config);
	return 0;
}

static int i3c_emul_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs,
				     uint8_t num_msgs, uint16_t addr)
{
	struct i3c_i2c_device_desc *desc;

	if (num_msgs == 0U) {
		return 0;
	}
	if (msgs == NULL) {
		return -EINVAL;
	}

	I3C_BUS_FOR_EACH_I2CDEV(dev, desc) {
		struct i2c_emul *emul = desc->controller_priv;

		if (desc->addr != addr || emul == NULL) {
			continue;
		}
		if (emul->mock_api != NULL && emul->mock_api->transfer != NULL) {
			int rc = emul->mock_api->transfer(emul->target, msgs, num_msgs, addr);

			if (rc != -ENOSYS) {
				return rc;
			}
		}
		return emul->api->transfer(emul->target, msgs, num_msgs, addr);
	}

	return -EIO;
}

int i3c_emul_register(const struct device *dev, struct i3c_emul *emul)
{
	emul->bus = dev;

	/*
	 * No desc->controller_priv linkup here — that happens in the
	 * attach_i3c_device hook, which can run repeatedly (initial bus
	 * init, secondary-controller handoff re-attach via i3c_sec_handoffed,
	 * etc.) and re-derives the link from the PID stored in the desc.
	 */
	LOG_INF("%s: register I3C emul '%s' (sa=0x%02x, pid=0x%012llx)", dev->name,
		emul->target->dev->name, emul->static_addr, (unsigned long long)emul->pid);

	return 0;
}

int i3c_emul_register_i2c(const struct device *dev, struct i2c_emul *emul)
{
	/*
	 * No desc->controller_priv linkup here — that happens in the
	 * attach_i2c_device hook, which the i3c subsystem invokes once the
	 * desc is added to the bus's attached_dev list. At register time
	 * the desc list is still empty.
	 */
	LOG_INF("%s: register legacy-i2c-on-i3c emul '%s' (addr=0x%02x)", dev->name,
		emul->target->dev->name, emul->addr);

	return 0;
}

bool i3c_emul_is_bus(const struct device *dev)
{
	extern const struct i3c_driver_api i3c_emul_api;

	return dev != NULL && dev->api == &i3c_emul_api;
}

int i3c_emul_target_raise_ibi(const struct emul *target, uint8_t *payload, uint8_t payload_len)
{
#ifdef CONFIG_I3C_USE_IBI
	const struct device *bus;
	struct i3c_emul *target_emul;
	struct i3c_device_desc *desc;

	if (target == NULL || target->bus_type != EMUL_BUS_TYPE_I3C) {
		return -EINVAL;
	}

	target_emul = target->bus.i3c;
	if (target_emul == NULL || target_emul->bus == NULL || target_emul->dynamic_addr == 0U) {
		return -EINVAL;
	}

	if (!(target_emul->enabled_events & I3C_CCC_EVT_INTR)) {
		return -ENOTCONN;
	}

	bus = target_emul->bus;
	desc = i3c_dev_list_i3c_addr_find(bus, target_emul->dynamic_addr);
	if (desc == NULL || desc->ibi_cb == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_I3C_IBI_WORKQUEUE
	return i3c_ibi_work_enqueue_target_irq(desc, payload, payload_len);
#else
	{
		struct i3c_ibi_payload ibi_payload = {0};

		if (payload_len > sizeof(ibi_payload.payload)) {
			payload_len = sizeof(ibi_payload.payload);
		}
		if ((payload != NULL) && (payload_len > 0U)) {
			(void)memcpy(ibi_payload.payload, payload, payload_len);
		}
		ibi_payload.payload_len = payload_len;

		return desc->ibi_cb(desc, payload_len > 0U ? &ibi_payload : NULL);
	}
#endif /* CONFIG_I3C_IBI_WORKQUEUE */
#else
	ARG_UNUSED(target);
	ARG_UNUSED(payload);
	ARG_UNUSED(payload_len);
	return -ENOSYS;
#endif /* CONFIG_I3C_USE_IBI */
}

int i3c_emul_target_raise_hj(const struct emul *target)
{
#ifdef CONFIG_I3C_USE_IBI
	struct i3c_emul *target_emul;
	struct i3c_emul_data *data;

	if (target == NULL || target->bus_type != EMUL_BUS_TYPE_I3C) {
		return -EINVAL;
	}

	target_emul = target->bus.i3c;
	if (target_emul == NULL || target_emul->bus == NULL) {
		return -EINVAL;
	}

	/*
	 * Hot-Join is only legal for devices that do not yet have a
	 * dynamic address (the whole point of HJ is to ask the active
	 * controller to run ENTDAA so this device gets one). A device
	 * with a DA must not raise HJ.
	 */
	if (target_emul->dynamic_addr != 0U) {
		return -EACCES;
	}

	if (!(target_emul->enabled_events & I3C_CCC_EVT_HJ)) {
		return -EACCES;
	}

	data = target_emul->bus->data;
	if (!data->ibi_hj_ack) {
		return -ENOTCONN;
	}

#ifdef CONFIG_I3C_IBI_WORKQUEUE
	return i3c_ibi_work_enqueue_hotjoin(target_emul->bus);
#else
	return 0;
#endif
#else
	ARG_UNUSED(target);
	return -ENOSYS;
#endif
}

int i3c_emul_target_raise_crr(const struct emul *target)
{
#ifdef CONFIG_I3C_USE_IBI
	struct i3c_emul_data *data;
	struct i3c_emul *target_emul;
	struct i3c_device_desc *desc;

	if (target == NULL || target->bus_type != EMUL_BUS_TYPE_I3C) {
		return -EINVAL;
	}

	target_emul = target->bus.i3c;
	if (target_emul == NULL || target_emul->bus == NULL || target_emul->dynamic_addr == 0U) {
		return -EINVAL;
	}

	if (!(target_emul->enabled_events & I3C_CCC_EVT_CR)) {
		return -EACCES;
	}

	data = target_emul->bus->data;
	if (!data->ibi_crr_ack) {
		return -ENOTCONN;
	}

	desc = i3c_dev_list_i3c_addr_find(target_emul->bus, target_emul->dynamic_addr);
	if (desc == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_I3C_IBI_WORKQUEUE
	return i3c_ibi_work_enqueue_controller_request(desc);
#else
	return 0;
#endif
#else
	ARG_UNUSED(target);
	return -ENOSYS;
#endif
}

static int i3c_emul_init(const struct device *dev)
{
	const struct i3c_emul_config *cfg = dev->config;
	struct i3c_emul_data *data = dev->data;
	int ret;

	data->common.ctrl_config.scl.i3c = cfg->i3c_scl_hz;
	data->common.ctrl_config.scl.i2c = cfg->i2c_scl_hz;

#ifdef CONFIG_I3C_TARGET
	/*
	 * If the DT supplied primary-controller-da, expose it as the bus
	 * controller's target-mode dynamic address so peer-addressed CCCs
	 * (GETACCCR for handoff in particular) route to a registered
	 * target_cfg. When the prop is absent, target_config.dynamic_addr
	 * stays 0 and the bus controller is a controller-only endpoint.
	 */
	data->target_config.dynamic_addr = cfg->common.primary_controller_da;
#endif

	/*
	 * Register peripheral emuls first so they set their per-emul ->bus
	 * pointer. Then run i3c_addr_slots_init: it calls
	 * i3c_attach_i3c_device for each DT-listed desc, which invokes our
	 * attach hook to PID-link desc->controller_priv to the matching
	 * peripheral. Bus init can then dispatch CCCs through that link.
	 */
	ret = emul_init_for_bus_from_list(dev, &cfg->emul_list);
	if (ret != 0) {
		return ret;
	}

	ret = i3c_addr_slots_init(dev);
	if (ret != 0) {
		return ret;
	}

	if ((cfg->common.flags & I3C_CONTROLLER_FLAG_DISABLE_BUS_INIT) == 0U) {
		ret = i3c_bus_init(dev, &cfg->common.dev_list);
		if (ret != 0) {
			LOG_WRN("%s: i3c_bus_init returned %d", dev->name, ret);
			ret = 0;
		}
	}

	return ret;
}

DEVICE_API(i3c, i3c_emul_api) = {
	.i2c_api = {
		.configure = i3c_emul_i2c_api_configure,
		.transfer = i3c_emul_i2c_api_transfer,
#ifdef CONFIG_I2C_RTIO
		.iodev_submit = i2c_iodev_submit_fallback,
#endif
	},
	.configure = i3c_emul_configure,
	.config_get = i3c_emul_config_get,
	.recover_bus = i3c_emul_recover_bus,
	.attach_i3c_device = i3c_emul_attach_i3c_device,
	.detach_i3c_device = i3c_emul_detach_i3c_device,
	.attach_i2c_device = i3c_emul_attach_i2c_device,
	.detach_i2c_device = i3c_emul_detach_i2c_device,
	.do_daa = i3c_emul_do_daa,
	.do_ccc = i3c_emul_do_ccc,
	.i3c_xfers = i3c_emul_xfers,
	.i3c_device_find = i3c_emul_device_find,
#ifdef CONFIG_I3C_CALLBACK
	.do_ccc_cb = i3c_emul_do_ccc_cb,
	.i3c_xfers_cb = i3c_emul_xfers_cb,
#endif
#ifdef CONFIG_I3C_USE_IBI
	.ibi_enable = i3c_emul_ibi_enable,
	.ibi_disable = i3c_emul_ibi_disable,
	.ibi_hj_response = i3c_emul_ibi_hj_response,
	.ibi_crr_response = i3c_emul_ibi_crr_response,
#endif
#ifdef CONFIG_I3C_TARGET
	.target_register = i3c_emul_target_register,
	.target_unregister = i3c_emul_target_unregister,
	.target_tx_write = i3c_emul_target_tx_write,
	.target_controller_handoff = i3c_emul_target_controller_handoff,
#endif
#ifdef CONFIG_I3C_RTIO
	.iodev_submit = i3c_iodev_submit_fallback,
#endif
};

#define EMUL_LINK_AND_COMMA(node_id)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	},

#define I3C_EMUL_INIT(n)                                                                           \
	static struct i3c_device_desc i3c_emul_i3c_dev_arr_##n[] = I3C_DEVICE_ARRAY_DT_INST(n);    \
	static struct i3c_i2c_device_desc i3c_emul_i2c_dev_arr_##n[] =                             \
		I3C_I2C_DEVICE_ARRAY_DT_INST(n);                                                   \
	static const struct emul_link_for_bus i3c_emul_children_##n[] = {                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, EMUL_LINK_AND_COMMA)};                        \
	static const struct i3c_emul_config i3c_emul_cfg_##n = {                                   \
		.common =                                                                          \
			{                                                                          \
				.dev_list =                                                        \
					{                                                          \
						.i3c = i3c_emul_i3c_dev_arr_##n,                   \
						.num_i3c = ARRAY_SIZE(i3c_emul_i3c_dev_arr_##n),   \
						.i2c = i3c_emul_i2c_dev_arr_##n,                   \
						.num_i2c = ARRAY_SIZE(i3c_emul_i2c_dev_arr_##n),   \
					},                                                         \
				.flags = I3C_CONTROLLER_CONFIG_FLAGS_DT_INST(n),                   \
				.primary_controller_da =                                           \
					DT_INST_PROP_OR(n, primary_controller_da, 0x00),           \
			},                                                                         \
		.emul_list =                                                                       \
			{                                                                          \
				.children = i3c_emul_children_##n,                                 \
				.num_children = ARRAY_SIZE(i3c_emul_children_##n),                 \
			},                                                                         \
		.i3c_scl_hz = DT_INST_PROP_OR(n, i3c_scl_hz, 0),                                   \
		.i2c_scl_hz = DT_INST_PROP_OR(n, i2c_scl_hz, 0),                                   \
	};                                                                                         \
	static struct i3c_emul_data i3c_emul_data_##n;                                             \
	DEVICE_DT_INST_DEFINE(n, i3c_emul_init, NULL, &i3c_emul_data_##n, &i3c_emul_cfg_##n,       \
			      POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY, &i3c_emul_api);

DT_INST_FOREACH_STATUS_OKAY(I3C_EMUL_INIT)
