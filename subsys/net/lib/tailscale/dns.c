/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "tailscale_internal.h"
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include "policy.h"
#include "socket.h"
#include "net_stack.h"

#define DNS_HEADER_SIZE        12U
#define DNS_QUESTION_TAIL_SIZE 4U
#define DNS_RECORD_TAIL_SIZE   10U
#define DNS_NAME_WIRE_SIZE     255U
#define DNS_NAME_TEXT_SIZE     (TAILSCALE_DNS_NAME_SIZE + 1U)
#define DNS_UDP_SIZE           512U
#define DNS_TCP_SIZE           2048U
#define DNS_MAX_ALIASES        8U
#define DNS_MAX_POINTERS       32U
#define DNS_A                  1U
#define DNS_CNAME              5U
#define DNS_AAAA               28U
#define DNS_IN                 1U
#define DNS_QR                 BIT(15)
#define DNS_OPCODE_MASK        (0xFU << 11)
#define DNS_TC                 BIT(9)
#define DNS_RD                 BIT(8)
#define DNS_RCODE_MASK         0xFU
#define DNS_REDIRECT           1
#define DNS_TRUNCATED          2

struct dns_name {
	uint8_t wire[DNS_NAME_WIRE_SIZE];
	size_t length;
};

struct dns_request {
	uint8_t frame[2U + DNS_HEADER_SIZE + DNS_NAME_WIRE_SIZE + DNS_QUESTION_TAIL_SIZE];
	struct dns_name name;
	size_t length;
	uint16_t id;
	uint16_t type;
};

struct dns_response {
	uint8_t packet[DNS_TCP_SIZE];
	size_t length;
	size_t answer_offset;
	uint16_t answer_count;
};

struct dns_lookup {
	uint64_t route_revision;
	int64_t deadline;
	net_sa_family_t family;
	struct tailscale_dns_result result;
};

static int check_publication(const struct dns_lookup *lookup)
{
	struct tailscale_peer_inventory_summary summary;

	tailscale_client_policy_summary(&summary);
	if (!summary.valid) {
		return -ENETDOWN;
	}
	if (summary.revision != lookup->result.revision ||
	    summary.generation != lookup->result.generation) {
		return -ESTALE;
	}
	return tailscale_client_policy_route_revision() == lookup->route_revision ? 0 : -ESTALE;
}

static int select_dns(const struct dns_lookup *lookup, const char *name,
		      struct tailscale_dns_selection *selection)
{
	int rc = tailscale_client_policy_select_dns(name, selection);

	if (rc != 0) {
		return rc;
	}
	if (selection->revision != lookup->result.revision ||
	    selection->generation != lookup->result.generation) {
		return -ESTALE;
	}
	return 0;
}

static int remaining_ms(int64_t deadline)
{
	return (int)MAX(deadline - k_uptime_get(), 0);
}

static void *address_bytes(const struct net_sockaddr_storage *addr)
{
	if (addr->ss_family == NET_AF_INET) {
		return &net_sin(net_sad(addr))->sin_addr;
	}
	return &net_sin6(net_sad(addr))->sin6_addr;
}

static void add_address(struct dns_lookup *lookup, const uint8_t *bytes)
{
	struct net_sockaddr_storage addr = {.ss_family = lookup->family};
	size_t width = lookup->family == NET_AF_INET ? sizeof(struct net_in_addr)
						     : sizeof(struct net_in6_addr);

	for (size_t i = 0; i < lookup->result.address_count; ++i) {
		if (memcmp(address_bytes(&lookup->result.addresses[i]), bytes, width) == 0) {
			return;
		}
	}
	if (lookup->result.address_count < ARRAY_SIZE(lookup->result.addresses)) {
		memcpy(address_bytes(&addr), bytes, width);
		lookup->result.addresses[lookup->result.address_count++] = addr;
	}
}

