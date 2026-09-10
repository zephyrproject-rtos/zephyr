/*
 * Copyright (c) 2018 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"

ZTEST_SUITE(posix_fs_test, NULL, test_mount, NULL, NULL, test_unmount);
