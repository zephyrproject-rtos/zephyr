/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_TC_SRC_STATES_INTERNAL_H_
#define ZEPHYR_SUBSYS_USBC_TC_SRC_STATES_INTERNAL_H_

/**
 * @brief Unattached.SRC
 */
void tc_unattached_src_entry(void *obj);
void tc_unattached_src_run(void *obj);

/**
 * @brief UnattachedWait.SRC
 */
void tc_unattached_wait_src_entry(void *obj);
void tc_unattached_wait_src_run(void *obj);
void tc_unattached_wait_src_exit(void *obj);

/**
 * @brief AttachWait.SRC
 */
void tc_attach_wait_src_entry(void *obj);
void tc_attach_wait_src_run(void *obj);
void tc_attach_wait_src_exit(void *obj);

/**
 * @brief Attached.SRC
 */
void tc_attached_src_entry(void *obj);
void tc_attached_src_run(void *obj);
void tc_attached_src_exit(void *obj);

/**
 * @brief Super State to set RP
 */
void tc_cc_rp_entry(void *obj);
#endif /* ZEPHYR_SUBSYS_USBC_TC_SRC_STATES_INTERNAL_H_ */