static int resolve_local(struct dns_lookup *lookup, const char *name)
{
	struct tailscale_dns_answer answer;
	int rc = tailscale_client_policy_resolve_magic_dns(name, lookup->family, &answer);

	if (rc == -ENOTSUP) {
		return -ENOENT;
	}
	if (rc != 0) {
		return rc;
	}
	if (answer.revision != lookup->result.revision ||
	    answer.generation != lookup->result.generation) {
		return -ESTALE;
	}
	for (size_t i = 0; i < answer.address_count; ++i) {
		add_address(lookup, answer.addresses[i].address);
	}
	return 0;
}

/* The policy selector has already validated the textual DNS name. */
static int make_request(const char *name, net_sa_family_t family, struct dns_request *request)
{
	uint8_t *header = request->frame + 2U;
	size_t offset = 0;

	while (name[0] != '\0' && name[0] != '.') {
		size_t label = strcspn(name, ".");

		request->name.wire[offset++] = label;
		for (size_t i = 0; i < label; ++i) {
			request->name.wire[offset++] = tolower((unsigned char)name[i]);
		}
		name += label + (name[label] == '.');
	}
	request->name.wire[offset++] = 0;
	request->name.length = offset;
	request->type = family == NET_AF_INET ? DNS_A : DNS_AAAA;
	int rc = sys_csrand_get(&request->id, sizeof(request->id));

	if (rc != 0) {
		return rc;
	}
	memset(header, 0, DNS_HEADER_SIZE);
	sys_put_be16(request->id, header);
	sys_put_be16(DNS_RD, header + 2U);
	sys_put_be16(1U, header + 4U);
	memcpy(header + DNS_HEADER_SIZE, request->name.wire, offset);
	sys_put_be16(request->type, header + DNS_HEADER_SIZE + offset);
	sys_put_be16(DNS_IN, header + DNS_HEADER_SIZE + offset + 2U);
	request->length = DNS_HEADER_SIZE + offset + DNS_QUESTION_TAIL_SIZE;
	sys_put_be16(request->length, request->frame);
	return 0;
}

static bool name_equal(const struct dns_name *left, const struct dns_name *right)
{
	return left->length == right->length && memcmp(left->wire, right->wire, left->length) == 0;
}

static int read_name(const struct dns_response *response, size_t *offset, struct dns_name *name)
{
	size_t index = *offset;
	size_t written = 0;
	unsigned int pointers = 0;

	while (index < response->length) {
		uint8_t label = response->packet[index++];

		if ((label & 0xC0U) == 0xC0U) {
			if (index >= response->length || pointers == DNS_MAX_POINTERS) {
				return -EBADMSG;
			}
			size_t target = ((label & 0x3FU) << 8) | response->packet[index++];

			if (target < DNS_HEADER_SIZE || target >= index - 2U) {
				return -EBADMSG;
			}
			if (pointers++ == 0U) {
				*offset = index;
			}
			index = target;
			continue;
		}
		if (label > 63U || label > response->length - index ||
		    written + 1U + label > sizeof(name->wire)) {
			return -EBADMSG;
		}
		name->wire[written++] = label;
		if (label == 0U) {
			name->length = written;
			if (pointers == 0U) {
				*offset = index;
			}
			return 0;
		}
		for (size_t i = 0; i < label; ++i) {
			name->wire[written++] = tolower(response->packet[index++]);
		}
	}
	return -EBADMSG;
}

