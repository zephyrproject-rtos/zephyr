/** @file
 *  @brief Bluetooth Decision-Based Advertising Filtering
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_DECISION_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_DECISION_H_

/**
 * @brief LE Decision-Based Advertising Filtering (DBAF)
 * @defgroup bt_le_decision Decision-Based Advertising Filtering
 * @ingroup bluetooth
 * @{
 *
 * An advertising set created with @ref BT_LE_ADV_OPT_DECISION_PDUS starts each advertising
 * event with a decision PDU that carries a few octets of decision data. Scanners and initiators
 * that enable decision processing apply their decision instructions to that data, and receive
 * the rest of the advertisement only if the instructions accept it. Scanners that do not enable
 * decision processing ignore such advertising sets.
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum length of the decision data of an advertising set, in octets. */
#define BT_LE_DECISION_DATA_MAX_LEN BT_HCI_LE_DECISION_DATA_LEN_MAX

/** Size of a Resolvable Tag key in octets. */
#define BT_LE_DECISION_KEY_SIZE 16

/**
 * @brief Size of the mask and the target of a test on the arbitrary data, in octets.
 *
 * A scanner tests the arbitrary data in a decision PDU by padding it with zeros to this length,
 * applying a bitwise AND with the mask, and comparing the result with the target. The format of
 * the test in the HCI command fixes this size, so a single test can check at most this many
 * octets of arbitrary data.
 */
#define BT_LE_DECISION_TEST_DATA_LEN BT_HCI_LE_DECISION_TEST_DATA_LEN

/**
 * @brief Maximum number of tests in the decision instructions, limited by the HCI command size.
 *
 * Controllers only have to support 8 tests, so more than 8 can fail on some Controllers.
 */
#define BT_LE_DECISION_TESTS_MAX                                                                   \
	((UINT8_MAX - sizeof(struct bt_hci_cp_le_set_decision_instructions)) /                    \
	 sizeof(struct bt_hci_le_decision_test))

/**
 * @brief Decision data of an advertising set.
 *
 * Members that are zero or NULL leave the corresponding subfield out of the decision data,
 * so the struct should be zero-initialized, for example with designated initializers.
 */
struct bt_le_decision_data {
	/**
	 * @brief Key used to generate a Resolvable Tag, or NULL for no tag.
	 *
	 * @ref BT_LE_DECISION_KEY_SIZE octets, in the byte order used for keys in HCI commands.
	 * Scanners holding the same key accept the tag with @ref BT_LE_DECISION_TEST_TAG.
	 */
	const uint8_t *key;

	/** Arbitrary data, placed after all other subfields. */
	const uint8_t *data;

	/** Length of @ref data in octets. */
	uint8_t data_len;
};

/**
 * @brief Set the decision data of an advertising set.
 *
 * The host copies @p param, so it does not need to remain valid after the call.
 *
 * When @ref bt_le_decision_data.key is set, the host generates a new Resolvable Tag each time
 * the advertising set gets a new private address. The new address and the new tag first appear
 * in the same advertising event, so a tag cannot be used to link one address to the next.
 *
 * Changing the decision data does not change the advertising address. To change both together,
 * set the decision data from the @ref bt_le_ext_adv_cb.rpa_expired callback.
 *
 * If the Resolvable Tag cannot be regenerated when the advertising set gets a new private
 * address, the host leaves the set disabled rather than advertising the new address with the
 * old tag. Use @ref bt_le_ext_adv_get_info to check whether the set is still enabled.
 *
 * @kconfig_dep{CONFIG_BT_DECISION_BASED_ADV_FILTERING}
 *
 * @param adv   Advertising set.
 * @param param Decision data.
 *
 * @return Zero on success or (negative) error code otherwise.
 * @retval -EINVAL The subfields do not fit in @ref BT_LE_DECISION_DATA_MAX_LEN octets.
 * @retval -ENOTSUP The Controller does not support Decision-Based Advertising Filtering, or
 *                  the advertising set uses a Controller-generated address, which the
 *                  Resolvable Tag could not follow.
 */
int bt_le_ext_adv_set_decision_data(struct bt_le_ext_adv *adv,
				    const struct bt_le_decision_data *param);

