/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CRYPTO_H_
#define MOCKS_CRYPTO_H_

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define CRYPTO_FFF_FAKES_LIST(FAKE) FAKE(bt_rand)

DECLARE_FAKE_VALUE_FUNC(int, bt_rand, void *, size_t);

#endif /* MOCKS_CRYPTO_H_ */
