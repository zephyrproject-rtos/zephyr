/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _THRIFT_SERVER_TFDSERVER_H_
#define _THRIFT_SERVER_TFDSERVER_H_ 1

#include <memory>
#include <vector>

#include <thrift/transport/TServerTransport.h>

namespace apache
{
namespace thrift
{
namespace transport
{

class TFDServer : public TServerTransport
{

public:
	/**
	 * Constructor.
	 *
	 * @param fd    file descriptor of the socket
	 */
	TFDServer(int fd);
	virtual ~TFDServer();

	virtual bool isOpen() const override;
	virtual THRIFT_SOCKET getSocketFD() override;
	virtual void close() override;

	virtual void interrupt() override;
	virtual void interruptChildren() override;

protected:
	TFDServer() : TFDServer(-1){};
	virtual std::shared_ptr<TTransport> acceptImpl() override;

	int fd;
	std::vector<std::shared_ptr<TTransport>> children;
};
} // namespace transport
} // namespace thrift
} // namespace apache

#endif /* _THRIFT_SERVER_TFDSERVER_H_ */