/** Fields of a decision PDU that a test can check. */
enum bt_le_decision_field {
	/** Resolvable Tag. The check passes if the tag resolves against a key. */
	BT_LE_DECISION_FIELD_TAG = BT_HCI_LE_DECISION_TEST_FIELD_RESOLVABLE_TAG,
	/** Advertising mode, which is always present. */
	BT_LE_DECISION_FIELD_ADV_MODE = BT_HCI_LE_DECISION_TEST_FIELD_ADV_MODE,
	/** Received signal strength. */
	BT_LE_DECISION_FIELD_RSSI = BT_HCI_LE_DECISION_TEST_FIELD_RSSI,
	/** Path loss. Absent if the decision PDU does not include the transmit power. */
	BT_LE_DECISION_FIELD_PATH_LOSS = BT_HCI_LE_DECISION_TEST_FIELD_PATH_LOSS,
	/**
	 * Advertiser address. Absent unless the advertiser uses
	 * @ref BT_LE_ADV_OPT_DECISION_ADVA.
	 */
	BT_LE_DECISION_FIELD_ADVA = BT_HCI_LE_DECISION_TEST_FIELD_ADVA,
};

/** Arbitrary data of exactly @p len octets (1 to 8). Absent for any other length. */
#define BT_LE_DECISION_FIELD_DATA_EXACT(len) BT_HCI_LE_DECISION_TEST_FIELD_DATA_EXACT(len)

/** Arbitrary data of at least @p len octets (1 to 8). Absent if shorter. */
#define BT_LE_DECISION_FIELD_DATA_AT_LEAST(len) BT_HCI_LE_DECISION_TEST_FIELD_DATA_AT_LEAST(len)

/** Arbitrary data of 1 to @p len octets (1 to 8). Absent if longer. */
#define BT_LE_DECISION_FIELD_DATA_AT_MOST(len) BT_HCI_LE_DECISION_TEST_FIELD_DATA_AT_MOST(len)

/** Vendor-specific field @p n (0 to 15). Its parameters go in @ref bt_le_decision_test.raw. */
#define BT_LE_DECISION_FIELD_VS(n) (BT_HCI_LE_DECISION_TEST_FIELD_VS_MIN + (n))

/**
 * @name Test flags
 *
 * A test passes if any of the selected conditions holds. If none of the PASS_IF flags is set,
 * @ref BT_LE_DECISION_PASS_IF_MATCH is used.
 * @{
 */
/** Pass if the field is present and the check passes. */
#define BT_LE_DECISION_PASS_IF_MATCH    BIT(0)
/** Pass if the field is present and the check fails. */
#define BT_LE_DECISION_PASS_IF_NO_MATCH BIT(1)
/** Pass if the field is absent. */
#define BT_LE_DECISION_PASS_IF_ABSENT   BIT(2)
/** @} */

/**
 * @name Advertising modes
 *
 * Modes accepted by a test on @ref BT_LE_DECISION_FIELD_ADV_MODE.
 * @{
 */
/** Non-connectable and non-scannable advertising. */
#define BT_LE_DECISION_ADV_MODE_NON_CONN_NON_SCAN BT_HCI_LE_DECISION_ADV_MODE_NON_CONN_NON_SCAN
/** Connectable advertising. */
#define BT_LE_DECISION_ADV_MODE_CONN              BT_HCI_LE_DECISION_ADV_MODE_CONN
/** Scannable advertising. */
#define BT_LE_DECISION_ADV_MODE_SCAN              BT_HCI_LE_DECISION_ADV_MODE_SCAN
/** @} */

/**
 * @name Advertiser address checks
 *
 * Checks applied by a test on @ref BT_LE_DECISION_FIELD_ADVA.
 * @{
 */
/** The advertiser is in the Filter Accept List. */
#define BT_LE_DECISION_ADVA_CHECK_FAL      BT_HCI_LE_DECISION_ADVA_CHECK_FAL
/** The advertiser is the first address. */
#define BT_LE_DECISION_ADVA_CHECK_ADDR     BT_HCI_LE_DECISION_ADVA_CHECK_ADDR_1
/** The advertiser is either address. */
#define BT_LE_DECISION_ADVA_CHECK_ADDR_ANY BT_HCI_LE_DECISION_ADVA_CHECK_ADDR_1_OR_2
/** @} */

