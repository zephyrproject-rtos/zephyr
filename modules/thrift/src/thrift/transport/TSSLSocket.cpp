/*
 * Copyright 2006 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <thrift/thrift-config.h>

#include <cstring>
#include <errno.h>
#include <memory>
#include <string>
#ifdef HAVE_ARPA_INET_H
#include <zephyr/posix/arpa/inet.h>
#endif
#include <sys/types.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#include <zephyr/net/tls_credentials.h>

#include <fcntl.h>

#include <thrift/TToString.h>
#include <thrift/concurrency/Mutex.h>
#include <thrift/transport/PlatformSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/ThriftTLScertificateType.h>

using namespace apache::thrift::concurrency;
using std::string;

struct CRYPTO_dynlock_value {
	Mutex mutex;
};

namespace apache
{
namespace thrift
{
namespace transport
{

static bool matchName(const char *host, const char *pattern, int size);
static char uppercase(char c);

// TSSLSocket implementation
TSSLSocket::TSSLSocket(std::shared_ptr<SSLContext> ctx, std::shared_ptr<TConfiguration> config)
	: TSocket(config), server_(false), ctx_(ctx)
{
	init();
}

TSSLSocket::TSSLSocket(std::shared_ptr<SSLContext> ctx,
		       std::shared_ptr<THRIFT_SOCKET> interruptListener,
		       std::shared_ptr<TConfiguration> config)
	: TSocket(config), server_(false), ctx_(ctx)
{
	init();
	interruptListener_ = interruptListener;
}

TSSLSocket::TSSLSocket(std::shared_ptr<SSLContext> ctx, THRIFT_SOCKET socket,
		       std::shared_ptr<TConfiguration> config)
	: TSocket(socket, config), server_(false), ctx_(ctx)
{
	init();
}

TSSLSocket::TSSLSocket(std::shared_ptr<SSLContext> ctx, THRIFT_SOCKET socket,
		       std::shared_ptr<THRIFT_SOCKET> interruptListener,
		       std::shared_ptr<TConfiguration> config)
	: TSocket(socket, interruptListener, config), server_(false), ctx_(ctx)
{
	init();
}

TSSLSocket::TSSLSocket(std::shared_ptr<SSLContext> ctx, string host, int port,
		       std::shared_ptr<TConfiguration> config)
	: TSocket(host, port, config), server_(false), ctx_(ctx)
{
	init();
}

TSSLSocket::TSSLSocket(std::shared_ptr<SSLContext> ctx, string host, int port,
		       std::shared_ptr<THRIFT_SOCKET> interruptListener,
		       std::shared_ptr<TConfiguration> config)
	: TSocket(host, port, config), server_(false), ctx_(ctx)
{
	init();
	interruptListener_ = interruptListener;
}

TSSLSocket::~TSSLSocket()
{
	close();
}

template <class T> inline void *cast_sockopt(T *v)
{
	return reinterpret_cast<void *>(v);
}

void TSSLSocket::authorize()
{
}

void TSSLSocket::openSecConnection(struct addrinfo *res)
{
	socket_ = socket(res->ai_family, res->ai_socktype, ctx_->protocol);

	if (socket_ == THRIFT_INVALID_SOCKET) {
		int errno_copy = THRIFT_GET_SOCKET_ERROR;
		GlobalOutput.perror("TSocket::open() socket() " + getSocketInfo(), errno_copy);
		throw TTransportException(TTransportException::NOT_OPEN, "socket()", errno_copy);
	}

	static const sec_tag_t sec_tag_list[3] = {
		Thrift_TLS_CA_CERT_TAG, Thrift_TLS_SERVER_CERT_TAG, Thrift_TLS_PRIVATE_KEY};

	int ret =
		setsockopt(socket_, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list));
	if (ret != 0) {
		throw TTransportException(TTransportException::NOT_OPEN,
					  "set TLS_SEC_TAG_LIST failed");
	}

	ret = setsockopt(socket_, SOL_TLS, TLS_PEER_VERIFY, &(ctx_->verifyMode),
			 sizeof(ctx_->verifyMode));
	if (ret != 0) {
		throw TTransportException(TTransportException::NOT_OPEN,
					  "set TLS_PEER_VERIFY failed");
	}

	ret = setsockopt(socket_, SOL_TLS, TLS_HOSTNAME, host_.c_str(), host_.size());
	if (ret != 0) {
		throw TTransportException(TTransportException::NOT_OPEN, "set TLS_HOSTNAME failed");
	}

	// Send timeout
	if (sendTimeout_ > 0) {
		setSendTimeout(sendTimeout_);
	}

	// Recv timeout
	if (recvTimeout_ > 0) {
		setRecvTimeout(recvTimeout_);
	}

	if (keepAlive_) {
		setKeepAlive(keepAlive_);
	}

	// Linger
	setLinger(lingerOn_, lingerVal_);

	// No delay
	setNoDelay(noDelay_);

#ifdef SO_NOSIGPIPE
	{
		int one = 1;
		setsockopt(socket_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
	}
#endif

// Uses a low min RTO if asked to.
#ifdef TCP_LOW_MIN_RTO
	if (getUseLowMinRto()) {
		int one = 1;
		setsockopt(socket_, IPPROTO_TCP, TCP_LOW_MIN_RTO, &one, sizeof(one));
	}
#endif

	// Set the socket to be non blocking for connect if a timeout exists
	int flags = THRIFT_FCNTL(socket_, THRIFT_F_GETFL, 0);
	if (connTimeout_ > 0) {
		if (-1 == THRIFT_FCNTL(socket_, THRIFT_F_SETFL, flags | THRIFT_O_NONBLOCK)) {
			int errno_copy = THRIFT_GET_SOCKET_ERROR;
			GlobalOutput.perror("TSocket::open() THRIFT_FCNTL() " + getSocketInfo(),
					    errno_copy);
			throw TTransportException(TTransportException::NOT_OPEN,
						  "THRIFT_FCNTL() failed", errno_copy);
		}
	} else {
		if (-1 == THRIFT_FCNTL(socket_, THRIFT_F_SETFL, flags & ~THRIFT_O_NONBLOCK)) {
			int errno_copy = THRIFT_GET_SOCKET_ERROR;
			GlobalOutput.perror("TSocket::open() THRIFT_FCNTL " + getSocketInfo(),
					    errno_copy);
			throw TTransportException(TTransportException::NOT_OPEN,
						  "THRIFT_FCNTL() failed", errno_copy);
		}
	}

	// Connect the socket

	ret = connect(socket_, res->ai_addr, static_cast<int>(res->ai_addrlen));

	// success case
	if (ret == 0) {
		goto done;
	}

	if ((THRIFT_GET_SOCKET_ERROR != THRIFT_EINPROGRESS) &&
	    (THRIFT_GET_SOCKET_ERROR != THRIFT_EWOULDBLOCK)) {
		int errno_copy = THRIFT_GET_SOCKET_ERROR;
		GlobalOutput.perror("TSocket::open() connect() " + getSocketInfo(), errno_copy);
		throw TTransportException(TTransportException::NOT_OPEN, "connect() failed",
					  errno_copy);
	}

	struct THRIFT_POLLFD fds[1];
	std::memset(fds, 0, sizeof(fds));
	fds[0].fd = socket_;
	fds[0].events = THRIFT_POLLOUT;
	ret = THRIFT_POLL(fds, 1, connTimeout_);

	if (ret > 0) {
		// Ensure the socket is connected and that there are no errors set
		int val;
		socklen_t lon;
		lon = sizeof(int);
		int ret2 = getsockopt(socket_, SOL_SOCKET, SO_ERROR, cast_sockopt(&val), &lon);
		if (ret2 == -1) {
			int errno_copy = THRIFT_GET_SOCKET_ERROR;
			GlobalOutput.perror("TSocket::open() getsockopt() " + getSocketInfo(),
					    errno_copy);
			throw TTransportException(TTransportException::NOT_OPEN, "getsockopt()",
						  errno_copy);
		}
		// no errors on socket, go to town
		if (val == 0) {
			goto done;
		}
		GlobalOutput.perror("TSocket::open() error on socket (after THRIFT_POLL) " +
					    getSocketInfo(),
				    val);
		throw TTransportException(TTransportException::NOT_OPEN, "socket open() error",
					  val);
	} else if (ret == 0) {
		// socket timed out
		string errStr = "TSocket::open() timed out " + getSocketInfo();
		GlobalOutput(errStr.c_str());
		throw TTransportException(TTransportException::NOT_OPEN, "open() timed out");
	} else {
		// error on THRIFT_POLL()
		int errno_copy = THRIFT_GET_SOCKET_ERROR;
		GlobalOutput.perror("TSocket::open() THRIFT_POLL() " + getSocketInfo(), errno_copy);
		throw TTransportException(TTransportException::NOT_OPEN, "THRIFT_POLL() failed",
					  errno_copy);
	}

done:
	// Set socket back to normal mode (blocking)
	if (-1 == THRIFT_FCNTL(socket_, THRIFT_F_SETFL, flags)) {
		int errno_copy = THRIFT_GET_SOCKET_ERROR;
		GlobalOutput.perror("TSocket::open() THRIFT_FCNTL " + getSocketInfo(), errno_copy);
		throw TTransportException(TTransportException::NOT_OPEN, "THRIFT_FCNTL() failed",
					  errno_copy);
	}

	setCachedAddress(res->ai_addr, static_cast<socklen_t>(res->ai_addrlen));
}

void TSSLSocket::init()
{
	handshakeCompleted_ = false;
	readRetryCount_ = 0;
	eventSafe_ = false;
}

void TSSLSocket::open()
{
	if (isOpen() || server()) {
		throw TTransportException(TTransportException::BAD_ARGS);
	}

	// Validate port number
	if (port_ < 0 || port_ > 0xFFFF) {
		throw TTransportException(TTransportException::BAD_ARGS,
					  "Specified port is invalid");
	}

	struct addrinfo hints, *res, *res0;
	res = nullptr;
	res0 = nullptr;
	int error;
	char port[sizeof("65535")];
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	sprintf(port, "%d", port_);

	error = getaddrinfo(host_.c_str(), port, &hints, &res0);

	if (error == DNS_EAI_NODATA) {
		hints.ai_flags &= ~AI_ADDRCONFIG;
		error = getaddrinfo(host_.c_str(), port, &hints, &res0);
	}

	if (error) {
		string errStr = "TSocket::open() getaddrinfo() " + getSocketInfo() +
				string(THRIFT_GAI_STRERROR(error));
		GlobalOutput(errStr.c_str());
		close();
		throw TTransportException(TTransportException::NOT_OPEN,
					  "Could not resolve host for client socket.");
	}

	// Cycle through all the returned addresses until one
	// connects or push the exception up.
	for (res = res0; res; res = res->ai_next) {
		try {
			openSecConnection(res);
			break;
		} catch (TTransportException &) {
			if (res->ai_next) {
				close();
			} else {
				close();
				freeaddrinfo(res0); // cleanup on failure
				throw;
			}
		}
	}

	// Free address structure memory
	freeaddrinfo(res0);
}

TSSLSocketFactory::TSSLSocketFactory(SSLProtocol protocol)
	: ctx_(std::make_shared<SSLContext>()), server_(false)
{
	switch (protocol) {
	case SSLTLS:
		break;
	case TLSv1_0:
		break;
	case TLSv1_1:
		ctx_->protocol = IPPROTO_TLS_1_1;
		break;
	case TLSv1_2:
		ctx_->protocol = IPPROTO_TLS_1_2;
		break;
	default:
		throw TTransportException(TTransportException::BAD_ARGS,
					  "Specified protocol is invalid");
	}
}

TSSLSocketFactory::~TSSLSocketFactory()
{
}

std::shared_ptr<TSSLSocket> TSSLSocketFactory::createSocket()
{
	std::shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_));
	setup(ssl);
	return ssl;
}

std::shared_ptr<TSSLSocket>
TSSLSocketFactory::createSocket(std::shared_ptr<THRIFT_SOCKET> interruptListener)
{
	std::shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_, interruptListener));
	setup(ssl);
	return ssl;
}

std::shared_ptr<TSSLSocket> TSSLSocketFactory::createSocket(THRIFT_SOCKET socket)
{
	std::shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_, socket));
	setup(ssl);
	return ssl;
}

std::shared_ptr<TSSLSocket>
TSSLSocketFactory::createSocket(THRIFT_SOCKET socket,
				std::shared_ptr<THRIFT_SOCKET> interruptListener)
{
	std::shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_, socket, interruptListener));
	setup(ssl);
	return ssl;
}

std::shared_ptr<TSSLSocket> TSSLSocketFactory::createSocket(const string &host, int port)
{
	std::shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_, host, port));
	setup(ssl);
	return ssl;
}

std::shared_ptr<TSSLSocket>
TSSLSocketFactory::createSocket(const string &host, int port,
				std::shared_ptr<THRIFT_SOCKET> interruptListener)
{
	std::shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_, host, port, interruptListener));
	setup(ssl);
	return ssl;
}

static void tlsCredtErrMsg(string &errors, const int status);

void TSSLSocketFactory::setup(std::shared_ptr<TSSLSocket> ssl)
{
	ssl->server(server());
	if (access_ == nullptr && !server()) {
		access_ = std::shared_ptr<AccessManager>(new DefaultClientAccessManager);
	}
	if (access_ != nullptr) {
		ssl->access(access_);
	}
}

void TSSLSocketFactory::ciphers(const string &enable)
{
}

void TSSLSocketFactory::authenticate(bool required)
{
	if (required) {
		ctx_->verifyMode = TLS_PEER_VERIFY_REQUIRED;
	} else {
		ctx_->verifyMode = TLS_PEER_VERIFY_NONE;
	}
}

void TSSLSocketFactory::loadCertificate(const char *path, const char *format)
{
	if (path == nullptr || format == nullptr) {
		throw TTransportException(
			TTransportException::BAD_ARGS,
			"loadCertificateChain: either <path> or <format> is nullptr");
	}
	if (strcmp(format, "PEM") == 0) {

	} else {
		throw TSSLException("Unsupported certificate format: " + string(format));
	}
}

void TSSLSocketFactory::loadCertificateFromBuffer(const char *aCertificate, const char *format)
{
	if (aCertificate == nullptr || format == nullptr) {
		throw TTransportException(TTransportException::BAD_ARGS,
					  "loadCertificate: either <path> or <format> is nullptr");
	}

	if (strcmp(format, "PEM") == 0) {
		const int status = tls_credential_add(Thrift_TLS_SERVER_CERT_TAG,
						      TLS_CREDENTIAL_SERVER_CERTIFICATE,
						      aCertificate, strlen(aCertificate) + 1);

		if (status != 0) {
			string errors;
			tlsCredtErrMsg(errors, status);
			throw TSSLException("tls_credential_add: " + errors);
		}
	} else {
		throw TSSLException("Unsupported certificate format: " + string(format));
	}
}

void TSSLSocketFactory::loadPrivateKey(const char *path, const char *format)
{
	if (path == nullptr || format == nullptr) {
		throw TTransportException(TTransportException::BAD_ARGS,
					  "loadPrivateKey: either <path> or <format> is nullptr");
	}
	if (strcmp(format, "PEM") == 0) {
		if (0) {
			string errors;
			// tlsCredtErrMsg(errors, status);
			throw TSSLException("SSL_CTX_use_PrivateKey_file: " + errors);
		}
	}
}

void TSSLSocketFactory::loadPrivateKeyFromBuffer(const char *aPrivateKey, const char *format)
{
	if (aPrivateKey == nullptr || format == nullptr) {
		throw TTransportException(TTransportException::BAD_ARGS,
					  "loadPrivateKey: either <path> or <format> is nullptr");
	}
	if (strcmp(format, "PEM") == 0) {
		const int status =
			tls_credential_add(Thrift_TLS_PRIVATE_KEY, TLS_CREDENTIAL_PRIVATE_KEY,
					   aPrivateKey, strlen(aPrivateKey) + 1);

		if (status != 0) {
			string errors;
			tlsCredtErrMsg(errors, status);
			throw TSSLException("SSL_CTX_use_PrivateKey: " + errors);
		}
	} else {
		throw TSSLException("Unsupported certificate format: " + string(format));
	}
}

void TSSLSocketFactory::loadTrustedCertificates(const char *path, const char *capath)
{
	if (path == nullptr) {
		throw TTransportException(TTransportException::BAD_ARGS,
					  "loadTrustedCertificates: <path> is nullptr");
	}
	if (0) {
		string errors;
		// tlsCredtErrMsg(errors, status);
		throw TSSLException("SSL_CTX_load_verify_locations: " + errors);
	}
}

void TSSLSocketFactory::loadTrustedCertificatesFromBuffer(const char *aCertificate,
							  const char *aChain)
{
	if (aCertificate == nullptr) {
		throw TTransportException(TTransportException::BAD_ARGS,
					  "loadTrustedCertificates: aCertificate is empty");
	}
	const int status = tls_credential_add(Thrift_TLS_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
					      aCertificate, strlen(aCertificate) + 1);

	if (status != 0) {
		string errors;
		tlsCredtErrMsg(errors, status);
		throw TSSLException("X509_STORE_add_cert: " + errors);
	}

	if (aChain) {
	}
}

void TSSLSocketFactory::randomize()
{
}

void TSSLSocketFactory::overrideDefaultPasswordCallback()
{
}

void TSSLSocketFactory::server(bool flag)
{
	server_ = flag;
	ctx_->verifyMode = TLS_PEER_VERIFY_NONE;
}

bool TSSLSocketFactory::server() const
{
	return server_;
}

int TSSLSocketFactory::passwordCallback(char *password, int size, int, void *data)
{
	auto *factory = (TSSLSocketFactory *)data;
	string userPassword;
	factory->getPassword(userPassword, size);
	int length = static_cast<int>(userPassword.size());
	if (length > size) {
		length = size;
	}
	strncpy(password, userPassword.c_str(), length);
	userPassword.assign(userPassword.size(), '*');
	return length;
}

// extract error messages from error queue
static void tlsCredtErrMsg(string &errors, const int status)
{
	if (status == EACCES) {
		errors = "Access to the TLS credential subsystem was denied";
	} else if (status == ENOMEM) {
		errors = "Not enough memory to add new TLS credential";
	} else if (status == EEXIST) {
		errors = "TLS credential of specific tag and type already exists";
	} else {
		errors = "Unknown error";
	}
}

/**
 * Default implementation of AccessManager
 */