static int read_header(const struct dns_request *request, struct dns_response *response)
{
	struct dns_name question;
	size_t offset = DNS_HEADER_SIZE;

	if (response->length < DNS_HEADER_SIZE || sys_get_be16(response->packet) != request->id) {
		return -EAGAIN;
	}
	uint16_t flags = sys_get_be16(response->packet + 2U);

	if ((flags & DNS_QR) == 0U || (flags & DNS_OPCODE_MASK) != 0U ||
	    sys_get_be16(response->packet + 4U) != 1U ||
	    read_name(response, &offset, &question) != 0 ||
	    response->length - offset < DNS_QUESTION_TAIL_SIZE ||
	    !name_equal(&request->name, &question) ||
	    sys_get_be16(response->packet + offset) != request->type ||
	    sys_get_be16(response->packet + offset + 2U) != DNS_IN) {
		return -EAGAIN;
	}
	if ((flags & DNS_TC) != 0U) {
		return DNS_TRUNCATED;
	}
	switch (flags & DNS_RCODE_MASK) {
	case 0:
		response->answer_offset = offset + DNS_QUESTION_TAIL_SIZE;
		response->answer_count = sys_get_be16(response->packet + 6U);
		return 0;
	case 2:
		return -EHOSTUNREACH;
	case 3:
		return -ENOENT;
	case 5:
		return -EACCES;
	default:
		return -EPROTO;
	}
}

static int wait_socket(int fd, short events, int64_t deadline)
{
	struct zsock_pollfd pfd = {.fd = fd, .events = events};
	int rc;

	do {
		int timeout = remaining_ms(deadline);

		if (timeout == 0) {
			return -ETIMEDOUT;
		}
		rc = zsock_poll(&pfd, 1, timeout);
	} while (rc < 0 && errno == EINTR);

	if (rc < 0) {
		return -errno;
	}
	if (rc == 0) {
		return -ETIMEDOUT;
	}
	return (pfd.revents & events) != 0 ? 0 : -ECONNRESET;
}

static int send_request(int fd, bool stream, const struct dns_lookup *lookup,
			const struct dns_request *request, int64_t deadline)
{
	const uint8_t *data = request->frame + (stream ? 0U : 2U);
	size_t length = request->length + (stream ? 2U : 0U);

	while (length != 0U) {
		int rc = wait_socket(fd, ZSOCK_POLLOUT, deadline);

		if (rc != 0) {
			return rc;
		}
		rc = check_publication(lookup);
		if (rc != 0) {
			return rc;
		}
		ssize_t sent = zsock_send(fd, data, length, ZSOCK_MSG_DONTWAIT);

		if (sent < 0 && (errno == EAGAIN || errno == EINTR)) {
			continue;
		}
		if (sent < 0) {
			return -errno;
		}
		if (sent == 0 || (!stream && (size_t)sent != length)) {
			return -EIO;
		}
		data += sent;
		length -= sent;
	}
	return 0;
}

static ssize_t receive(int fd, uint8_t *data, size_t length, int flags, int64_t deadline)
{
	for (;;) {
		int rc = wait_socket(fd, ZSOCK_POLLIN, deadline);

		if (rc != 0) {
			return rc;
		}
		ssize_t received = zsock_recv(fd, data, length, flags | ZSOCK_MSG_DONTWAIT);

		if (received >= 0) {
			return received;
		}
		if (errno != EAGAIN && errno != EINTR) {
			return -errno;
		}
	}
}

static int receive_exact(int fd, uint8_t *data, size_t length, int64_t deadline)
{
	while (length != 0U) {
		ssize_t n = receive(fd, data, length, 0, deadline);

		if (n <= 0) {
			return n == 0 ? -ECONNRESET : n;
		}
		data += n;
		length -= n;
	}
	return 0;
}

static int receive_response(int fd, bool stream, const struct dns_request *request,
			    struct dns_response *response, int64_t deadline)
{
	if (stream) {
		uint8_t length[2];
		int rc = receive_exact(fd, length, sizeof(length), deadline);

		if (rc != 0) {
			return rc;
		}
		response->length = sys_get_be16(length);
		if (response->length > sizeof(response->packet)) {
			return -EMSGSIZE;
		}
		rc = receive_exact(fd, response->packet, response->length, deadline);
		if (rc != 0) {
			return rc;
		}
		rc = read_header(request, response);
		return rc == DNS_TRUNCATED || rc == -EAGAIN ? -EBADMSG : rc;
	}
	for (;;) {
		ssize_t n = receive(fd, response->packet, DNS_UDP_SIZE, ZSOCK_MSG_TRUNC, deadline);

		if (n < 0) {
			return n;
		}
		response->length = MIN(n, DNS_UDP_SIZE);
		int rc = read_header(request, response);

		if (rc == -EAGAIN) {
			continue;
		}
		return (size_t)n > DNS_UDP_SIZE && rc == 0 ? DNS_TRUNCATED : rc;
	}
}