/** A single test of the decision instructions. */
struct bt_le_decision_test {
	/** Field to check, see @ref bt_le_decision_field and the BT_LE_DECISION_FIELD_* macros. */
	uint8_t field;

	/** Test flags, see BT_LE_DECISION_PASS_IF_*. */
	uint8_t flags;

	/** Parameters of the check, selected by @ref field. */
	union {
		/**
		 * For @ref BT_LE_DECISION_FIELD_TAG, key of @ref BT_LE_DECISION_KEY_SIZE octets.
		 */
		const uint8_t *key;

		/**
		 * For @ref BT_LE_DECISION_FIELD_ADV_MODE, accepted modes, see
		 * BT_LE_DECISION_ADV_MODE_*.
		 */
		uint8_t adv_modes;

		/** For @ref BT_LE_DECISION_FIELD_RSSI, accepted range in dBm, inclusive. */
		struct {
			/** Minimum RSSI in dBm. */
			int8_t min;
			/** Maximum RSSI in dBm. */
			int8_t max;
		} rssi;

		/** For @ref BT_LE_DECISION_FIELD_PATH_LOSS, accepted range in dB, inclusive. */
		struct {
			/** Minimum path loss in dB. */
			uint8_t min;
			/** Maximum path loss in dB. */
			uint8_t max;
		} path_loss;

		/**
		 * For @ref BT_LE_DECISION_FIELD_ADVA, a resolvable private address is resolved
		 * before it is compared.
		 */
		struct {
			/** Check to apply, BT_LE_DECISION_ADVA_CHECK_*. */
			uint8_t check;
			/** Addresses to compare with. Unused entries may be NULL. */
			const bt_addr_le_t *addr[2];
		} adva;

		/**
		 * Arbitrary data fields: the data, padded with zeros to 8 octets and ANDed with
		 * @p mask, must equal @p target.
		 */
		struct {
			/** Mask applied to the data. */
			uint8_t mask[BT_LE_DECISION_TEST_DATA_LEN];
			/** Value the masked data must equal. */
			uint8_t target[BT_LE_DECISION_TEST_DATA_LEN];
		} data;

		/** Parameters of vendor-specific fields and other fields without a member above. */
		uint8_t raw[BT_HCI_LE_DECISION_TEST_PARAMS_LEN];
	};
};

/** A group of tests that all must pass. */
struct bt_le_decision_group {
	/** Tests in the group. */
	const struct bt_le_decision_test *tests;
	/** Number of tests in the group. */
	size_t num_tests;
};

/**
 * @brief Set the decision instructions used when scanning and initiating.
 *
 * A decision PDU is accepted if all the tests in any one group pass. The instructions apply
 * when scanning with @ref BT_LE_SCAN_OPT_DECISIONS or @ref BT_LE_SCAN_OPT_DECISIONS_ONLY, and
 * when connecting with the BT_CONN_LE_OPT_DECISIONS* options. They may be changed at any time.
 *
 * Passing no groups makes every decision PDU fail, which is also the behavior before any
 * instructions have been set.
 *
 * The instructions must be set again after each call to @ref bt_enable.
 *
 * @kconfig_dep{CONFIG_BT_DECISION_BASED_ADV_FILTERING}
 *
 * @param groups     Groups of tests.
 * @param num_groups Number of groups.
 *
 * @return Zero on success or (negative) error code otherwise.
 * @retval -EAGAIN Bluetooth has not been enabled.
 * @retval -EINVAL A group is empty, a test has invalid parameters, or the tests do not fit in
 *                 the HCI command, see @ref BT_LE_DECISION_TESTS_MAX.
 * @retval -ENOTSUP The Controller does not support Decision-Based Advertising Filtering.
 * @retval -ENOSPC The Controller doesn't support the number of tests given.
 */
int bt_le_scan_set_decision_instructions(const struct bt_le_decision_group *groups,
					 size_t num_groups);

/**
 * @brief Initialize a group from an array of tests.
 *
 * @param _tests Array of @ref bt_le_decision_test.
 */
#define BT_LE_DECISION_GROUP(_tests)                                                               \
	{                                                                                          \
		.tests = (_tests), .num_tests = ARRAY_SIZE(_tests),                                \
	}

