/*
 * Copyright 2022 Young Mei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#endif

#include <cstdio>
#include <cstdlib>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/TServerSocket.h>

#include "Hello.h"
#include "HelloHandler.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

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
	my_addr = CONFIG_NET_CONFIG_MY_IPV4_ADDR;
#else
	if (IS_ENABLED(CONFIG_THRIFT_SSL_SOCKET)) {
		if (argc != 5) {
			printf("usage: %s <ip> <native-cert.pem> <native-key.pem> "
			       "<qemu-cert.pem>\n",
			       argv[0]);
			return EXIT_FAILURE;
		}
	} else {
		if (argc != 2) {
			printf("usage: %s <ip>\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	my_addr = std::string(argv[1]);
#endif

	const int port = 4242;
	std::shared_ptr<TServerTransport> serverTransport;
	std::shared_ptr<TTransportFactory> transportFactory;
	std::shared_ptr<TProtocolFactory> protocolFactory;
	std::shared_ptr<HelloHandler> handler(new HelloHandler());
	std::shared_ptr<TProcessor> processor(new HelloProcessor(handler));

	if (IS_ENABLED(CONFIG_THRIFT_SSL_SOCKET)) {
		std::shared_ptr<TSSLSocketFactory> socketFactory(new TSSLSocketFactory());
		socketFactory->server(true);
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
		serverTransport =
			std::make_shared<TSSLServerSocket>("0.0.0.0", port, socketFactory);
	} else {
		serverTransport = std::make_shared<TServerSocket>(my_addr, port);
	}

	transportFactory = std::make_shared<TBufferedTransportFactory>();
	if (IS_ENABLED(CONFIG_THRIFT_COMPACT_PROTOCOL)) {
		protocolFactory = std::make_shared<TCompactProtocolFactory>();
	} else {
		protocolFactory = std::make_shared<TBinaryProtocolFactory>();
	}

	TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);

	try {
		server.serve();
	} catch (std::exception &e) {
		printf("caught exception: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