static int exchange(const struct dns_lookup *lookup, const struct tailscale_dns_server *server,
		    const struct dns_request *request, struct dns_response *response,
		    int64_t deadline, bool stream)
{
	struct net_sockaddr_storage addr = {.ss_family = server->family};
	net_socklen_t length;

	if (server->family == NET_AF_INET) {
		struct net_sockaddr_in *a = net_sin(net_sad(&addr));

		a->sin_port = net_htons(server->port);
		memcpy(&a->sin_addr, server->address, sizeof(a->sin_addr));
		length = sizeof(*a);
	} else {
		struct net_sockaddr_in6 *a = net_sin6(net_sad(&addr));

		a->sin6_port = net_htons(server->port);
		memcpy(&a->sin6_addr, server->address, sizeof(a->sin6_addr));
		length = sizeof(*a);
	}
	int rc = check_publication(lookup);

	if (rc != 0) {
		return rc;
	}
	int timeout = remaining_ms(deadline);

	if (timeout == 0) {
		return -ETIMEDOUT;
	}
	if (lookup->route_revision == 0U) {
		return -ENETDOWN;
	}
	int fd = tailscale_socket_connect_dns(stream ? NET_SOCK_STREAM : NET_SOCK_DGRAM,
					      net_sad(&addr), length, lookup->route_revision,
					      timeout);

	if (fd < 0) {
		return fd;
	}
	rc = send_request(fd, stream, lookup, request, deadline);
	if (rc == 0) {
		rc = receive_response(fd, stream, request, response, deadline);
	}
	(void)tailscale_socket_close(fd);
	int current = check_publication(lookup);

	return current == 0 ? rc : current;
}

static int read_answers(struct dns_lookup *lookup, const struct dns_response *response,
			const struct dns_name *name, uint16_t type, struct dns_name *alias)
{
	size_t offset = response->answer_offset;
	size_t width = type == DNS_A ? sizeof(struct net_in_addr) : sizeof(struct net_in6_addr);

	lookup->result.address_count = 0U;
	alias->length = 0U;
	for (size_t i = 0; i < response->answer_count; ++i) {
		struct dns_name owner;

		if (read_name(response, &offset, &owner) != 0 ||
		    response->length - offset < DNS_RECORD_TAIL_SIZE) {
			return -EBADMSG;
		}
		uint16_t record_type = sys_get_be16(response->packet + offset);
		uint16_t record_class = sys_get_be16(response->packet + offset + 2U);
		uint16_t record_len = sys_get_be16(response->packet + offset + 8U);

		offset += DNS_RECORD_TAIL_SIZE;
		if (record_len > response->length - offset) {
			return -EBADMSG;
		}
		if (record_class == DNS_IN && name_equal(name, &owner)) {
			if (record_type == type) {
				if (record_len != width) {
					return -EBADMSG;
				}
				add_address(lookup, response->packet + offset);
			} else if (record_type == DNS_CNAME) {
				size_t end = offset;

				if (alias->length != 0U || read_name(response, &end, alias) != 0 ||
				    end != offset + record_len) {
					return -EBADMSG;
				}
			}
		}
		offset += record_len;
	}
	if (alias->length != 0U) {
		return lookup->result.address_count == 0U ? DNS_REDIRECT : -EBADMSG;
	}
	return lookup->result.address_count == 0U ? -ENOENT : 0;
}

