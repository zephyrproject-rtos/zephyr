/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Devicetree based selection of the devices under test
 */

#ifndef ZEPHYR_SUBSYS_TESTSUITE_INCLUDE_ZEPHYR_TEST_DEVICES_H_
#define ZEPHYR_SUBSYS_TESTSUITE_INCLUDE_ZEPHYR_TEST_DEVICES_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/**
 * @brief Devices under test
 * @defgroup test_devices Devices under test
 * @ingroup ztest
 *
 * A test suite exercising a class of devices lets the board select which
 * instances to exercise, through a phandle list on the `/zephyr,user` node
 * named after the device class:
 *
 * @code{.dts}
 * / {
 *         zephyr,user {
 *                 dma-test-devs = <&edma3>, <&edma4>;
 *         };
 * };
 * @endcode
 *
 * The test source refers to that list by its lowercase-and-underscores form,
 * and generates one test case per listed instance, so that a failure or skip
 * on one instance neither masks nor blocks the remaining ones:
 *
 * @code{.c}
 * TEST_DEVS_REQUIRE(dma_test_devs);
 *
 * static const struct device *const dma_devs[] = TEST_DEVS_ARRAY(dma_test_devs);
 *
 * #define DEFINE_DMA_TESTS(idx, prop)                                                        \
 *         ZTEST(dma_m2m, test_dma##idx##_m2m)                                                \
 *         {                                                                                  \
 *                 run_test(dma_devs[idx]);                                                   \
 *         }
 *
 * TEST_DEVS_FOR_EACH_IDX(dma_test_devs, DEFINE_DMA_TESTS)
 * @endcode
 *
 * Twister selects the boards providing the list with:
 *
 * @code{.yaml}
 * filter: dt_node_has_prop("/zephyr,user", "dma-test-devs")
 * @endcode
 *
 * @{
 */

/** @brief Node holding the lists of devices under test. */
#define TEST_DEVS_NODE DT_PATH(zephyr_user)

/**
 * @brief Check whether the board selects any device under test for @p prop.
 *
 * @param prop lowercase-and-underscores name of the device list.
 *
 * @return 1 if the list is present, 0 otherwise.
 */
#define TEST_DEVS_EXIST(prop) DT_NODE_HAS_PROP(TEST_DEVS_NODE, prop)

/**
 * @brief Require the board to select the devices under test for @p prop.
 *
 * Use at file scope in a suite that cannot run without the list. A board
 * missing it then fails with a readable message, instead of an error coming
 * out of the devicetree macro expansion.
 *
 * @param prop lowercase-and-underscores name of the device list.
 */
#define TEST_DEVS_REQUIRE(prop)                                                                    \
	BUILD_ASSERT(TEST_DEVS_EXIST(prop),                                                        \
		     "this test requires the board to list the devices under test in a "           \
		     "phandle list on /zephyr,user, named after the device class")

/**
 * @brief Number of devices under test selected by @p prop.
 *
 * @param prop lowercase-and-underscores name of the device list.
 *
 * @return the number of devices in the list.
 */
#define TEST_DEVS_LEN(prop) DT_PROP_LEN(TEST_DEVS_NODE, prop)

/**
 * @brief Node identifier of a device under test.
 *
 * @param prop lowercase-and-underscores name of the device list.
 * @param idx index into the list.
 *
 * @return the node identifier of the device at index @p idx.
 */
#define TEST_DEVS_NODE_BY_IDX(prop, idx) DT_PHANDLE_BY_IDX(TEST_DEVS_NODE, prop, idx)

/**
 * @brief Device pointer of a device under test.
 *
 * @param prop lowercase-and-underscores name of the device list.
 * @param idx index into the list.
 *
 * @return a pointer to the `struct device` at index @p idx.
 */
#define TEST_DEVS_GET_BY_IDX(prop, idx) DEVICE_DT_GET(TEST_DEVS_NODE_BY_IDX(prop, idx))

/** @cond INTERNAL_HIDDEN */
#define Z_TEST_DEVS_GET_BY_IDX(idx, prop) TEST_DEVS_GET_BY_IDX(prop, idx)
/** @endcond */

/**
 * @brief Static initializer for an array holding every device under test.
 *
 * @param prop lowercase-and-underscores name of the device list.
 */
#define TEST_DEVS_ARRAY(prop)                                                                      \
	{                                                                                          \
		LISTIFY(TEST_DEVS_LEN(prop), Z_TEST_DEVS_GET_BY_IDX, (,), prop)                    \
	}

/**
 * @brief Expand @p fn once per device under test.
 *
 * @p fn is called as `fn(idx, prop)`, where `idx` is the index of the device
 * in the list. Use it to generate one test case per instance, so that the
 * failing instance is identifiable from the test case name.
 *
 * @param prop lowercase-and-underscores name of the device list.
 * @param fn macro to expand for each index.
 */
#define TEST_DEVS_FOR_EACH_IDX(prop, fn) LISTIFY(TEST_DEVS_LEN(prop), fn, (), prop)

/**
 * @}
 */

#endif /* ZEPHYR_SUBSYS_TESTSUITE_INCLUDE_ZEPHYR_TEST_DEVICES_H_ */
