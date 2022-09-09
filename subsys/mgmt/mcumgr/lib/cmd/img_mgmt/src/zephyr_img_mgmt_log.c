/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <img_mgmt/img_mgmt.h>
/**
 * Log event types (all events are CBOR-encoded):
 *
 * upstart:
 *	 When: upon receiving an upload request with an offset of 0.
 *	 Structure:
 *	 {
 *		 "ev": "upstart",
 *		 "rc": <mgmt-error-code (int)>
 *	 }
 *
 * updone:
 *	 When: upon receiving an upload request containing the final chunk of an
 *		   image OR a failed upload request with a non-zero offset.
 *	 Structure:
 *	 {
 *		 "ev": "updone",
 *		 "rc": <mgmt-error-code (int)>
 *		 "hs": <image-hash (byte-string)> (only present on success)
 *	 }
 *
 * pend:
 *	 When: upon receiving a non-permanent `set-pending` request.
 *	 Structure:
 *	 {
 *		 "ev": "pend",
 *		 "rc": <mgmt-error-code (int)>,
 *		 "hs": <image-hash (byte-string)>
 *	 }
 *
 * conf:
 *	 When: upon receiving a `confirm` request OR a permanent `set-pending`
 *		   request.
 *	 Structure:
 *	 {
 *		 "ev": "conf",
 *		 "rc": <mgmt-error-code (int)>,
 *		 "hs": <image-hash (byte-string)> (only present for `set-pending`)
 *	 }
 */

#define IMG_MGMT_LOG_EV_UPSTART   "upstart"
#define IMG_MGMT_LOG_EV_UPDONE	"updone"
#define IMG_MGMT_LOG_EV_PEND	  "pend"
#define IMG_MGMT_LOG_EV_CONF	  "conf"

int
img_mgmt_impl_log_upload_start(int status)
{
	return 0;
}

int
img_mgmt_impl_log_upload_done(int status, const uint8_t *hash)
{
	return 0;
}

int
img_mgmt_impl_log_pending(int status, const uint8_t *hash)
{
	return 0;
}

int
img_mgmt_impl_log_confirm(int status, const uint8_t *hash)
{
	return 0;
}
