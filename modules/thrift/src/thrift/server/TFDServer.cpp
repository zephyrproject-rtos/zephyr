/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <system_error>

#include <errno.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/posix/unistd.h>

#include <thrift/transport/TFDTransport.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "thrift/server/TFDServer.h"

LOG_MODULE_REGISTER(TFDServer, LOG_LEVEL_INF);

using namespace std;

namespace apache
{
namespace thrift
{
namespace transport
{

class xport : public TVirtualTransport<xport>
{
      public:
	xport(int fd) : xport(fd, eventfd(0, EFD_SEMAPHORE))
	{
	}
	xport(int fd, int efd) : fd(fd), efd(efd)
	{
		__ASSERT(fd >= 0, "invalid fd %d", fd);
		__ASSERT(efd >= 0, "invalid efd %d", efd);

		LOG_DBG("created xport with fd %d and efd %d", fd, efd);
	}

	~xport()
	{
		close();
	}

	virtual uint32_t read_virt(uint8_t *buf, uint32_t len) override
	{
		int r;
		array<pollfd, 2> pollfds = {
			(pollfd){
				.fd = fd,
				.events = POLLIN,
				.revents = 0,
			},
			(pollfd){
				.fd = efd,
				.events = POLLIN,
				.revents = 0,
			},
		};

		if (!isOpen()) {
			return 0;
		}

		r = poll(&pollfds.front(), pollfds.size(), -1);
		if (r == -1) {
			if (efd == -1 || fd == -1) {
				/* channel has been closed */
				return 0;
			}

			LOG_ERR("failed to poll fds %d, %d: %d", fd, efd, errno);
			throw system_error(errno, system_category(), "poll");
		}

		for (auto &pfd : pollfds) {
			if (pfd.revents & POLLNVAL) {
				LOG_DBG("fd %d is invalid", pfd.fd);
				return 0;
			}
		}

		if (pollfds[0].revents & POLLIN) {
			r = ::read(fd, buf, len);
			if (r == -1) {
				LOG_ERR("failed to read %d bytes from fd %d: %d", len, fd, errno);
				system_error(errno, system_category(), "read");
			}

			__ASSERT_NO_MSG(r > 0);

			return uint32_t(r);
		}

		__ASSERT_NO_MSG(pollfds[1].revents & POLLIN);

		return 0;
	}

	virtual void write_virt(const uint8_t *buf, uint32_t len) override
	{

		if (!isOpen()) {
			throw TTransportException(TTransportException::END_OF_FILE);
		}

		for (int r = 0; len > 0; buf += r, len -= r) {
			r = ::write(fd, buf, len);
			if (r == -1) {
				LOG_ERR("writing %u bytes to fd %d failed: %d", len, fd, errno);
				throw system_error(errno, system_category(), "write");
			}

			__ASSERT_NO_MSG(r > 0);
		}
	}

	void interrupt()
	{
		if (!isOpen()) {
			return;
		}

		constexpr uint64_t x = 0xb7e;
		int r = ::write(efd, &x, sizeof(x));
		if (r == -1) {
			LOG_ERR("writing %zu bytes to fd %d failed: %d", sizeof(x), efd, errno);
			throw system_error(errno, system_category(), "write");
		}

		__ASSERT_NO_MSG(r > 0);

		LOG_DBG("interrupted xport with fd %d and efd %d", fd, efd);

		// there is no interrupt() method in the parent class, but the intent of
		// interrupt() is to prevent future communication on this transport. The
		// most reliable way we have of doing this is to close it :-)
		close();
	}

	void close() override
	{
		if (isOpen()) {
			::close(efd);
			LOG_DBG("closed xport with fd %d and efd %d", fd, efd);

			efd = -1;
			// we only have a copy of fd and do not own it
			fd = -1;
		}
	}

	bool isOpen() const override
	{
		return fd >= 0 && efd >= 0;
	}

      protected:
	int fd;
	int efd;
};

TFDServer::TFDServer(int fd) : fd(fd)
{
}

TFDServer::~TFDServer()
{
	interruptChildren();
	interrupt();
}

bool TFDServer::isOpen() const
{
	return fd >= 0;
}

shared_ptr<TTransport> TFDServer::acceptImpl()
{
	if (!isOpen()) {
		throw TTransportException(TTransportException::INTERRUPTED);
	}

	children.push_back(shared_ptr<TTransport>(new xport(fd)));

	return children.back();
}

THRIFT_SOCKET TFDServer::getSocketFD()
{
	return fd;
}

void TFDServer::close()
{
	// we only have a copy of fd and do not own it
	fd = -1;
}

void TFDServer::interrupt()
{
	close();
}

void TFDServer::interruptChildren()
{
	for (auto c : children) {
		auto child = reinterpret_cast<xport *>(c.get());
		child->interrupt();
	}

	children.clear();
}
} // namespace transport
} // namespace thrift
} // namespace apache
