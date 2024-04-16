/*
 * Copyright 2006 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _THRIFT_TRANSPORT_TSSLSERVERSOCKET_H_
#define _THRIFT_TRANSPORT_TSSLSERVERSOCKET_H_ 1

#include <thrift/transport/TServerSocket.h>

namespace apache
{
namespace thrift
{
namespace transport
{

class TSSLSocketFactory;

/**
 * Server socket that accepts SSL connections.
 */
class TSSLServerSocket : public TServerSocket
{
public:
	/**
	 * Constructor.  Binds to all interfaces.
	 *
	 * @param port    Listening port
	 * @param factory SSL socket factory implementation
	 */
	TSSLServerSocket(int port, std::shared_ptr<TSSLSocketFactory> factory);

	/**
	 * Constructor.  Binds to the specified address.
	 *
	 * @param address Address to bind to
	 * @param port    Listening port
	 * @param factory SSL socket factory implementation
	 */
	TSSLServerSocket(const std::string &address, int port,
			 std::shared_ptr<TSSLSocketFactory> factory);

	/**
	 * Constructor.  Binds to all interfaces.
	 *
	 * @param port        Listening port
	 * @param sendTimeout Socket send timeout
	 * @param recvTimeout Socket receive timeout
	 * @param factory     SSL socket factory implementation
	 */
	TSSLServerSocket(int port, int sendTimeout, int recvTimeout,
			 std::shared_ptr<TSSLSocketFactory> factory);

	void listen() override;
	void close() override;

protected:
	std::shared_ptr<TSocket> createSocket(THRIFT_SOCKET socket) override;
	std::shared_ptr<TSSLSocketFactory> factory_;
};
} // namespace transport
} // namespace thrift
} // namespace apache

#endif
