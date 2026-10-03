/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_BUILD_ID_H
#define TEST_BUILD_ID_H

/*
 * What CONFIG_LOG_BACKEND_FLASH_BUILD_ID evaluates to in this test, reached
 * through CONFIG_LOG_BACKEND_FLASH_BUILD_ID_HEADER the same way an application
 * would reach its own version string.
 */
#define TEST_BUILD_ID "log_backend_flash test build"

/** A build id that is not the one the running firmware carries. */
#define TEST_FOREIGN_BUILD_ID "some other firmware"

#endif /* TEST_BUILD_ID_H */
