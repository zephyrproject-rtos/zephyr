/*
 * Copyright 2022 Young Mei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#endif

#include <cstdlib>
#include <iostream>

#include <fcntl.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/TSocket.h>

#include "Hello.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

#ifndef IS_ENABLED
#define IS_ENABLED(flag) flag
#endif

#ifndef CONFIG_THRIFT_COMPACT_PROTOCOL
#define CONFIG_THRIFT_COMPACT_PROTOCOL 0
#endif

#ifndef CONFIG_THRIFT_SSL_SOCKET
#define CONFIG_THRIFT_SSL_SOCKET 0
#endif

#ifdef __ZEPHYR__
int main(void)
#else
int main(int argc, char **argv)
#endif
{
	std::string my_addr;

#ifdef __ZEPHYR__
	my_addr = CONFIG_NET_CONFIG_PEER_IPV4_ADDR;
#else
	if (IS_ENABLED(CONFIG_THRIFT_SSL_SOCKET)) {
		if (argc != 5) {
			printf("usage: %s <ip> <native-cert.pem> <native-key.pem> "
			       "<qemu-cert.pem>\n",
			       argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (argc >= 2) {
		my_addr = std::string(argv[1]);
	} else {
		my_addr = "192.0.2.1";
	}
#endif

	int port = 4242;
	std::shared_ptr<TProtocol> protocol;
	std::shared_ptr<TTransport> transport;
	std::shared_ptr<TSSLSocketFactory> socketFactory;
	std::shared_ptr<TTransport> trans;

	if (IS_ENABLED(CONFIG_THRIFT_SSL_SOCKET)) {
		const int port = 4242;
		socketFactory = std::make_shared<TSSLSocketFactory>();
		socketFactory->authenticate(true);

#ifdef __ZEPHYR__
		static const char qemu_cert_pem[] = {
#include "qemu_cert.pem.inc"
			'\0'
		};

		static const char qemu_key_pem[] = {
#include "qemu_key.pem.inc"
			'\0'
		};

		static const char native_cert_pem[] = {
#include "native_cert.pem.inc"
			'\0'
		};

		socketFactory->loadCertificateFromBuffer(qemu_cert_pem);
		socketFactory->loadPrivateKeyFromBuffer(qemu_key_pem);
		socketFactory->loadTrustedCertificatesFromBuffer(native_cert_pem);
#else
		socketFactory->loadCertificate(argv[2]);
		socketFactory->loadPrivateKey(argv[3]);
		socketFactory->loadTrustedCertificates(argv[4]);
#endif
		trans = socketFactory->createSocket(my_addr, port);
	} else {
		trans = std::make_shared<TSocket>(my_addr, port);
	}

	transport = std::make_shared<TBufferedTransport>(trans);

	if (IS_ENABLED(CONFIG_THRIFT_COMPACT_PROTOCOL)) {
		protocol = std::make_shared<TCompactProtocol>(transport);
	} else {
		protocol = std::make_shared<TBinaryProtocol>(transport);
	}

	HelloClient client(protocol);

	try {
		transport->open();
		client.ping();
		std::string s;
		client.echo(s, "Hello, world!");
		for (int i = 0; i < 5; ++i) {
			client.counter();
		}

		transport->close();
	} catch (std::exception &e) {
		printf("caught exception: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
