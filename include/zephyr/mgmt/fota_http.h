/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Firmware Over-the-Air over HTTP
 */

#ifndef ZEPHYR_INCLUDE_MGMT_FOTA_HTTP_H_
#define ZEPHYR_INCLUDE_MGMT_FOTA_HTTP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/net/tls_credentials.h>

/**
 * @brief Firmware Over-the-Air over HTTP
 * @defgroup fota_http FOTA HTTP
 * @since 4.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Transfer state reported while an image is being written. */
struct fota_http_progress {
	/** Bytes present in the slot so far, including resumed data. */
	size_t written;
	/** Total image size, or 0 when the server did not announce it. */
	size_t total;
	/** HTTP status code of the response. */
	int http_status;
};

/**
 * @brief Progress callback.
 *
 * Invoked from the downloading thread every time a body fragment has been
 * written to flash.
 *
 * @param progress Current state of the transfer.
 * @param user_data Pointer from fota_http_download_params.
 */
typedef void (*fota_http_progress_cb_t)(const struct fota_http_progress *progress, void *user_data);

/**
 * @brief Completion callback of fota_http_download_async().
 *
 * Invoked from the library thread once the download has finished.
 *
 * @param result Return value of the download, 0 on success.
 * @param user_data Pointer from fota_http_download_params.
 */
typedef void (*fota_http_done_cb_t)(int result, void *user_data);

/** @brief Parameters of a download. */
struct fota_http_download_params {
	/**
	 * Absolute URL of the form "http://host[:port]/path", or "https"
	 * when @kconfig{CONFIG_FOTA_HTTP_TLS} is enabled.
	 */
	const char *url;
	/**
	 * Optional NULL terminated list of extra request headers. Each
	 * entry is a complete header line ending in "\r\n", for example
	 * "Authorization: Bearer token\r\n".
	 */
	const char *const *headers;
	/**
	 * Optional expected SHA-256 digest of the image, 32 bytes. Requires
	 * @kconfig{CONFIG_FOTA_HTTP_SHA256_CHECK}.
	 */
	const uint8_t *sha256;
	/**
	 * Optional TLS security tags to use instead of
	 * @kconfig{CONFIG_FOTA_HTTP_TLS_SEC_TAG}.
	 */
	const sec_tag_t *sec_tags;
	/** Number of entries in @ref sec_tags. */
	size_t sec_tag_count;
	/**
	 * Optional host name to verify the server certificate against,
	 * when it differs from the URL host. Needed when the URL carries
	 * an IP address and the certificate has no matching IP entry.
	 */
	const char *tls_hostname;
	/** Optional progress callback. */
	fota_http_progress_cb_t progress_cb;
	/** Passed to the callbacks untouched. */
	void *user_data;
	/** Image to update in a multi-image layout, 0 for the only image. */
	uint8_t image_index;
	/**
	 * Continue a previously interrupted download. The saved offset is
	 * only used when the URL and image index match the interrupted
	 * download, and the request carries If-Range when the server sent
	 * an ETag or Last-Modified header, so a file that changed on the
	 * server is fetched again from the start. Requires
	 * @kconfig{CONFIG_FOTA_HTTP_RESUME}.
	 */
	bool resume;
};

/**
 * @brief Download a firmware image into the MCUboot secondary slot.
 *
 * Blocks until the whole image is written or the transfer fails. A failed
 * download leaves the slot partially written, which MCUboot rejects.
 *
 * Only the image is stored. Call fota_http_apply() to boot it.
 *
 * While the running image is a test image the secondary slot holds the
 * image MCUboot reverts to, so downloads are refused until
 * fota_http_confirm() has been called.
 *
 * The caller's stack must fit the HTTP client and, with https, the TLS
 * handshake; fota_http_download_async() uses a library thread instead.
 *
 * @param params Download parameters. Pointed-to data must stay valid until
 *               the call returns.
 *
 * @retval 0 The image was written and its header is valid.
 * @retval -EINVAL The URL could not be parsed or exceeds the buffer sizes.
 * @retval -ENOTSUP The URL scheme or a requested feature is not supported.
 * @retval -ENODEV No slot exists for @p image_index.
 * @retval -EBUSY Another download is in progress.
 * @retval -EBADMSG The server answered with an unexpected status.
 * @retval -ELOOP More redirects than @kconfig{CONFIG_FOTA_HTTP_MAX_REDIRECTS}.
 * @retval -ENODATA The server sent an empty body.
 * @retval -EMSGSIZE Fewer bytes arrived than the server announced.
 * @retval -ECANCELED fota_http_cancel() was called.
 * @retval -EILSEQ The SHA-256 digest does not match.
 * @retval -ENOEXEC The data written is not an MCUboot image.
 * @retval -EPERM The running image is not confirmed, or the image is older
 *                than the running one.
 * @retval -errno Any socket, HTTP client or flash error.
 */
int fota_http_download(const struct fota_http_download_params *params);

