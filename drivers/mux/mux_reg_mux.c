/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT reg_mux

#include <zephyr/drivers/mux.h>

struct reg_mux_control_spec {
        uint32_t mux;
};

struct reg_mux_states_spec {
	uint32_t mux;
	uint32_t state;
};

union reg_mux_spec {
	struct reg_mux_control_spec controls;
	struct reg_mux_states_spec states;
};

struct reg_mux_masks_entry {
	uint32_t offset;
	uint32_t mask;
};

struct reg_mux_config {
        DEVICE_MMIO_ROM;
	struct reg_mux_masks_entry *muxes;
};

static struct reg_mux_masks_entry *reg_mux_entry_get(const struct device *dev,
						     const struct reg_mux_control_spec *control)
{
	const struct reg_mux_config *config = dev->config;
        uint32_t mux_idx = control->mux;

	return &config->muxes[mux_idx];
}

static uint32_t *reg_mux_addr_get(const struct device *dev, const struct reg_mux_masks_entry *mux)
{
        uint32_t base = DEVICE_MMIO_GET(dev);
        uint32_t offset = mux->offset;

	return (uint32_t*)(base + offset);
}

static uint32_t reg_mux_mask_get(const struct reg_mux_masks_entry *mux)
{
	return mux->mask;
}

static int reg_mux_state_get(const struct device *dev, mux_control_t mux, mux_state_t *state)
{
	/* we don't need the state cell regardless so we can just use this as a control spec */
        struct reg_mux_control_spec *control = (struct reg_mux_control_spec *)mux->control;
	struct reg_mux_masks_entry *entry = reg_mux_entry_get(dev, control);

	volatile uint32_t *addr = reg_mux_addr_get(dev, entry);
	uint32_t mask = reg_mux_mask_get(entry);

        *state = FIELD_GET(mask, *addr);

	return 0;
}

static int reg_mux_configure(const struct device *dev, mux_control_t mux)
{
	if (!mux->has_state_cell) {
		/* for this function, we need the state cell, if this spec doesn't have it
		 * then rest of this function could access some memory not part of the spec
		 * where the state cell would otherwise be, so error before doing that.
		 */
		return -EINVAL;
	}

	union reg_mux_spec *spec = (union reg_mux_spec *)mux->control;

	/* at this point we know it is a states spec because we checked */
        struct reg_mux_states_spec *state_spec = &spec->states;

	struct reg_mux_masks_entry *entry = reg_mux_entry_get(dev, &spec->controls);

	volatile uint32_t *addr = reg_mux_addr_get(dev, entry);
        uint32_t mask = reg_mux_mask_get(entry);
	uint32_t state = state_spec->state;
        uint32_t val = FIELD_PREP(mask, state);

	*addr &= ~mask;
	*addr |= val;

        mux_state_t read_back;

        reg_mux_state_get(dev, mux, &read_back);
        if (state != read_back) {
                /* maybe register is not powered ? */
                return -EIO;
        }

        return 0;
}

static DEVICE_API(mux_control, reg_mux_driver_api) = {
        .configure = reg_mux_configure,
        .state_get = reg_mux_state_get,
};

#define REG_MUX_INIT(n)                                                         	\
	uint32_t reg_mux_masks[] = { 							\
		DT_INST_FOREACH_PROP_ELEM_SEP(n, mux_reg_masks, DT_PROP_BY_IDX, (,))	\
	};										\
        static const struct reg_mux_config reg_mux_##n##_config = {             	\
                DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                           	\
		.muxes = (struct reg_mux_masks_entry *)&reg_mux_masks,			\
        };                                                                      	\
        DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &reg_mux_##n##_config,       	\
                                PRE_KERNEL_1, CONFIG_MUX_CONTROL_INIT_PRIORITY,         \
                                &reg_mux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(REG_MUX_INIT);