/**
 * @brief Test that passes if the Resolvable Tag resolves against a key.
 *
 * @param _key Key of @ref BT_LE_DECISION_KEY_SIZE octets.
 */
#define BT_LE_DECISION_TEST_TAG(_key)                                                              \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_TAG, .key = (_key),                                  \
	}

/** @brief Test that passes if the decision data has no Resolvable Tag. */
#define BT_LE_DECISION_TEST_NO_TAG()                                                               \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_TAG, .flags = BT_LE_DECISION_PASS_IF_ABSENT,         \
	}

/**
 * @brief Test that passes if the RSSI is within a range.
 *
 * @param _min Minimum RSSI in dBm.
 * @param _max Maximum RSSI in dBm.
 */
#define BT_LE_DECISION_TEST_RSSI(_min, _max)                                                       \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_RSSI, .rssi = {.min = (_min), .max = (_max) },       \
	}

/**
 * @brief Test that passes if the path loss is within a range.
 *
 * The test fails if the decision PDU does not include the transmit power.
 *
 * @param _min Minimum path loss in dB.
 * @param _max Maximum path loss in dB.
 */
#define BT_LE_DECISION_TEST_PATH_LOSS(_min, _max)                                                  \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_PATH_LOSS,                                           \
		.path_loss = {.min = (_min), .max = (_max) },                                      \
	}

/**
 * @brief Test that passes for the given advertising modes.
 *
 * @param _modes Bitwise OR of BT_LE_DECISION_ADV_MODE_*.
 */
#define BT_LE_DECISION_TEST_ADV_MODE(_modes)                                                       \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_ADV_MODE, .adv_modes = (_modes),                     \
	}

/** @brief Test that passes if the advertiser is in the Filter Accept List. */
#define BT_LE_DECISION_TEST_ADVA_FAL()                                                             \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_ADVA,                                                \
		.adva = {.check = BT_LE_DECISION_ADVA_CHECK_FAL },                                 \
	}

/**
 * @brief Test that passes if the advertiser has the given address.
 *
 * @param _addr Pointer to the address, of type public or random.
 */
#define BT_LE_DECISION_TEST_ADVA(_addr)                                                            \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_ADVA,                                                \
		.adva = {.check = BT_LE_DECISION_ADVA_CHECK_ADDR, .addr = {(_addr)} },             \
	}

/** @cond INTERNAL_HIDDEN */
#define Z_BT_LE_DECISION_MASK_OCTET(_octet) 0xFF

/** Enforce number of octets within the range 1 to @ref BT_LE_DECISION_TEST_DATA_LEN. */
#define Z_BT_LE_DECISION_DATA_LEN(_bytes...)                                                       \
	(NUM_VA_ARGS(_bytes) +                                                                     \
	 ZERO_OR_COMPILE_ERROR(IN_RANGE(NUM_VA_ARGS(_bytes), 1, BT_LE_DECISION_TEST_DATA_LEN)))
/** @endcond */

/**
 * @brief Test that passes if the arbitrary data equals the given octets.
 *
 * @param _bytes 1 to 8 octets.
 */
#define BT_LE_DECISION_TEST_DATA_EQUALS(_bytes...)                                                 \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_DATA_EXACT(Z_BT_LE_DECISION_DATA_LEN(_bytes)),       \
		.data = {                                                                          \
			.mask = {FOR_EACH(Z_BT_LE_DECISION_MASK_OCTET, (,), _bytes)},              \
			.target = {_bytes},                                                        \
		},                                                                                 \
	}

/**
 * @brief Test that passes if the arbitrary data starts with the given octets.
 *
 * @param _bytes 1 to 8 octets.
 */
#define BT_LE_DECISION_TEST_DATA_STARTS_WITH(_bytes...)                                            \
	{                                                                                          \
		.field = BT_LE_DECISION_FIELD_DATA_AT_LEAST(Z_BT_LE_DECISION_DATA_LEN(_bytes)),    \
		.data = {                                                                          \
			.mask = {FOR_EACH(Z_BT_LE_DECISION_MASK_OCTET, (,), _bytes)},              \
			.target = {_bytes},                                                        \
		},                                                                                 \
	}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_DECISION_H_ */
