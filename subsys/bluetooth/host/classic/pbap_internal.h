/* pbap_internal.h - Internal for Phone Book Access Protocol handling */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/classic/pbap.h>

#define BT_PBAP_FLAG_RSP_ONGOING BIT(0)

/** @brief PBAP connection state enumeration */
enum __packed bt_pbap_state {
	/** PBAP disconnected */
	BT_PBAP_STATE_DISCONNECTED = 0,
	/** PBAP PBAP in connecting state */
	BT_PBAP_STATE_CONNECTING = 1,
	/** PBAP ready for upper layer traffic on it */
	BT_PBAP_STATE_CONNECTED = 2,
	/** PBAP in disconnecting state */
	BT_PBAP_STATE_DISCONNECTING = 3,
};

/** @brief Transport layer state enumeration */
enum __packed bt_pbap_transport_state {
	/** @brief Transport is disconnected */
	BT_PBAP_TRANSPORT_STATE_DISCONNECTED = 0,
	/** @brief Transport is connecting */
	BT_PBAP_TRANSPORT_STATE_CONNECTING = 1,
	/** @brief Transport is connected */
	BT_PBAP_TRANSPORT_STATE_CONNECTED = 2,
	/** @brief Transport is disconnecting */
	BT_PBAP_TRANSPORT_STATE_DISCONNECTING = 3,
};

/* PBAP PSE Supported features */
#if defined(CONFIG_BT_PBAP_PSE_DOWNLOAD)
/** @brief Enable download feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_DOWNLOAD_ENABLE BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD
#else
/** @brief Disable download feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_DOWNLOAD_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_DOWNLOAD */

#if defined(CONFIG_BT_PBAP_PSE_BROWSING)
/** @brief Enable browsing feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_BROWSING_ENABLE BT_PBAP_SUPPORTED_FEATURE_BROWSING
#else
/** @brief Disable browsing feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_BROWSING_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_BROWSING */

#if defined(CONFIG_BT_PBAP_PSE_DATABASE_IDENTIFIER)
/** @brief Enable database identifier feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_DATABASE_IDENTIFIER_ENABLE BT_PBAP_SUPPORTED_FEATURE_DATABASE_IDENTIFIER
#else
/** @brief Disable database identifier feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_DATABASE_IDENTIFIER_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_DATABASE_IDENTIFIER */

#if defined(CONFIG_BT_PBAP_PSE_FOLDER_VERSION_COUNTERS)
/** @brief Enable folder version counters feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_FOLDER_VERSION_COUNTERS_ENABLE                                         \
	BT_PBAP_SUPPORTED_FEATURE_FOLDER_VERSION_COUNTERS
#else
/** @brief Disable folder version counters feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_FOLDER_VERSION_COUNTERS_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_FOLDER_VERSION_COUNTERS */

#if defined(CONFIG_BT_PBAP_PSE_VCARD_SELECTOR)
/** @brief Enable vCard selector feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_VCARD_SELECTOR_ENABLE BT_PBAP_SUPPORTED_FEATURE_VCARD_SELECTOR
#else
/** @brief Disable vCard selector feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_VCARD_SELECTOR_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_VCARD_SELECTOR */

#if defined(CONFIG_BT_PBAP_PSE_ENHANCED_MISSED_CALLS)
/** @brief Enable enhanced missed calls feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_ENHANCED_MISSED_CALLS_ENABLE                                           \
	BT_PBAP_SUPPORTED_FEATURE_ENHANCED_MISSED_CALLS
#else
/** @brief Disable enhanced missed calls feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_ENHANCED_MISSED_CALLS_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_ENHANCED_MISSED_CALLS */

#if defined(CONFIG_BT_PBAP_PSE_UCI_VCARD_PROPERTY)
/** @brief Enable UCI vCard property feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_UCI_VCARD_PROPERTY_ENABLE BT_PBAP_SUPPORTED_FEATURE_UCI_VCARD_PROPERTY
#else
/** @brief Disable UCI vCard property feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_UCI_VCARD_PROPERTY_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_UCI_VCARD_PROPERTY */

#if defined(CONFIG_BT_PBAP_PSE_UID_VCARD_PROPERTY)
/** @brief Enable UID vCard property feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_UID_VCARD_PROPERTY_ENABLE BT_PBAP_SUPPORTED_FEATURE_UID_VCARD_PROPERTY
#else
/** @brief Disable UID vCard property feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_UID_VCARD_PROPERTY_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_UID_VCARD_PROPERTY */

