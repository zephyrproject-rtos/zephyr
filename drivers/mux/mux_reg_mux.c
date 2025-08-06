/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT reg_mux

#include <zephyr/kernel.h>
#include <zephyr/drivers/mux.h>

struct reg_mux_control_spec {
	uint32_t mux;
};

struct reg_mux_masks_entry {
	uint32_t offset;
	uint32_t mask;
};

struct reg_mux_config {
	DEVICE_MMIO_ROM;
	const struct reg_mux_masks_entry *muxes;
	size_t num_muxes;
};

struct reg_mux_data {
	struct k_mutex lock;
};

static const struct reg_mux_masks_entry *reg_mux_entry_get(const struct device *dev,
					const struct reg_mux_control_spec *control)
{
	const struct reg_mux_config *config = dev->config;
	uint32_t mux_idx = control->mux;

	if (mux_idx >= config->num_muxes) {
		return NULL;
	}

	return &config->muxes[mux_idx];
}

static uint32_t *reg_mux_addr_get(const struct device *dev, const struct reg_mux_masks_entry *mux)
{
	uint32_t base = DEVICE_MMIO_GET(dev);
	uint32_t offset = mux->offset;

	return (uint32_t *)(base + offset);
}

static uint32_t reg_mux_mask_get(const struct reg_mux_masks_entry *mux)
{
	return mux->mask;
}

static int reg_mux_state_get(const struct device *dev, struct mux_control *mux, uint32_t *state)
{
	struct reg_mux_data *data = dev->data;
	/* cells[0] is always the mux channel index, matching reg_mux_control_spec layout */
	const struct reg_mux_control_spec *control =
		(const struct reg_mux_control_spec *)mux->cells;
	const struct reg_mux_masks_entry *entry = reg_mux_entry_get(dev, control);

	if (entry == NULL) {
		return -EINVAL;
	}

	volatile uint32_t *addr = reg_mux_addr_get(dev, entry);
	uint32_t mask = reg_mux_mask_get(entry);

	k_mutex_lock(&data->lock, K_FOREVER);
	*state = FIELD_GET(mask, *addr);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int reg_mux_configure(const struct device *dev, struct mux_control *mux, uint32_t state)
{
	struct reg_mux_data *data = dev->data;
	/* cells[0] is always the mux channel index, matching reg_mux_control_spec layout */
	const struct reg_mux_control_spec *control =
		(const struct reg_mux_control_spec *)mux->cells;
	const struct reg_mux_masks_entry *entry = reg_mux_entry_get(dev, control);

	if (entry == NULL) {
		return -EINVAL;
	}

	volatile uint32_t *addr = reg_mux_addr_get(dev, entry);
	uint32_t mask = reg_mux_mask_get(entry);
	uint32_t read_back;

	k_mutex_lock(&data->lock, K_FOREVER);
	*addr = (*addr & ~mask) | FIELD_PREP(mask, state);
	/* Inline readback under the same lock to avoid recursive locking */
	read_back = FIELD_GET(mask, *addr);
	k_mutex_unlock(&data->lock);

	if (state != read_back) {
		/* maybe register is not powered ? */
		return -EIO;
	}

	return 0;
}

static int reg_mux_init(const struct device *dev)
{
	struct reg_mux_data *data = dev->data;

	k_mutex_init(&data->lock);

	return 0;
}

static DEVICE_API(mux_control, reg_mux_driver_api) = {
	.configure = reg_mux_configure,
	.state_get = reg_mux_state_get,
};

/*
 * Union-based initialisation: the DT mux-reg-masks property is a flat
 * <offset mask offset mask ...> uint32_t array.  Initialise via the .raw
 * member (uint32_t[]) and access the hardware table through the .entries
 * member (struct reg_mux_masks_entry[]).  Union type-punning between members
 * of the same union is defined behaviour in C99/C11 (unlike pointer casting).
 * The BUILD_ASSERT below enforces that struct reg_mux_masks_entry has no
 * padding, so the two interpretations are guaranteed to be bit-compatible.
 */
#define REG_MUX_INIT(n)									\
	static const union {								\
		uint32_t raw[DT_INST_PROP_LEN(n, mux_reg_masks)];			\
		struct reg_mux_masks_entry					\
			entries[DT_INST_PROP_LEN(n, mux_reg_masks) / 2];	\
	} reg_mux_masks_##n = {							\
		.raw = {								\
			DT_INST_FOREACH_PROP_ELEM_SEP(n, mux_reg_masks,	\
						      DT_PROP_BY_IDX, (,))	\
		}									\
	};										\
	static const struct reg_mux_config reg_mux_##n##_config = {			\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.muxes = reg_mux_masks_##n.entries,					\
		.num_muxes = DT_INST_PROP_LEN(n, mux_reg_masks) / 2,			\
	};										\
	static struct reg_mux_data reg_mux_data_##n;					\
	DEVICE_DT_INST_DEFINE(n, &reg_mux_init, NULL,					\
			      &reg_mux_data_##n, &reg_mux_##n##_config,		\
			      PRE_KERNEL_1, CONFIG_MUX_CONTROL_INIT_PRIORITY,		\
			      &reg_mux_driver_api);

BUILD_ASSERT(sizeof(struct reg_mux_masks_entry) == 2 * sizeof(uint32_t),
	     "struct reg_mux_masks_entry must be exactly two uint32_t fields with no padding");

DT_INST_FOREACH_STATUS_OKAY(REG_MUX_INIT);
