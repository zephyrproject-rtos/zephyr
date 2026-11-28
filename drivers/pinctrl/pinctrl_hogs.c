/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Axonne Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pinctrl_hogs, CONFIG_PINCTRL_LOG_LEVEL);

#if defined(CONFIG_LOG)
#define PINCTRL_HOG_LOG(level, hog, fmt, ...)                                                      \
	LOG_##level("%s: " fmt, (hog)->name, ##__VA_ARGS__)
#else
#define PINCTRL_HOG_LOG(...)
#endif

#if defined(CONFIG_PINCTRL_HOGS_INIT_PRE_KERNEL_1)
#define PINCTRL_HOGS_INIT_LEVEL PRE_KERNEL_1
#elif defined(CONFIG_PINCTRL_HOGS_INIT_PRE_KERNEL_2)
#define PINCTRL_HOGS_INIT_LEVEL PRE_KERNEL_2
#else
#define PINCTRL_HOGS_INIT_LEVEL POST_KERNEL
#endif

struct pinctrl_hog {
#if defined(CONFIG_LOG)
	const char *name;
#endif
	const pinctrl_soc_pin_t *pins;
	uint8_t pin_cnt;
};

/* A pin controller consumes a default state defined beneath itself. */
#define PINCTRL_HOG_MATCH(idx, node_id)                                                            \
	COND_CODE_1(DT_SAME_NODE(node_id,                                                          \
		DT_PARENT(DT_PINCTRL_BY_NAME(node_id, default, idx))), (1), ())

#define PINCTRL_HOG_ENABLED(node_id)                                                               \
	COND_CODE_1(DT_PINCTRL_HAS_NAME(node_id, default),                                         \
		(UTIL_NOT(IS_EMPTY(LISTIFY(DT_NUM_PINCTRLS_BY_NAME(node_id, default),              \
			PINCTRL_HOG_MATCH, (), node_id)))), (0))

#define PINCTRL_HOG_PROP(node_id)                                                                  \
	UTIL_CAT(pinctrl_, DT_PINCTRL_NAME_TO_IDX(node_id, default))

#define PINCTRL_HOG_PINS(node_id)                                                                  \
	UTIL_CAT(pinctrl_hog_pins_, DT_DEP_ORD(node_id))

#define PINCTRL_HOG_DEFINE(node_id)                                                                \
	COND_CODE_1(PINCTRL_HOG_ENABLED(node_id),                                                  \
		(static const pinctrl_soc_pin_t PINCTRL_HOG_PINS(node_id)[] =                      \
			Z_PINCTRL_STATE_PINS_INIT(node_id, PINCTRL_HOG_PROP(node_id));             \
		 BUILD_ASSERT(ARRAY_SIZE(PINCTRL_HOG_PINS(node_id)) <= UINT8_MAX,                  \
			      "Too many pins in the controller default state");), ())

DT_FOREACH_STATUS_OKAY_NODE(PINCTRL_HOG_DEFINE)

/* clang-format off */
#define PINCTRL_HOG_ENTRY(node_id)                                                                 \
	{                                                                                          \
		IF_ENABLED(CONFIG_LOG, (.name = DT_NODE_FULL_NAME(node_id),))                      \
		.pins = PINCTRL_HOG_PINS(node_id),                                                 \
		.pin_cnt = ARRAY_SIZE(PINCTRL_HOG_PINS(node_id)),                                  \
	},

#define PINCTRL_HOG_ENTRY_COND(node_id)                                                            \
	COND_CODE_1(PINCTRL_HOG_ENABLED(node_id), (PINCTRL_HOG_ENTRY(node_id)), ())

static const struct pinctrl_hog pinctrl_hogs[] = {
	DT_FOREACH_STATUS_OKAY_NODE(PINCTRL_HOG_ENTRY_COND)
};
/* clang-format on */

static int pinctrl_hogs_init(void)
{
	const struct pinctrl_hog *hog;
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(pinctrl_hogs); i++) {
		hog = &pinctrl_hogs[i];

		err = pinctrl_configure_pins(hog->pins, hog->pin_cnt, PINCTRL_REG_NONE);
		if (err < 0) {
			PINCTRL_HOG_LOG(ERR, hog, "Failed to configure pins (err %d)", err);
			return err;
		}

		PINCTRL_HOG_LOG(DBG, hog, "Applied default state");
	}

	return 0;
}

SYS_INIT(pinctrl_hogs_init, PINCTRL_HOGS_INIT_LEVEL, CONFIG_PINCTRL_HOGS_INIT_PRIORITY);