#if defined(CONFIG_BT_PBAP_PSE_CONTACT_REFERENCING)
/** @brief Enable contact referencing feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_CONTACT_REFERENCING_ENABLE BT_PBAP_SUPPORTED_FEATURE_CONTACT_REFERENCING
#else
/** @brief Disable contact referencing feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_CONTACT_REFERENCING_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_CONTACT_REFERENCING */

#if defined(CONFIG_BT_PBAP_PSE_DEFAULT_CONTACT_IMAGE)
/** @brief Enable default contact image feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_DEFAULT_CONTACT_IMAGE_ENABLE                                           \
	BT_PBAP_SUPPORTED_FEATURE_DEFAULT_CONTACT_IMAGE
#else
/** @brief Disable default contact image feature for PBAP PSE. */
#define BT_PBAP_PSE_FEATURE_DEFAULT_CONTACT_IMAGE_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_DEFAULT_CONTACT_IMAGE */

/** @brief PBAP PSE supported features bitmask.
 *
 *  Combination of all enabled PBAP PSE features based on configuration.
 */
#define BT_PBAP_PSE_SUPPORTED_FEATURES                                                             \
	(BT_PBAP_PSE_FEATURE_DOWNLOAD_ENABLE | BT_PBAP_PSE_FEATURE_BROWSING_ENABLE |               \
	 BT_PBAP_PSE_FEATURE_DATABASE_IDENTIFIER_ENABLE |                                          \
	 BT_PBAP_PSE_FEATURE_FOLDER_VERSION_COUNTERS_ENABLE |                                      \
	 BT_PBAP_PSE_FEATURE_VCARD_SELECTOR_ENABLE |                                               \
	 BT_PBAP_PSE_FEATURE_ENHANCED_MISSED_CALLS_ENABLE |                                        \
	 BT_PBAP_PSE_FEATURE_UCI_VCARD_PROPERTY_ENABLE |                                           \
	 BT_PBAP_PSE_FEATURE_UID_VCARD_PROPERTY_ENABLE |                                           \
	 BT_PBAP_PSE_FEATURE_CONTACT_REFERENCING_ENABLE |                                          \
	 BT_PBAP_PSE_FEATURE_DEFAULT_CONTACT_IMAGE_ENABLE)

#if defined(CONFIG_BT_PBAP_PSE_REPOSITORY_LOCAL_PHONE_BOOK)
/** @brief Enable local phone book repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_LOCAL_PHONE_BOOK_ENABLE                                             \
	BT_PBAP_SUPPORTED_REPOSITORIES_LOCAL_PHONE_BOOK
#else
/** @brief Disable local phone book repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_LOCAL_PHONE_BOOK_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_REPOSITORY_LOCAL_PHONE_BOOK */

#if defined(CONFIG_BT_PBAP_PSE_REPOSITORY_SIM)
/** @brief Enable SIM repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_SIM_ENABLE BT_PBAP_SUPPORTED_REPOSITORIES_SIM
#else
/** @brief Disable SIM repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_SIM_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_REPOSITORY_SIM */

#if defined(CONFIG_BT_PBAP_PSE_REPOSITORY_SPEED_DIAL)
/** @brief Enable speed dial repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_SPEED_DIAL_ENABLE BT_PBAP_SUPPORTED_REPOSITORIES_SPEED_DIAL
#else
/** @brief Disable speed dial repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_SPEED_DIAL_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_REPOSITORY_SPEED_DIAL */

#if defined(CONFIG_BT_PBAP_PSE_REPOSITORY_FAVORITES)
/** @brief Enable favorites repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_FAVORITES_ENABLE BT_PBAP_SUPPORTED_REPOSITORIES_FAVORITES
#else
/** @brief Disable favorites repository for PBAP PSE. */
#define BT_PBAP_PSE_REPOSITORY_FAVORITES_ENABLE 0
#endif /* CONFIG_BT_PBAP_PSE_REPOSITORY_FAVORITES */

/** @brief PBAP PSE supported repositories bitmask.
 *
 *  Combination of all enabled PBAP PSE repositories based on configuration.
 */
#define BT_PBAP_PSE_SUPPORTED_REPOSITORIES                                                         \
	(BT_PBAP_PSE_REPOSITORY_LOCAL_PHONE_BOOK_ENABLE | BT_PBAP_PSE_REPOSITORY_SIM_ENABLE |      \
	 BT_PBAP_PSE_REPOSITORY_SPEED_DIAL_ENABLE | BT_PBAP_PSE_REPOSITORY_FAVORITES_ENABLE)
