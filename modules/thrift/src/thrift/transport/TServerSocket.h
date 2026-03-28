/*
 * Copyright 2006 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _THRIFT_TRANSPORT_TSERVERSOCKET_H_
#define _THRIFT_TRANSPORT_TSERVERSOCKET_H_ 1

#include <functional>

#include <thrift/concurrency/Mutex.h>
#include <thrift/transport/PlatformSocket.h>
#include <thrift/transport/TServerTransport.h>

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <zephyr/posix/sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <zephyr/posix/netdb.h>
#endif

namespace apache
{
namespace thrift
{
namespace transport
{

class TSocket;

/**
 * Server socket implementation of TServerTransport. Wrapper around a unix
 * socket listen and accept calls.
 *
 */
class TServerSocket : public TServerTransport
{
public:
	typedef std::function<void(THRIFT_SOCKET fd)> socket_func_t;

	const static int DEFAULT_BACKLOG = 1024;

	/**
	 * Constructor.
	 *
	 * @param port    Port number to bind to
	 */
	TServerSocket(int port);

	/**
	 * Constructor.
	 *
	 * @param port        Port number to bind to
	 * @param sendTimeout Socket send timeout
	 * @param recvTimeout Socket receive timeout
	 */
	TServerSocket(int port, int sendTimeout, int recvTimeout);

	/**
	 * Constructor.
	 *
	 * @param address Address to bind to
	 * @param port    Port number to bind to
	 */
	TServerSocket(const std::string &address, int port);

	/**
	 * Constructor used for unix sockets.
	 *
	 * @param path Pathname for unix socket.
	 */
	TServerSocket(const std::string &path);

	~TServerSocket() override;

	bool isOpen() const override;

	void setSendTimeout(int sendTimeout);
	void setRecvTimeout(int recvTimeout);

	void setAcceptTimeout(int accTimeout);
	void setAcceptBacklog(int accBacklog);

	void setRetryLimit(int retryLimit);
	void setRetryDelay(int retryDelay);

	void setKeepAlive(bool keepAlive)
	{
		keepAlive_ = keepAlive;
	}

	void setTcpSendBuffer(int tcpSendBuffer);
	void setTcpRecvBuffer(int tcpRecvBuffer);

	// listenCallback gets called just before listen, and after all Thrift
	// setsockopt calls have been made.  If you have custom setsockopt
	// things that need to happen on the listening socket, this is the place to do it.
	void setListenCallback(const socket_func_t &listenCallback)
	{
		listenCallback_ = listenCallback;
	}

	// acceptCallback gets called after each accept call, on the newly created socket.
	// It is called after all Thrift setsockopt calls have been made.  If you have
	// custom setsockopt things that need to happen on the accepted
	// socket, this is the place to do it.
	void setAcceptCallback(const socket_func_t &acceptCallback)
	{
		acceptCallback_ = acceptCallback;
	}

	// When enabled (the default), new children TSockets will be constructed so
	// they can be interrupted by TServerTransport::interruptChildren().
	// This is more expensive in terms of system calls (poll + recv) however
	// ensures a connected client cannot interfere with TServer::stop().
	//
	// When disabled, TSocket children do not incur an additional poll() call.
	// Server-side reads are more efficient, however a client can interfere with
	// the server's ability to shutdown properly by staying connected.
	//
	// Must be called before listen(); mode cannot be switched after that.
	// \throws std::logic_error if listen() has been called
	void setInterruptableChildren(bool enable);

	THRIFT_SOCKET getSocketFD() override
	{
		return serverSocket_;
	}

	int getPort() const;

	std::string getPath() const;

	bool isUnixDomainSocket() const;

	void listen() override;
	void interrupt() override;
	void interruptChildren() override;
	void close() override;

      protected:
	std::shared_ptr<TTransport> acceptImpl() override;
	virtual std::shared_ptr<TSocket> createSocket(THRIFT_SOCKET client);
	bool interruptableChildren_;
	std::shared_ptr<THRIFT_SOCKET> pChildInterruptSockReader_; // if interruptableChildren_ this
								   // is shared with child TSockets

	void _setup_sockopts();
	void _setup_tcp_sockopts();

private:
	void notify(THRIFT_SOCKET notifySock);
	void _setup_unixdomain_sockopts();

protected:
	int port_;
	std::string address_;
	std::string path_;
	THRIFT_SOCKET serverSocket_;
	int acceptBacklog_;
	int sendTimeout_;
	int recvTimeout_;
	int accTimeout_;
	int retryLimit_;
	int retryDelay_;
	int tcpSendBuffer_;
	int tcpRecvBuffer_;
	bool keepAlive_;
	bool listening_;

	concurrency::Mutex rwMutex_;	    // thread-safe interrupt
	THRIFT_SOCKET interruptSockWriter_; // is notified on interrupt()
	THRIFT_SOCKET
	interruptSockReader_; // is used in select/poll with serverSocket_ for interruptability
	THRIFT_SOCKET childInterruptSockWriter_; // is notified on interruptChildren()

	socket_func_t listenCallback_;
	socket_func_t acceptCallback_;
};
} // namespace transport
} // namespace thrift
} // namespace apache

#endif // #ifndef _THRIFT_TRANSPORT_TSERVERSOCKET_H_
