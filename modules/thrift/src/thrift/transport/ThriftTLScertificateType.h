/*
 * Copyright 2022 Young Mei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_THRIFT_SRC_THRIFT_TRANSPORT_THRIFTTLSCERTIFICATETYPE_H_
#define ZEPHYR_MODULES_THRIFT_SRC_THRIFT_TRANSPORT_THRIFTTLSCERTIFICATETYPE_H_

namespace apache::thrift::transport
{

enum ThriftTLScertificateType {
	Thrift_TLS_CA_CERT_TAG,
	Thrift_TLS_SERVER_CERT_TAG,
	Thrift_TLS_PRIVATE_KEY,
};

} // namespace apache::thrift::transport

#endif /* ZEPHYR_MODULES_THRIFT_SRC_THRIFT_TRANSPORT_THRIFTTLSCERTIFICATETYPE_H_ */
