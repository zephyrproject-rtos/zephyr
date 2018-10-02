/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_DUMMY_H__
#define SHELL_DUMMY_H__

#include <shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_dummy_transport_api;

struct shell_dummy {
	bool initialized;
};

#define SHELL_DUMMY_DEFINE(_name)					\
	static struct shell_dummy _name##_shell_dummy;			\
	struct shell_transport _name = {				\
		.api = &shell_dummy_transport_api,			\
		.ctx = (struct shell_dummy *)&_name##_shell_dummy	\
	}

/**
 * @brief This function shall not be used directly. It provides pointer to shell
 *	  dummy backend instance.
 *
 * Function returns pointer to the shell dummy instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_dummy_get_ptr(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_DUMMY_H__ */
