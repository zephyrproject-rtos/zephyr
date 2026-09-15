/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CA_CERTIFICATE_H_
#define CA_CERTIFICATE_H_

/* Self-signed test certificate for localhost, replace it with the server's CA. */
static const unsigned char ca_certificate[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDJzCCAg+gAwIBAgIUCvlxxYDX0QDpTj+8mHduQJfL36MwDQYJKoZIhvcNAQEL\r\n"
	"BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDkwNDE2MTg1NloYDzIxMjYw\r\n"
	"ODExMTYxODU2WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB\r\n"
	"AQUAA4IBDwAwggEKAoIBAQC+NOVyZ9cyisDfBVAfTmb3Ml6eMeKkL1K6+fEK/To0\r\n"
	"k5EVz2E3v/Rbrf4IeqdZ+26YX0aSNajWXsSDJT1Elvq0b+DuSv4krdZGd4Thgx33\r\n"
	"7QtNncKkbSCXFjrNQ5dcu4iDHguDDx1aouvvlPoBTe8LPLf4lmJhXVTPJ/kz1wiF\r\n"
	"wruWb15Jzd8z1wTvbfQ14yiuWMjmU1xlEIT+i7qpU3mZXwi4Y3Yn7pceIRz8jy+M\r\n"
	"jcYrcKLQW90iI6Hx+FD5XvtgPOTwE3+u0NLd2FzQO+Nz9emGAMtftRTn0NASl1qQ\r\n"
	"1r9m0RwnOODx6CNg5yQL021Bdei4hdaNDsrLorzW0asBAgMBAAGjbzBtMB0GA1Ud\r\n"
	"DgQWBBQZ11rhHi7VlRYCqj71OlGaaV8HbzAfBgNVHSMEGDAWgBQZ11rhHi7VlRYC\r\n"
	"qj71OlGaaV8HbzAPBgNVHRMBAf8EBTADAQH/MBoGA1UdEQQTMBGCCWxvY2FsaG9z\r\n"
	"dIcEfwAAATANBgkqhkiG9w0BAQsFAAOCAQEAajFfYRnby8AJ8f3T6n6TEYkz5617\r\n"
	"6Wl1STAqwbxwT5TiOwjGwlM/FkK3lsdZAGjH46dbGLAtxfqjrgjuKHBiyZq54+kP\r\n"
	"2req8kiA6psOPKMDyhJEkeYqSaJl2z88eEpZ902fFxRLz17TgSOyJ3CRgw5WRkmc\r\n"
	"rZja3hNuHH6AEyJJVTqu3XDv3HCQLE3E4KRpIf4dt/yq5hJGkAo/QROE0egeeFD5\r\n"
	"rigiS98Fs7aHhVbPrEWiV35EFnUA1gY8haOnFJIeOY8qduMnJBvtam9/ewqPxe60\r\n"
	"kV1hkEWrDKOzVtxaLJAuh68NJTuUy5GTjW7Z1+ed4cMZ0bmdyNPIRbt8qA==\r\n"
	"-----END CERTIFICATE-----\r\n";

#endif /* CA_CERTIFICATE_H_ */
