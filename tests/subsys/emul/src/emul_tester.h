/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_SUBSYS_EMUL_SRC_EMUL_TESTER_H_
#define TEST_SUBSYS_EMUL_SRC_EMUL_TESTER_H_

#include <zephyr/drivers/emul.h>

struct emul_tester_backend_api {
	int (*set_action)(const struct emul *target, int action);
	int (*get_action)(const struct emul *target, int *action);
};

static inline int emul_tester_backend_set_action(const struct emul *target, int action)
{
	const struct emul_tester_backend_api *api =
		(const struct emul_tester_backend_api *)target->backend_api;

	return api->set_action(target, action);
}

static inline int emul_tester_backend_get_action(const struct emul *target, int *action)
{
	const struct emul_tester_backend_api *api =
		(const struct emul_tester_backend_api *)target->backend_api;

	return api->get_action(target, action);
}

#endif /* TEST_SUBSYS_EMUL_SRC_EMUL_TESTER_H_ */
