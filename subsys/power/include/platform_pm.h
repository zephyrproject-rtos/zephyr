/*
 * Copyright (c) 2020 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PLATFORM_PM_H_
#define _PLATFORM_PM_H_

#include <string.h>
#include <sys/util.h>
#include <power_state.h>
#include <toolchain/common.h>

#ifdef __cplusplus
extern "C" {
#endif

struct platform_pm;

/**
 * @brief Platform power management API.
 */
struct platform_pm_api {
	/* initialize platform power management */
	void (*init)(void);

	/* platform resume after successful suspend */
	void (*resume)(pm_state_t state);

	/* callback used to enter platform's pm_state */
	int (*suspend)(pm_state_t state);
};

/**
 * @brief Platform power management structure.
 */
struct platform_pm {
	const char *name;
	struct platform_pm_api *api;
};

/**
 * @brief Create platform_pm instance.
 *
 * @param _name Instance name.
 * @param _api Power management policy API.
 */
#define PLATFORM_PM_DEFINE(_name, _api) \
	static const Z_STRUCT_SECTION_ITERABLE(platform_pm, _name) = \
	{ \
		.name = STRINGIFY(_name), \
		.api = &_api \
	}

/**
 * @brief Initialize platform power management.
 *
 * @param platform Pointer to platform_pm instance.
 */
static inline void platform_pm_init(struct platform_pm *platform)
{
	if (platform && platform->api) {
		platform->api->init();
	}
}

/**
 * @brief Resume platform from pm_state.
 *
 * @param platform Pointer to platform_pm instance.
 * @param state Platform's current power state.
 */
static inline void platform_pm_resume(struct platform_pm *platform,
				      pm_state_t state);
{
	if (platform && platform->api) {
		platform->api->resume(state & PM_STATE_BIT_MASK);
	}
}

/**
 * @brief Enter platform's pm_state.
 *
 * @param platform Pointer to platform_pm instance.
 * @param state Power state platform going to.
 *
 * @return 0 if successful, -1 otherwise.
 */
static inline int platform_pm_suspend(struct platform_pm *platform,
				      pm_state_t state);
{
	if (platform && platform->api) {
		return platform->api->suspend(state & PM_STATE_BIT_MASK);
	}

	return -1;
}

/**
 * @brief Get platform power management based on the name
 *        of platform_pm in platform power management section.
 *
 * @param name Name of wanted platform_pm.
 *
 * @return Pointer of the wanted platform_pm or NULL.
 */
static inline struct platform_pm *platform_pm_get(char *name)
{
	Z_STRUCT_SECTION_FOREACH(platform_pm, platform) {
		if (strcmp(platform->name, name) == 0) {
			return platform;
		}
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
