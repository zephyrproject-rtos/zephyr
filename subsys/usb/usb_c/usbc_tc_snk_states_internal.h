/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_TC_SNK_STATES_H_
#define ZEPHYR_SUBSYS_USBC_TC_SNK_STATES_H_

/**
 * @brief Unattached.SNK
 */
void tc_unattached_snk_entry(void *obj);
void tc_unattached_snk_run(void *obj);

/**
 * @brief AttachWait.SNK
 */
void tc_attach_wait_snk_entry(void *obj);
void tc_attach_wait_snk_run(void *obj);
void tc_attach_wait_snk_exit(void *obj);

/**
 * @brief Attached.SNK
 */
void tc_attached_snk_entry(void *obj);
void tc_attached_snk_run(void *obj);
void tc_attached_snk_exit(void *obj);

/**
 * @brief Super state that applies Rd
 */
void tc_cc_rd_entry(void *obj);

#endif /* ZEPHYR_SUBSYS_USBC_TC_SNK_STATES_H_ */