/**
 * @brief Download a firmware image on the library thread.
 *
 * Returns as soon as the transfer is queued. @p done_cb is invoked with
 * the result fota_http_download() would have returned.
 *
 * @param params Download parameters. The structure and the URL are copied,
 *               the other data it points to must stay valid until
 *               @p done_cb runs.
 * @param done_cb Completion callback, may be NULL.
 *
 * @retval 0 The download was queued.
 * @retval -EINVAL @p params or its URL is NULL, or the URL is too long.
 * @retval -ENOTSUP A requested feature is not compiled in.
 * @retval -EPERM The running image is not confirmed.
 * @retval -EBUSY Another download is in progress.
 */
int fota_http_download_async(const struct fota_http_download_params *params,
			     fota_http_done_cb_t done_cb);

/**
 * @brief Abort the download in progress.
 *
 * The transfer stops when the next fragment arrives or the request times
 * out, and reports -ECANCELED.
 *
 * @retval 0 A download was running and will stop.
 * @retval -EALREADY No download is running.
 */
int fota_http_cancel(void);

/**
 * @brief Request MCUboot to boot a downloaded image on the next reset.
 *
 * The secondary slot must hold an image with a valid MCUboot header. The
 * signature is only checked by the bootloader itself.
 *
 * When several images must be updated together, download and apply every
 * one of them before rebooting: MCUboot validates the whole set and swaps
 * it in one go, so a reboot with only part of the set applied swaps only
 * what was marked.
 *
 * @param image_index Image to boot, 0 for the only image.
 * @param permanent When true the image is marked confirmed right away. When
 *                  false it boots once for evaluation and the bootloader
 *                  reverts to the current image unless fota_http_confirm()
 *                  is called before the following reset. A revert needs an
 *                  MCUboot mode that supports it, such as swap-using-move or
 *                  swap-using-offset.
 *
 * @retval 0 The upgrade was requested.
 * @retval -ENODEV No slot exists for @p image_index.
 * @retval -ENOEXEC The secondary slot holds no valid image.
 * @retval -errno Any flash or bootloader error.
 */
int fota_http_apply(uint8_t image_index, bool permanent);

/**
 * @brief Confirm a running image so the bootloader stops reverting it.
 *
 * Only the image named by @p image_index is confirmed, so a layout that
 * carries firmware for a second device confirms just what this device
 * validated. Confirm every image of a set that must run together.
 *
 * @param image_index Image to confirm, 0 for the only image.
 *
 * @retval 0 The image was confirmed.
 * @retval -ENODEV No slot exists for @p image_index.
 * @retval -errno Any flash or bootloader error.
 */
int fota_http_confirm(uint8_t image_index);

/**
 * @brief Check whether the running image still has to be confirmed.
 *
 * Reports the state of image 0 only, because the bootloader interface has no
 * per-image query. An application updating several images tracks the rest
 * itself.
 *
 * @return true after a test upgrade until fota_http_confirm() is called.
 */
bool fota_http_confirm_pending(void);

/**
 * @brief Flash area identifier of the slot a download is written to.
 *
 * With direct-XIP bootloaders the slot is fixed at link time, so the
 * caller must fetch the image variant built for it.
 *
 * @param image_index Image whose secondary slot is queried.
 *
 * @return Flash area identifier, or negative errno code when the image
 *         has no secondary slot.
 */
int fota_http_upload_slot(uint8_t image_index);

/**
 * @brief Read the MCUboot header of the image in the secondary slot.
 *
 * @param image_index Image whose secondary slot is read.
 * @param header Output header.
 *
 * @retval 0 A valid header was read.
 * @retval -EIO The slot holds no valid image.
 * @retval -errno Any flash error.
 */
int fota_http_image_header(uint8_t image_index, struct mcuboot_img_header *header);

/**
 * @brief Bytes written by the most recent download.
 *
 * @return Number of bytes in the slot, 0 if no download ran.
 */
size_t fota_http_bytes_written(void);

/**
 * @brief Register the CA certificate used to authenticate https servers.
 *
 * The certificate is stored under @kconfig{CONFIG_FOTA_HTTP_TLS_SEC_TAG}.
 * An application that already registered a certificate under that tag with
 * tls_credential_add() does not need to call this.
 *
 * @param cert DER or PEM encoded certificate. It is not copied, so it must
 *             remain valid for as long as https downloads are performed.
 * @param len Size of @p cert in bytes.
 *
 * @return 0 on success, negative errno code on failure.
 */
int fota_http_tls_add_ca(const void *cert, size_t len);

/**
 * @brief Register a client certificate and key for mutual TLS.
 *
 * Both are stored under @kconfig{CONFIG_FOTA_HTTP_TLS_SEC_TAG} and are
 * not copied.
 *
 * @param cert DER or PEM encoded client certificate.
 * @param cert_len Size of @p cert in bytes.
 * @param key DER or PEM encoded private key.
 * @param key_len Size of @p key in bytes.
 *
 * @return 0 on success, negative errno code on failure.
 */
int fota_http_tls_add_client_cert(const void *cert, size_t cert_len, const void *key,
				  size_t key_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_FOTA_HTTP_H_ */