static int absolute_name(const struct dns_name *name, char text[DNS_NAME_TEXT_SIZE])
{
	size_t input = 0;
	size_t output = 0;

	while (input + 1U < name->length) {
		uint8_t length = name->wire[input++];

		for (size_t i = 0; i < length; ++i) {
			uint8_t c = name->wire[input++];

			if (!isalnum(c) && c != '-' && c != '_') {
				return -EBADMSG;
			}
			text[output++] = c;
		}
		text[output++] = '.';
	}
	if (output == 0U) {
		text[output++] = '.';
	}
	text[output] = '\0';
	return 0;
}

static bool server_selected(const struct tailscale_dns_selection *selection,
			    enum tailscale_dns_source source,
			    const struct tailscale_dns_server *server)
{
	if (selection->source != source) {
		return false;
	}
	for (size_t i = 0; i < selection->server_count; ++i) {
		const struct tailscale_dns_server *candidate = &selection->servers[i];

		if (candidate->family == server->family && candidate->port == server->port &&
		    memcmp(candidate->address, server->address, sizeof(server->address)) == 0) {
			return true;
		}
	}
	return false;
}

static int follow_answers(struct dns_lookup *lookup, const struct dns_request *request,
			  const struct dns_response *response,
			  const struct tailscale_dns_selection *selection, size_t server_index,
			  char name[DNS_NAME_TEXT_SIZE], unsigned int *aliases)
{
	struct dns_name current = request->name;
	struct dns_name alias;
	char target[DNS_NAME_TEXT_SIZE];
	unsigned int hops = *aliases;

	for (;;) {
		int rc = read_answers(lookup, response, &current, request->type, &alias);

		if (rc != DNS_REDIRECT) {
			if (rc == -ENOENT && hops != *aliases) {
				break;
			}
			return rc;
		}
		if (++hops > DNS_MAX_ALIASES || name_equal(&current, &alias)) {
			return -ELOOP;
		}
		rc = absolute_name(&alias, target);
		if (rc != 0) {
			return rc;
		}
		struct tailscale_dns_selection next;

		rc = select_dns(lookup, target, &next);
		if (rc != 0) {
			return rc;
		}
		if (!server_selected(&next, selection->source, &selection->servers[server_index])) {
			break;
		}
		current = alias;
	}
	*aliases = hops;
	strcpy(name, target);
	return DNS_REDIRECT;
}

static int resolve_forwarded(struct dns_lookup *lookup,
			     const struct tailscale_dns_selection *selection,
			     char name[DNS_NAME_TEXT_SIZE], unsigned int *aliases)
{
	struct dns_request request;
	struct dns_response response;
	int rc = make_request(name, lookup->family, &request);

	if (rc != 0) {
		return rc;
	}
	rc = -ENOENT;
	for (size_t i = 0; i < selection->server_count; ++i) {
		int64_t now = k_uptime_get();
		int64_t timeout = lookup->deadline - now;

		if (timeout <= 0) {
			return -ETIMEDOUT;
		}
		int left = selection->server_count - i;
		int64_t deadline = now + MAX(timeout / left, 1);
		const struct tailscale_dns_server *server = &selection->servers[i];

		rc = exchange(lookup, server, &request, &response, deadline, false);
		if (rc == DNS_TRUNCATED) {
			rc = exchange(lookup, server, &request, &response, deadline, true);
		}
		if (rc == 0) {
			rc = follow_answers(lookup, &request, &response, selection, i, name,
					    aliases);
		}
		if (rc == 0 || rc == DNS_REDIRECT || rc == -ENOENT || rc == -ESTALE ||
		    rc == -ENETDOWN || rc == -ELOOP) {
			break;
		}
	}
	return rc;
}

static int resolve_name(struct dns_lookup *lookup, char name[DNS_NAME_TEXT_SIZE], bool local_only)
{
	unsigned int aliases = 0U;

	do {
		struct tailscale_dns_selection selection;
		int rc = select_dns(lookup, name, &selection);

		if (rc != 0) {
			return rc;
		}
		if (selection.source == TAILSCALE_DNS_LOCAL) {
			return resolve_local(lookup, name);
		}
		if (local_only || selection.source == TAILSCALE_DNS_NONE) {
			return -ENOENT;
		}
		rc = resolve_forwarded(lookup, &selection, name, &aliases);
		if (rc != DNS_REDIRECT) {
			return rc;
		}
	} while (remaining_ms(lookup->deadline) > 0);
	return -ETIMEDOUT;
}

