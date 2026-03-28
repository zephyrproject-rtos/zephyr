/*
 * Copyright 2006 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>

#include <thrift/thrift_export.h>
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/ThriftTLScertificateType.h>

#include <thrift/transport/TSocketUtils.h>

template <class T> inline void *cast_sockopt(T *v)
{
	return reinterpret_cast<void *>(v);
}

void destroyer_of_fine_sockets(THRIFT_SOCKET *ssock);

namespace apache
{
namespace thrift
{
namespace transport
{

/**
 * SSL server socket implementation.
 */
TSSLServerSocket::TSSLServerSocket(int port, std::shared_ptr<TSSLSocketFactory> factory)
	: TServerSocket(port), factory_(factory)
{
	factory_->server(true);
}

TSSLServerSocket::TSSLServerSocket(const std::string &address, int port,
				   std::shared_ptr<TSSLSocketFactory> factory)
	: TServerSocket(address, port), factory_(factory)
{
	factory_->server(true);
}

TSSLServerSocket::TSSLServerSocket(int port, int sendTimeout, int recvTimeout,
				   std::shared_ptr<TSSLSocketFactory> factory)
	: TServerSocket(port, sendTimeout, recvTimeout), factory_(factory)
{
	factory_->server(true);
}

std::shared_ptr<TSocket> TSSLServerSocket::createSocket(THRIFT_SOCKET client)
{
	if (interruptableChildren_) {
		return factory_->createSocket(client, pChildInterruptSockReader_);

	} else {
		return factory_->createSocket(client);
	}
}

