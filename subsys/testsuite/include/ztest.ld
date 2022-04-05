/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_ARM_MMU)
#define _ZTEST_ALIGN CONFIG_MMU_PAGE_SIZE
#elif defined(CONFIG_ARM_MPU)
#define _ZTEST_ALIGN CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#elif defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)
#define _ZTEST_ALIGN 16
#else
#define _ZTEST_ALIGN 4
#endif

#if defined(CONFIG_ZTEST)
Z_LINK_ITERABLE_ALIGNED(ztest_suite_node, _ZTEST_ALIGN);

#if defined(CONFIG_ZTEST_NEW_API)
Z_LINK_ITERABLE_ALIGNED(ztest_unit_test, _ZTEST_ALIGN);
Z_LINK_ITERABLE_ALIGNED(ztest_test_rule, _ZTEST_ALIGN);
#endif /* CONFIG_ZTEST_NEW_API */

#endif /* CONFIG_ZTEST */