int tailscale_dns_resolve(const char *name, net_sa_family_t family, int32_t timeout_ms,
			struct tailscale_dns_result *result)
{
	struct tailscale_peer_inventory_summary summary;
	struct dns_lookup lookup = {0};
	char qualified[DNS_NAME_TEXT_SIZE];

	if (name == NULL || result == NULL || timeout_ms <= 0 ||
	    (family != NET_AF_INET && family != NET_AF_INET6)) {
		return -EINVAL;
	}
	size_t length = strnlen(name, sizeof(qualified));

	if (length == 0U || length == sizeof(qualified)) {
		return -EINVAL;
	}
	lookup.deadline = k_uptime_get() + timeout_ms;
	lookup.family = family;
	lookup.route_revision = tailscale_client_policy_route_revision();
	tailscale_client_policy_summary(&summary);
	if (!summary.valid) {
		return -ENETDOWN;
	}
	lookup.result.revision = summary.revision;
	lookup.result.generation = summary.generation;

	bool search = strchr(name, '.') == NULL;
	int rc;

	if (search) {
		if (length > 63U) {
			return -EINVAL;
		}
		memcpy(qualified, name, length);
		memcpy(qualified + length, ".", 2U);
		rc = resolve_name(&lookup, qualified, true);
		for (size_t index = 0; rc == -ENOENT; ++index) {
			char domain[TAILSCALE_DNS_NAME_SIZE];

			rc = tailscale_client_policy_dns_search_domain(summary.revision, index,
								       domain);
			if (rc != 0) {
				break;
			}
			size_t suffix_length = strlen(domain);

			if (length + 1U + suffix_length >= TAILSCALE_DNS_NAME_SIZE) {
				rc = -ENOENT;
				continue;
			}
			memcpy(qualified, name, length);
			qualified[length] = '.';
			memcpy(qualified + length + 1U, domain, suffix_length + 1U);
			rc = resolve_name(&lookup, qualified, false);
		}
	} else {
		memcpy(qualified, name, length + 1U);
		rc = resolve_name(&lookup, qualified, false);
	}
	int current = check_publication(&lookup);

	if (current != 0) {
		return current;
	}
	if (remaining_ms(lookup.deadline) == 0) {
		return -ETIMEDOUT;
	}
	if (rc == 0) {
		*result = lookup.result;
	}
	return rc;
}

int tailscale_net_query(const char *host, net_sa_family_t family, dns_resolve_cb_t callback,
		       void *user_data)
{
	struct tailscale_dns_result result;
	int rc = tailscale_client_policy_dns_authority(host);

	if (rc == -ENOTSUP) {
		return rc;
	}
	if (rc == 0) {
		rc = tailscale_dns_resolve(host, family, CONFIG_NET_SOCKETS_DNS_TIMEOUT, &result);
	}
	if (rc != 0) {
		switch (rc) {
		case -ENOENT:
			return DNS_EAI_NONAME;
		case -EAFNOSUPPORT:
			return DNS_EAI_ADDRFAMILY;
		case -ENETDOWN:
		case -ESTALE:
		case -ETIME:
		case -ETIMEDOUT:
			return DNS_EAI_AGAIN;
		default:
			return DNS_EAI_FAIL;
		}
	}
	for (size_t i = 0; i < result.address_count; ++i) {
		struct dns_addrinfo item = {
			.ai_family = family,
			.ai_addrlen = family == NET_AF_INET ? sizeof(struct net_sockaddr_in) :
							      sizeof(struct net_sockaddr_in6),
			.ai_addr_storage = result.addresses[i],
		};

		callback(DNS_EAI_INPROGRESS, &item, user_data);
	}
	return 0;
}