void TSSLServerSocket::listen()
{
	THRIFT_SOCKET sv[2];
	// Create the socket pair used to interrupt
	if (-1 == THRIFT_SOCKETPAIR(AF_LOCAL, SOCK_STREAM, 0, sv)) {
		GlobalOutput.perror("TServerSocket::listen() socketpair() interrupt",
				    THRIFT_GET_SOCKET_ERROR);
		interruptSockWriter_ = THRIFT_INVALID_SOCKET;
		interruptSockReader_ = THRIFT_INVALID_SOCKET;
	} else {
		interruptSockWriter_ = sv[1];
		interruptSockReader_ = sv[0];
	}

	// Create the socket pair used to interrupt all clients
	if (-1 == THRIFT_SOCKETPAIR(AF_LOCAL, SOCK_STREAM, 0, sv)) {
		GlobalOutput.perror("TServerSocket::listen() socketpair() childInterrupt",
				    THRIFT_GET_SOCKET_ERROR);
		childInterruptSockWriter_ = THRIFT_INVALID_SOCKET;
		pChildInterruptSockReader_.reset();
	} else {
		childInterruptSockWriter_ = sv[1];
		pChildInterruptSockReader_ = std::shared_ptr<THRIFT_SOCKET>(
			new THRIFT_SOCKET(sv[0]), destroyer_of_fine_sockets);
	}

	// Validate port number
	if (port_ < 0 || port_ > 0xFFFF) {
		throw TTransportException(TTransportException::BAD_ARGS,
					  "Specified port is invalid");
	}

	// Resolve host:port strings into an iterable of struct addrinfo*
	AddressResolutionHelper resolved_addresses;
	try {
		resolved_addresses.resolve(address_, std::to_string(port_), SOCK_STREAM,
					   AI_PASSIVE | AI_V4MAPPED);

	} catch (const std::system_error &e) {
		GlobalOutput.printf("getaddrinfo() -> %d; %s", e.code().value(), e.what());
		close();
		throw TTransportException(TTransportException::NOT_OPEN,
					  "Could not resolve host for server socket.");
	}

	// we may want to try to bind more than once, since THRIFT_NO_SOCKET_CACHING doesn't
	// always seem to work. The client can configure the retry variables.
	int retries = 0;
	int errno_copy = 0;

	// -- TCP socket -- //

	auto addr_iter = AddressResolutionHelper::Iter{};

	// Via DNS or somehow else, single hostname can resolve into many addresses.
	// Results may contain perhaps a mix of IPv4 and IPv6.  Here, we iterate
	// over what system gave us, picking the first address that works.
	do {
		if (!addr_iter) {
			// init + recycle over many retries
			addr_iter = resolved_addresses.iterate();
		}
		auto trybind = *addr_iter++;

		serverSocket_ = socket(trybind->ai_family, trybind->ai_socktype, IPPROTO_TLS_1_2);
		if (serverSocket_ == -1) {
			errno_copy = THRIFT_GET_SOCKET_ERROR;
			continue;
		}

		_setup_sockopts();
		_setup_tcp_sockopts();

		static const sec_tag_t sec_tag_list[3] = {
			Thrift_TLS_CA_CERT_TAG, Thrift_TLS_SERVER_CERT_TAG, Thrift_TLS_PRIVATE_KEY};

		int ret = setsockopt(serverSocket_, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
				     sizeof(sec_tag_list));
		if (ret != 0) {
			throw TTransportException(TTransportException::NOT_OPEN,
						  "set TLS_SEC_TAG_LIST failed");
		}

#ifdef IPV6_V6ONLY
		if (trybind->ai_family == AF_INET6) {
			int zero = 0;
			if (-1 == setsockopt(serverSocket_, IPPROTO_IPV6, IPV6_V6ONLY,
					     cast_sockopt(&zero), sizeof(zero))) {
				GlobalOutput.perror("TServerSocket::listen() IPV6_V6ONLY ",
						    THRIFT_GET_SOCKET_ERROR);
			}
		}
#endif // #ifdef IPV6_V6ONLY

		if (0 == ::bind(serverSocket_, trybind->ai_addr,
				static_cast<int>(trybind->ai_addrlen))) {
			break;
		}
		errno_copy = THRIFT_GET_SOCKET_ERROR;

		// use short circuit evaluation here to only sleep if we need to
	} while ((retries++ < retryLimit_) && (THRIFT_SLEEP_SEC(retryDelay_) == 0));

	// retrieve bind info
	if (port_ == 0 && retries <= retryLimit_) {
		struct sockaddr_storage sa;
		socklen_t len = sizeof(sa);
		std::memset(&sa, 0, len);
		if (::getsockname(serverSocket_, reinterpret_cast<struct sockaddr *>(&sa), &len) <
		    0) {
			errno_copy = THRIFT_GET_SOCKET_ERROR;
			GlobalOutput.perror("TServerSocket::getPort() getsockname() ", errno_copy);
		} else {
			if (sa.ss_family == AF_INET6) {
				const auto *sin =
					reinterpret_cast<const struct sockaddr_in6 *>(&sa);
				port_ = ntohs(sin->sin6_port);
			} else {
				const auto *sin = reinterpret_cast<const struct sockaddr_in *>(&sa);
				port_ = ntohs(sin->sin_port);
			}
		}
	}

	// throw error if socket still wasn't created successfully
	if (serverSocket_ == THRIFT_INVALID_SOCKET) {
		GlobalOutput.perror("TServerSocket::listen() socket() ", errno_copy);
		close();
		throw TTransportException(TTransportException::NOT_OPEN,
					  "Could not create server socket.", errno_copy);
	}

	// throw an error if we failed to bind properly
	if (retries > retryLimit_) {
		char errbuf[1024];

		THRIFT_SNPRINTF(errbuf, sizeof(errbuf),
				"TServerSocket::listen() Could not bind to port %d", port_);

		GlobalOutput(errbuf);
		close();
		throw TTransportException(TTransportException::NOT_OPEN, "Could not bind",
					  errno_copy);
	}

	if (listenCallback_) {
		listenCallback_(serverSocket_);
	}

	// Call listen
	if (-1 == ::listen(serverSocket_, acceptBacklog_)) {
		errno_copy = THRIFT_GET_SOCKET_ERROR;
		GlobalOutput.perror("TServerSocket::listen() listen() ", errno_copy);
		close();
		throw TTransportException(TTransportException::NOT_OPEN, "Could not listen",
					  errno_copy);
	}

	// The socket is now listening!
	listening_ = true;
}

void TSSLServerSocket::close()
{
	rwMutex_.lock();
	if (pChildInterruptSockReader_ != nullptr &&
	    *pChildInterruptSockReader_ != THRIFT_INVALID_SOCKET) {
		::THRIFT_CLOSESOCKET(*pChildInterruptSockReader_);
		*pChildInterruptSockReader_ = THRIFT_INVALID_SOCKET;
	}

	rwMutex_.unlock();

	TServerSocket::close();
}

} // namespace transport
} // namespace thrift
} // namespace apache
