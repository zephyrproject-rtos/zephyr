/* alert.h -- DTLS alert protocol
 *
 * Copyright (C) 2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file alert.h
 * @brief DTLS alert protocol
 */

#ifndef _DTLS_ALERT_H_
#define _DTLS_ALERT_H_

typedef enum {
  DTLS_ALERT_LEVEL_WARNING=1,
  DTLS_ALERT_LEVEL_FATAL=2
} dtls_alert_level_t;

typedef enum {
  DTLS_ALERT_CLOSE_NOTIFY = 0,			/* close_notify */
  DTLS_ALERT_UNEXPECTED_MESSAGE = 10,		/* unexpected_message */
  DTLS_ALERT_BAD_RECORD_MAC = 20,		/* bad_record_mac */
  DTLS_ALERT_RECORD_OVERFLOW = 22,		/* record_overflow */
  DTLS_ALERT_DECOMPRESSION_FAILURE = 30,	/* decompression_failure */
  DTLS_ALERT_HANDSHAKE_FAILURE = 40,		/* handshake_failure */
  DTLS_ALERT_BAD_CERTIFICATE = 42,		/* bad_certificate */
  DTLS_ALERT_UNSUPPORTED_CERTIFICATE = 43,	/* unsupported_certificate */
  DTLS_ALERT_CERTIFICATE_REVOKED = 44,		/* certificate_revoked */
  DTLS_ALERT_CERTIFICATE_EXPIRED = 45,		/* certificate_expired */
  DTLS_ALERT_CERTIFICATE_UNKNOWN = 46,		/* certificate_unknown */
  DTLS_ALERT_ILLEGAL_PARAMETER = 47,		/* illegal_parameter */
  DTLS_ALERT_UNKNOWN_CA = 48,			/* unknown_ca */
  DTLS_ALERT_ACCESS_DENIED = 49,		/* access_denied */
  DTLS_ALERT_DECODE_ERROR = 50,			/* decode_error */
  DTLS_ALERT_DECRYPT_ERROR = 51,		/* decrypt_error */
  DTLS_ALERT_PROTOCOL_VERSION = 70,		/* protocol_version */
  DTLS_ALERT_INSUFFICIENT_SECURITY = 71,	/* insufficient_security */
  DTLS_ALERT_INTERNAL_ERROR = 80,		/* internal_error */
  DTLS_ALERT_USER_CANCELED = 90,		/* user_canceled */
  DTLS_ALERT_NO_RENEGOTIATION = 100,		/* no_renegotiation */
  DTLS_ALERT_UNSUPPORTED_EXTENSION = 110	/* unsupported_extension */
} dtls_alert_t;

#define DTLS_EVENT_CONNECT        0x01DC /**< initiated handshake */
#define DTLS_EVENT_CONNECTED      0x01DE /**< handshake or re-negotiation
					  * has finished */
#define DTLS_EVENT_RENEGOTIATE    0x01DF /**< re-negotiation has started */

static inline int
dtls_alert_create(dtls_alert_level_t level, dtls_alert_t desc)
{
  return -((level << 8) | desc);
}

static inline int
dtls_alert_fatal_create(dtls_alert_t desc)
{
  return dtls_alert_create(DTLS_ALERT_LEVEL_FATAL, desc);
}

#endif /* _DTLS_ALERT_H_ */
