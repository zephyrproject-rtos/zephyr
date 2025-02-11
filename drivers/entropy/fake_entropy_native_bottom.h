/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef DRIVERS_ENTROPY_FAKE_ENTROPY_NATIVE_BOTTOM_H
#define DRIVERS_ENTROPY_FAKE_ENTROPY_NATIVE_BOTTOM_H

#ifdef __cplusplus
extern "C" {
#endif

void entropy_native_seed(unsigned int seed, bool seed_random);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_ENTROPY_FAKE_ENTROPY_NATIVE_BOTTOM_H */