Decision DefaultClientAccessManager::verify(const sockaddr_storage &sa) noexcept
{
	(void)sa;
	return SKIP;
}

Decision DefaultClientAccessManager::verify(const string &host, const char *name, int size) noexcept
{
	if (host.empty() || name == nullptr || size <= 0) {
		return SKIP;
	}
	return (matchName(host.c_str(), name, size) ? ALLOW : SKIP);
}

Decision DefaultClientAccessManager::verify(const sockaddr_storage &sa, const char *data,
					    int size) noexcept
{
	bool match = false;
	if (sa.ss_family == AF_INET && size == sizeof(in_addr)) {
		match = (memcmp(&((sockaddr_in *)&sa)->sin_addr, data, size) == 0);
	} else if (sa.ss_family == AF_INET6 && size == sizeof(in6_addr)) {
		match = (memcmp(&((sockaddr_in6 *)&sa)->sin6_addr, data, size) == 0);
	}
	return (match ? ALLOW : SKIP);
}

/**
 * Match a name with a pattern. The pattern may include wildcard. A single
 * wildcard "*" can match up to one component in the domain name.
 *
 * @param  host    Host name, typically the name of the remote host
 * @param  pattern Name retrieved from certificate
 * @param  size    Size of "pattern"
 * @return True, if "host" matches "pattern". False otherwise.
 */
bool matchName(const char *host, const char *pattern, int size)
{
	bool match = false;
	int i = 0, j = 0;
	while (i < size && host[j] != '\0') {
		if (uppercase(pattern[i]) == uppercase(host[j])) {
			i++;
			j++;
			continue;
		}
		if (pattern[i] == '*') {
			while (host[j] != '.' && host[j] != '\0') {
				j++;
			}
			i++;
			continue;
		}
		break;
	}
	if (i == size && host[j] == '\0') {
		match = true;
	}
	return match;
}

// This is to work around the Turkish locale issue, i.e.,
// toupper('i') != toupper('I') if locale is "tr_TR"
char uppercase(char c)
{
	if ('a' <= c && c <= 'z') {
		return c + ('A' - 'a');
	}
	return c;
}
} // namespace transport
} // namespace thrift
} // namespace apache
