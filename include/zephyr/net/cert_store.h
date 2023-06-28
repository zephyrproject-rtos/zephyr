/*
 * Copyright (c) 2023 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_CERT_STORE_H_
#define ZEPHYR_INCLUDE_NET_CERT_STORE_H_

#define FINGERPRINT_HEX_SIZE 41
#define FINGERPRINT_SHA_SIZE 20

/* Certificate store structure */
struct cert_store {
	/* Cert store directory */
	struct fs_dir_t store_dir;
	/* Cert store literal path */
	const char *store_path;
};

/**
 * @brief Opens a certificate store at the specified path.
 *
 * @param store_path Path to the certificate store.
 * @param store Pointer to a structure representing the opened certificate store.
 * @return Returns 0 on success, a negative error value on failure.
 */
int cert_store_open(const char *store_path, struct cert_store *store);

/**
 * @brief Closes a certificate store.
 *
 * @param store Pointer to the certificate store to be closed.
 * @return Returns 0 on success, a negative error value on failure.
 */
int cert_store_close(struct cert_store *store);

/**
 * @brief Calculates and returns the fingerprint of a certificate.
 *
 * @param cert_buf Buffer containing the certificate to calculate the fingerprint from.
 * @param cert_sz Size of the certificate data.
 * @param fingerprint_buf Buffer to receive the calculated fingerprint.
 * @return Returns 0 on success, a negative error value on failure.
 */
int cert_store_fingerprint(const uint8_t *cert_buf, size_t cert_sz, uint8_t *fingerprint_buf);

/**
 * @brief Stores a certificate in the certificate store.
 *
 * @param store Pointer to the certificate store where the certificate will be stored.
 * @param cert_buf Buffer containing the certificate data.
 * @param cert_sz Size of the certificate data.
 * @return Returns 0 on success, a negative error value on failure.
 */
int cert_store_mgmt_store(struct cert_store *store, const uint8_t *cert_buf, size_t cert_sz);

/**
 * @brief Deletes a certificate from the certificate store using its fingerprint.
 *
 * @param store Pointer to the certificate store.
 * @param cert_finger Fingerprint of the certificate to delete.
 * @return Returns 0 on success, a negative error value on failure.
 */
int cert_store_mgmt_delete(const struct cert_store *store, const uint8_t *cert_finger);

/**
 * @brief Loads a certificate from the certificate store using its fingerprint
 *
 * @param store Pointer to the certificate store.
 * @param cert_finger Fingerprint of the certificate to load.
 * @param cert_buf Buffer where the loaded certificate will be stored.
 * @param buf_sz Size of the buffer pointed to by `cert_buf`.
 * @param cert_sz Size of loaded certificate.
 * @return Returns 0 on success, a negative error value on failure.
 */
int cert_store_mgmt_load(const struct cert_store *store, const uint8_t *cert_finger,
			 const uint8_t *cert_buf, size_t buf_sz, size_t *cert_sz);

#endif /* ZEPHYR_INCLUDE_NET_CERT_STORE_H_ */
