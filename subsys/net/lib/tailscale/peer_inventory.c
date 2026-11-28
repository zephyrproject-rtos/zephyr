/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <zephyr/net/net_ip.h>
#include "tailscale_internal.h"
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_utf8.h>

#include "policy_internal.h"

#define MAP_JSON_MAX_BYTES 65536
#define MAP_JSON_MAX_DEPTH 16
#define MAP_JSON_MAX_FIELDS 64
#define GO_ZERO_UNIX_SECONDS INT64_C(-62135596800)

struct json_span {
	const char *begin;
	const char *end;
};

struct json_cursor {
	const char *next;
	const char *end;
};

static void json_space(struct json_cursor *cursor)
{
	while (cursor->next != cursor->end &&
	       (*cursor->next == ' ' || *cursor->next == '\t' ||
		*cursor->next == '\r' || *cursor->next == '\n')) {
		cursor->next++;
	}
}

static int json_peek(struct json_cursor *cursor)
{
	json_space(cursor);
	return cursor->next == cursor->end ? -1 : (unsigned char)*cursor->next;
}

static int json_pop(struct json_cursor *cursor)
{
	int ch = json_peek(cursor);

	if (ch >= 0) {
		cursor->next++;
	}
	return ch;
}

static bool json_take(struct json_cursor *cursor, const char *text, size_t len)
{
	if ((size_t)(cursor->end - cursor->next) < len ||
	    memcmp(cursor->next, text, len) != 0) {
		return false;
	}
	cursor->next += len;
	return true;
}

static int json_string(struct json_cursor *cursor)
{
	if (json_pop(cursor) != '"') {
		return -EBADMSG;
	}
	while (cursor->next != cursor->end) {
		unsigned char ch = *cursor->next++;

		if (ch == '"') {
			return 0;
		}
		if (ch < 0x20) {
			break;
		}
		if (ch != '\\') {
			continue;
		}
		if (cursor->next == cursor->end) {
			break;
		}
		ch = *cursor->next++;
		if (ch == 'u') {
			for (size_t i = 0; i < 4; i++) {
				if (cursor->next == cursor->end ||
				    !isxdigit((unsigned char)*cursor->next++)) {
					return -EBADMSG;
				}
			}
		} else if (ch == 0 || strchr("\"\\/bfnrt", ch) == NULL) {
			break;
		}
	}
	return -EBADMSG;
}

static bool json_digit(const struct json_cursor *cursor)
{
	return cursor->next != cursor->end && isdigit((unsigned char)*cursor->next);
}

static bool json_digits(struct json_cursor *cursor)
{
	const char *start = cursor->next;

	while (json_digit(cursor)) {
		cursor->next++;
	}
	return cursor->next != start;
}

static bool json_number(struct json_cursor *cursor)
{
	(void)json_take(cursor, "-", 1);

	if (json_take(cursor, "0", 1)) {
		if (json_digit(cursor)) {
			return false;
		}
	} else if (!json_digits(cursor)) {
		return false;
	}
	if (json_take(cursor, ".", 1) && !json_digits(cursor)) {
		return false;
	}
	if (json_take(cursor, "e", 1) || json_take(cursor, "E", 1)) {
		if (!json_take(cursor, "+", 1)) {
			(void)json_take(cursor, "-", 1);
		}
		return json_digits(cursor);
	}
	return true;
}

static int json_value(struct json_cursor *cursor, unsigned int depth)
{
	int opening = json_peek(cursor);
	int closing;

	switch (opening) {
	case '"':
		return json_string(cursor);
	case 't':
		return json_take(cursor, "true", 4) ? 0 : -EBADMSG;
	case 'f':
		return json_take(cursor, "false", 5) ? 0 : -EBADMSG;
	case 'n':
		return json_take(cursor, "null", 4) ? 0 : -EBADMSG;
	case '{':
	case '[':
		break;
	default:
		return json_number(cursor) ? 0 : -EBADMSG;
	}

	if (depth == MAP_JSON_MAX_DEPTH) {
		return -E2BIG;
	}
	cursor->next++;
	closing = opening == '{' ? '}' : ']';
	if (json_peek(cursor) == closing) {
		cursor->next++;
		return 0;
	}

	for (;;) {
		if (opening == '{' && (json_string(cursor) != 0 || json_pop(cursor) != ':')) {
			return -EBADMSG;
		}
		int ret = json_value(cursor, depth + 1);

		if (ret != 0) {
			return ret;
		}
		int separator = json_pop(cursor);

		if (separator == closing) {
			return 0;
		}
		if (separator != ',') {
			return -EBADMSG;
		}
	}
}

static struct json_span json_extract(struct json_cursor *cursor)
{
	json_space(cursor);
	struct json_span value = {.begin = cursor->next};

	/* The entire document has already been validated by json_value(). */
	(void)json_value(cursor, 0);
	value.end = cursor->next;
	return value;
}

static struct json_cursor json_elements(struct json_span container)
{
	if (container.begin == NULL || *container.begin == 'n') {
		return (struct json_cursor){0};
	}
	return (struct json_cursor){container.begin + 1, container.end - 1};
}

static bool json_has_element(struct json_cursor *cursor)
{
	json_space(cursor);
	return cursor->next != cursor->end;
}

static void json_separator(struct json_cursor *cursor)
{
	if (json_has_element(cursor)) {
		cursor->next++;
	}
}

static char json_kind(struct json_span span)
{
	return span.begin == NULL ? '\0' : *span.begin;
}

static bool json_null(struct json_span span)
{
	return span.begin == NULL || *span.begin == 'n';
}

static bool nonempty_container(struct json_span token)
{
	struct json_cursor cursor = json_elements(token);

	return json_has_element(&cursor);
}

static struct json_span string_value(struct json_span span)
{
	return (struct json_span){span.begin + 1, span.end - 1};
}

static bool spans_equal(struct json_span left, struct json_span right)
{
	if (left.end - left.begin != right.end - right.begin) {
		return false;
	}
	while (left.begin != left.end) {
		if (tolower((unsigned char)*left.begin++) !=
		    tolower((unsigned char)*right.begin++)) {
			return false;
		}
	}
	return true;
}

static int json_fields(struct json_span object, const char *const names[], size_t count,
		       struct json_span fields[], bool strict)
{
	struct json_span seen[MAP_JSON_MAX_FIELDS];
	size_t nseen = 0;
	struct json_cursor cursor = json_elements(object);

	if (json_kind(object) != '{') {
		return -EBADMSG;
	}
	while (json_has_element(&cursor)) {
		struct json_span key = string_value(json_extract(&cursor));

		if (nseen == ARRAY_SIZE(seen)) {
			return -E2BIG;
		}
		/* Control field names are ASCII. Reject escapes that could hide duplicates. */
		if (memchr(key.begin, '\\', key.end - key.begin) != NULL) {
			return -EBADMSG;
		}
		for (size_t i = 0; i < nseen; i++) {
			if (spans_equal(key, seen[i])) {
				return -EBADMSG;
			}
		}
		seen[nseen++] = key;
		(void)json_pop(&cursor);
		struct json_span value = json_extract(&cursor);

		size_t i;

		for (i = 0; i < count; i++) {
			struct json_span name = {names[i], names[i] + strlen(names[i])};

			if (spans_equal(key, name)) {
				fields[i] = value;
				break;
			}
		}
		if (strict && i == count) {
			return -ENOTSUP;
		}
		json_separator(&cursor);
	}

	return 0;
}

static int uint_value(struct json_span token, uint64_t *number)
{
	uint64_t value = 0;

	if (json_null(token) || token.begin == token.end ||
	    (*token.begin == '0' && token.end - token.begin != 1)) {
		return -EBADMSG;
	}
	for (const char *p = token.begin; p != token.end; p++) {
		if (!isdigit((unsigned char)*p) || value > (INT64_MAX - (*p - '0')) / 10ULL) {
			return -EBADMSG;
		}
		value = value * 10 + (*p - '0');
	}
	*number = value;
	return 0;
}

static int bool_value(struct json_span token, bool *value)
{
	switch (json_kind(token)) {
	case 't':
		*value = true;
		return 0;
	case 'f':
		*value = false;
		return 0;
	default:
		return -EBADMSG;
	}
}

static int text_value(struct json_span token, char *text, size_t capacity)
{
	if (json_kind(token) != '"') {
		return -EBADMSG;
	}
	struct json_span value = string_value(token);
	size_t len = value.end - value.begin;

	if (len >= capacity) {
		return -E2BIG;
	}
	if (memchr(value.begin, '\\', len) != NULL) {
		return -EBADMSG;
	}
	memcpy(text, value.begin, len);
	text[len] = '\0';
	return 0;
}

static int key_value(struct json_span token, const char *prefix, uint8_t key[], bool allow_zero)
{
	size_t prefix_len = strlen(prefix);

	if (json_kind(token) != '"') {
		return -EBADMSG;
	}
	struct json_span value = string_value(token);

	if ((size_t)(value.end - value.begin) != prefix_len + 2 * TAILSCALE_PEER_PUBLIC_KEY_SIZE ||
	    memcmp(value.begin, prefix, prefix_len) != 0 ||
	    hex2bin(value.begin + prefix_len, 2 * TAILSCALE_PEER_PUBLIC_KEY_SIZE,
		    key, TAILSCALE_PEER_PUBLIC_KEY_SIZE) != TAILSCALE_PEER_PUBLIC_KEY_SIZE) {
		return -EBADMSG;
	}
	for (size_t i = 0; i < TAILSCALE_PEER_PUBLIC_KEY_SIZE; i++) {
		if (key[i] != 0) {
			return 0;
		}
	}
	return allow_zero ? 0 : -EBADMSG;
}

static int parse_prefix(char *text, struct tailscale_peer_prefix *prefix, bool allow_host)
{
	char *slash = strrchr(text, '/');
	bool ipv6 = strchr(text, ':') != NULL;
	uint64_t max_length = ipv6 ? 128U : 32U;
	uint64_t length;

	if (slash != NULL) {
		int rc = uint_value((struct json_span){slash + 1, text + strlen(text)}, &length);

		if (rc != 0) {
			return rc;
		}
		*slash = '\0';
	} else if (allow_host) {
		length = max_length;
	} else {
		return -EBADMSG;
	}

	prefix->family = ipv6 ? NET_AF_INET6 : NET_AF_INET;
	if (length > max_length ||
	    net_addr_pton(prefix->family, text, prefix->address) != 0) {
		return -EBADMSG;
	}
	prefix->length = length;
	for (size_t i = 0; i < sizeof(prefix->address); i++) {
		unsigned int bits = length > 8 ? 8 : length;

		prefix->address[i] &= (uint8_t)(0xffU << (8U - bits));
		length -= bits;
	}
	return 0;
}

static int prefix_value(struct json_span token, struct tailscale_peer_prefix *prefix)
{
	char text[NET_INET6_ADDRSTRLEN + 5];
	int rc = text_value(token, text, sizeof(text));

	return rc == 0 ? parse_prefix(text, prefix, false) : rc;
}

static int prefix_array(struct json_span array, struct tailscale_peer_prefix prefixes[],
			size_t capacity, uint8_t *count)
{
	struct json_cursor cursor = json_elements(array);

	*count = 0;
	while (json_has_element(&cursor)) {
		if (*count == capacity) {
			return -E2BIG;
		}
		int rc = prefix_value(json_extract(&cursor), &prefixes[*count]);

		if (rc != 0) {
			return rc;
		}
		(*count)++;
		json_separator(&cursor);
	}
	return 0;
}

static int endpoint_value(struct json_span token, struct tailscale_peer_endpoint *endpoint)
{
	char text[NET_INET6_ADDRSTRLEN + 8];
	char *host = text;
	char *port;
	uint64_t number;
	int rc = text_value(token, text, sizeof(text));

	if (rc != 0) {
		return rc;
	}
	port = strrchr(text, ':');
	if (port == NULL) {
		return -EBADMSG;
	}
	*port++ = '\0';
	if (text[0] == '[') {
		size_t len = strlen(text);

		if (len < 3U || text[len - 1U] != ']') {
			return -EBADMSG;
		}
		text[len - 1U] = '\0';
		host++;
		endpoint->family = NET_AF_INET6;
	} else {
		endpoint->family = NET_AF_INET;
	}
	/* A map describes remote candidates independently of the local IP stack. */
	rc = uint_value((struct json_span){port, port + strlen(port)}, &number);
	if (rc != 0 || number == 0U || number > UINT16_MAX ||
	    net_addr_pton(endpoint->family, host, endpoint->address) != 0) {
		return -EBADMSG;
	}
	endpoint->port = number;
	return 0;
}

static int endpoint_array(struct json_span array, struct tailscale_peer *peer)
{
	struct json_cursor cursor = json_elements(array);

	peer->endpoint_count = 0;
	memset(peer->endpoints, 0, sizeof(peer->endpoints));
	while (json_has_element(&cursor)) {
		int rc;

		if (peer->endpoint_count == ARRAY_SIZE(peer->endpoints)) {
			return -E2BIG;
		}
		rc = endpoint_value(json_extract(&cursor), &peer->endpoints[peer->endpoint_count]);
		if (rc != 0) {
			return rc;
		}
		peer->endpoint_count++;
		json_separator(&cursor);
	}
	return 0;
}

enum peer_field {
	PEER_ID,
	PEER_PATCH_ID,
	PEER_STABLE_ID,
	PEER_KEY,
	PEER_DISCO_KEY,
	PEER_KEY_EXPIRY,
	PEER_KEY_SIGNATURE,
	PEER_ADDRESSES,
	PEER_ALLOWED_IPS,
	PEER_ENDPOINTS,
	PEER_ONLINE,
	PEER_DERP,
	PEER_PATCH_DERP,
	PEER_EXPIRED,
	PEER_UNSIGNED,
	PEER_AUTHORIZED,
	PEER_CAP_MAP,
	PEER_CAPABILITIES,
	PEER_PRIMARY_ROUTES,
	PEER_LEGACY_DERP,
	PEER_NAME,
	PEER_USER,
	PEER_SHARER,
	PEER_MACHINE,
	PEER_HOSTINFO,
	PEER_CREATED,
	PEER_CAP,
	PEER_TAGS,
	PEER_LAST_SEEN,
	PEER_FIELD_COUNT,
};

static const char *const peer_names[] = {
	[PEER_ID] = "ID",
	[PEER_PATCH_ID] = "NodeID",
	[PEER_STABLE_ID] = "StableID",
	[PEER_KEY] = "Key",
	[PEER_DISCO_KEY] = "DiscoKey",
	[PEER_KEY_EXPIRY] = "KeyExpiry",
	[PEER_KEY_SIGNATURE] = "KeySignature",
	[PEER_ADDRESSES] = "Addresses",
	[PEER_ALLOWED_IPS] = "AllowedIPs",
	[PEER_ENDPOINTS] = "Endpoints",
	[PEER_ONLINE] = "Online",
	[PEER_DERP] = "HomeDERP",
	[PEER_PATCH_DERP] = "DERPRegion",
	[PEER_EXPIRED] = "Expired",
	[PEER_UNSIGNED] = "UnsignedPeerAPIOnly",
	[PEER_AUTHORIZED] = "MachineAuthorized",
	[PEER_CAP_MAP] = "CapMap",
	[PEER_CAPABILITIES] = "Capabilities",
	[PEER_PRIMARY_ROUTES] = "PrimaryRoutes",
	[PEER_LEGACY_DERP] = "DERP",
	[PEER_NAME] = "Name",
	[PEER_USER] = "User",
	[PEER_SHARER] = "Sharer",
	[PEER_MACHINE] = "Machine",
	[PEER_HOSTINFO] = "Hostinfo",
	[PEER_CREATED] = "Created",
	[PEER_CAP] = "Cap",
	[PEER_TAGS] = "Tags",
	[PEER_LAST_SEEN] = "LastSeen",
};

static int apply_peer_fields(struct tailscale_peer *peer,
			     const struct json_span fields[PEER_FIELD_COUNT], bool patch)
{
	uint64_t derp;
	bool online;

	for (enum peer_field field = 0; field < PEER_FIELD_COUNT; field++) {
		struct json_span value = fields[field];
		int rc = 0;

		if (json_null(value)) {
			continue;
		}
		switch (field) {
		case PEER_KEY:
			rc = key_value(value, "nodekey:", peer->key, false);
			break;
		case PEER_DISCO_KEY:
			rc = key_value(value, "discokey:", peer->disco_key, true);
			break;
		case PEER_KEY_EXPIRY:
			rc = text_value(value, peer->key_expiry, sizeof(peer->key_expiry));
			break;
		case PEER_KEY_SIGNATURE:
			if (json_kind(value) != '"') {
				return -EBADMSG;
			}
			peer->key_signature_present = value.end - value.begin > 2;
			break;
		case PEER_ENDPOINTS:
			if (json_kind(value) != '[') {
				return -EBADMSG;
			}
			rc = endpoint_array(value, peer);
			break;
		case PEER_ONLINE:
			rc = bool_value(value, &online);
			if (rc == 0) {
				peer->online = online ? TAILSCALE_PEER_ONLINE :
							TAILSCALE_PEER_OFFLINE;
			}
			break;
		case PEER_DERP:
		case PEER_PATCH_DERP:
			if ((field == PEER_PATCH_DERP) == patch) {
				rc = uint_value(value, &derp);
				if (rc != 0 || derp > UINT16_MAX) {
					return -EBADMSG;
				}
				if (!patch || derp != 0) {
					peer->derp_region = derp;
				}
			}
			break;
		case PEER_STABLE_ID:
			if (!patch) {
				rc = text_value(value, peer->stable_id, sizeof(peer->stable_id));
			}
			break;
		case PEER_EXPIRED:
			if (!patch) {
				rc = bool_value(value, &peer->expired);
			}
			break;
		case PEER_UNSIGNED:
			if (!patch) {
				rc = bool_value(value, &peer->unsigned_peer_api_only);
			}
			break;
		case PEER_ADDRESSES:
		case PEER_ALLOWED_IPS:
			if (!patch && json_kind(value) != '[') {
				return -EBADMSG;
			}
			break;
		default:
			break;
		}
		if (rc != 0) {
			return rc;
		}
	}

	if (patch) {
		return 0;
	}
	if (json_null(fields[PEER_KEY])) {
		return -EBADMSG;
	}

	int rc = prefix_array(fields[PEER_ADDRESSES], peer->addresses,
			      ARRAY_SIZE(peer->addresses), &peer->address_count);

	if (rc != 0) {
		return rc;
	}
	if (!json_null(fields[PEER_ALLOWED_IPS])) {
		rc = prefix_array(fields[PEER_ALLOWED_IPS], peer->allowed_ips,
				  ARRAY_SIZE(peer->allowed_ips), &peer->allowed_ip_count);
		if (rc != 0) {
			return rc;
		}
	}
	if (json_null(fields[PEER_ALLOWED_IPS]) || peer->unsigned_peer_api_only) {
		memset(peer->allowed_ips, 0, sizeof(peer->allowed_ips));
		memcpy(peer->allowed_ips, peer->addresses,
		       peer->address_count * sizeof(peer->addresses[0]));
		peer->allowed_ip_count = peer->address_count;
	}
	return 0;
}

#ifdef CONFIG_TAILSCALE_POLICY
static int dns_name_value(struct json_span token, char name[TAILSCALE_DNS_NAME_SIZE]);

static int32_t timestamp_decimal(const char *text, size_t len)
{
	int32_t value = 0;

	for (size_t i = 0; i < len; i++) {
		if (text[i] < '0' || text[i] > '9') {
			return -1;
		}
		value = value * 10 + (text[i] - '0');
	}
	return value;
}

/* The control protocol encodes Go time.Time with RFC3339 nanosecond precision. */
static int parse_timestamp(const char *text, int64_t *seconds, uint32_t *nanoseconds)
{
	uint32_t ns = 0U;
	int32_t offset = 0;

	if (strlen(text) < 20U || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
	    text[13] != ':' || text[16] != ':') {
		return -EBADMSG;
	}

	int32_t year = timestamp_decimal(text, 4U);
	int32_t month = timestamp_decimal(text + 5U, 2U);
	int32_t day = timestamp_decimal(text + 8U, 2U);
	int32_t hour = timestamp_decimal(text + 11U, 2U);
	int32_t minute = timestamp_decimal(text + 14U, 2U);
	int32_t second = timestamp_decimal(text + 17U, 2U);
	const uint8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);

	if (year < 0 || month < 1 || month > 12 || day < 1 ||
	    day > month_days[month - 1] + (month == 2 && leap) ||
	    hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
	    second < 0 || second > 59) {
		return -EBADMSG;
	}
	text += 19U;
	if (*text == '.') {
		const char *start = ++text;

		while (*text >= '0' && *text <= '9') {
			text++;
		}
		size_t digits = text - start;

		if (digits == 0U || digits > 9U) {
			return -EBADMSG;
		}
		ns = timestamp_decimal(start, digits);
		for (; digits < 9U; digits++) {
			ns *= 10U;
		}
	}
	if (*text != 'Z' || text[1] != '\0') {
		if ((*text != '+' && *text != '-') || strlen(text) != 6U || text[3] != ':') {
			return -EBADMSG;
		}
		int32_t hours = timestamp_decimal(text + 1U, 2U);
		int32_t minutes = timestamp_decimal(text + 4U, 2U);

		if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
			return -EBADMSG;
		}
		offset = (60 * hours + minutes) * 60;
		if (*text == '-') {
			offset = -offset;
		}
	}

	const struct tm date = {
		.tm_year = year - TIME_UTILS_BASE_YEAR,
		.tm_mon = month - 1,
		.tm_mday = day,
		.tm_hour = hour,
		.tm_min = minute,
		.tm_sec = second,
	};
	*seconds = timeutil_timegm64(&date) - offset;
	*nanoseconds = ns;
	return 0;
}

static int credential_expiry(struct tailscale_peer *peer)
{
	int64_t utc;
	uint32_t ns;
	int rc = parse_timestamp(peer->key_expiry, &utc, &ns);

	if (rc != 0) {
		return rc;
	}

	/* Go's zero time is 0001-01-01 at midnight UTC. */
	peer->key_expires = utc != GO_ZERO_UNIX_SECONDS || ns != 0U;
	peer->key_expiry_ms = peer->key_expires ? utc * 1000 + ns / 1000000U : 0;
	return 0;
}
#endif /* CONFIG_TAILSCALE_POLICY */

/* These attributes configure optional services, client preferences or the
 * administration UI. None grants an IP flow or enables a service here.
 * Active tailnet lock and signed peer keys are checked separately.
 */
static int host_capabilities(struct json_span map, struct tailscale_peer *peer)
{
	static const char *const names[] = {
		"default-auto-update",
		"https",
		"https://tailscale.com/cap/file-sharing",
		"https://tailscale.com/cap/is-admin",
		"https://tailscale.com/cap/is-owner",
		"https://tailscale.com/cap/ssh",
		"https://tailscale.com/cap/tailnet-lock",
		"probe-udp-lifetime",
		"ssh-behavior-v1",
		"ssh-env-vars",
		"store-appc-routes",
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
		"suggest-exit-node",
		"suggest-exit-node-ui",
#endif
		"tailnet-display-name",
		"tailnet.maxKeyDuration",
	};
	struct json_cursor cursor = json_elements(map);
	uint32_t seen = 0;

	#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
	peer->suggest_exit = false;
#else
	ARG_UNUSED(peer);
#endif
	BUILD_ASSERT(ARRAY_SIZE(names) <= 32);
	while (json_has_element(&cursor)) {
		struct json_span key = string_value(json_extract(&cursor));
		size_t i;

		for (i = 0; i < ARRAY_SIZE(names); ++i) {
			if ((size_t)(key.end - key.begin) == strlen(names[i]) &&
			    !memcmp(key.begin, names[i], strlen(names[i]))) {
				break;
			}
		}
		if (i == ARRAY_SIZE(names)) {
			return -ENOTSUP;
		}
		if (seen & BIT(i)) {
			return -EBADMSG;
		}
		seen |= BIT(i);
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
		if (!strcmp(names[i], "suggest-exit-node")) {
			peer->suggest_exit = true;
		}
#endif
		(void)json_pop(&cursor);
		struct json_span value = json_extract(&cursor);

		if (!json_null(value) && json_kind(value) != '[') {
			return -EBADMSG;
		}
		json_separator(&cursor);
	}
	return 0;
}

static int policy_peer_fields(struct tailscale_peer *peer,
			      const struct json_span fields[PEER_FIELD_COUNT], bool patch,
			      bool self)
{
	bool authorized = true;
	int rc;

	if ((!patch && (!json_null(fields[PEER_PATCH_ID]) ||
			!json_null(fields[PEER_PATCH_DERP]))) ||
	    (patch && (!json_null(fields[PEER_ID]) || !json_null(fields[PEER_ADDRESSES]) ||
		       !json_null(fields[PEER_ALLOWED_IPS]) || !json_null(fields[PEER_STABLE_ID]) ||
		       !json_null(fields[PEER_UNSIGNED]) || !json_null(fields[PEER_EXPIRED]) ||
		       !json_null(fields[PEER_DERP]) || !json_null(fields[PEER_LEGACY_DERP]) ||
		       !json_null(fields[PEER_AUTHORIZED])))) {
		return -ENOTSUP;
	}
	const enum peer_field unsupported_arrays[] = {
		PEER_CAPABILITIES,
#ifndef CONFIG_TAILSCALE_ROUTE_POLICY
		PEER_PRIMARY_ROUTES,
#endif
	};

	for (size_t i = 0; i < ARRAY_SIZE(unsupported_arrays); i++) {
		struct json_span field = fields[unsupported_arrays[i]];

		if (!json_null(field)) {
			if (json_kind(field) != '[') {
				return -EBADMSG;
			}
			if (nonempty_container(field)) {
				return -ENOTSUP;
			}
		}
	}
	if (!json_null(fields[PEER_CAP_MAP])) {
		if (json_kind(fields[PEER_CAP_MAP]) != '{') {
			return -EBADMSG;
		}
		rc = host_capabilities(fields[PEER_CAP_MAP], peer);
		if (rc != 0) {
			return rc;
		}
	}
	if (!json_null(fields[PEER_AUTHORIZED])) {
		rc = bool_value(fields[PEER_AUTHORIZED], &authorized);
		if (rc != 0) {
			return rc;
		}
	}
	rc = apply_peer_fields(peer, fields, patch);
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
	if (rc == 0 && !json_null(fields[PEER_PRIMARY_ROUTES])) {
		struct tailscale_peer_prefix prefixes[TAILSCALE_PEER_PREFIX_CAPACITY] = {0};
		uint8_t count = 0;

		BUILD_ASSERT(TAILSCALE_PEER_PREFIX_CAPACITY <= 8);
		rc = prefix_array(fields[PEER_PRIMARY_ROUTES], prefixes, ARRAY_SIZE(prefixes),
				  &count);
		peer->primary_routes = 0;
		for (size_t i = 0; rc == 0 && i < count; ++i) {
			size_t j;

			for (j = 0; j < peer->allowed_ip_count; ++j) {
				const struct tailscale_peer_prefix *p = &peer->allowed_ips[j];

				if (p->family == prefixes[i].family &&
				    p->length == prefixes[i].length &&
				    !memcmp(p->address, prefixes[i].address,
					    p->family == NET_AF_INET ? 4 : 16)) {
					break;
				}
			}
			if (j == peer->allowed_ip_count || (peer->primary_routes & BIT(j))) {
				return -EBADMSG;
			}
			peer->primary_routes |= BIT(j);
		}
	}
#endif
#ifdef CONFIG_TAILSCALE_POLICY
	if (rc == 0 && !json_null(fields[PEER_KEY_EXPIRY])) {
		rc = credential_expiry(peer);
	}
	if (rc == 0 && !json_null(fields[PEER_NAME])) {
		if (json_kind(fields[PEER_NAME]) == '"' &&
		    fields[PEER_NAME].end - fields[PEER_NAME].begin == 2) {
			peer->dns_name[0] = '\0';
		} else {
			rc = dns_name_value(fields[PEER_NAME], peer->dns_name);
		}
	}
#endif
	if (rc != 0) {
		return rc;
	}
	peer->expired |= !authorized;
	if (peer->key_signature_present || peer->unsigned_peer_api_only ||
	    (self && peer->expired)) {
		return -EKEYREJECTED;
	}
	if (!patch && peer->address_count == 0U) {
		return -EBADMSG;
	}
	for (size_t i = 0; i < peer->address_count; i++) {
		const struct tailscale_peer_prefix *prefix = &peer->addresses[i];

		if (prefix->length != (prefix->family == NET_AF_INET ? 32U : 128U)) {
			return -EBADMSG;
		}
	}
	if (!json_null(fields[PEER_LEGACY_DERP]) && peer->derp_region == 0U) {
		struct tailscale_peer_endpoint endpoint;

		rc = endpoint_value(fields[PEER_LEGACY_DERP], &endpoint);
		if (rc != 0 || endpoint.family != NET_AF_INET ||
		    memcmp(endpoint.address, (const uint8_t[]){127U, 3U, 3U, 40U}, 4U) != 0) {
			return -EBADMSG;
		}
		peer->derp_region = endpoint.port;
	}
	return 0;
}

static struct tailscale_peer *find_peer(struct tailscale_peer_snapshot *snapshot, uint64_t id)
{
	for (size_t i = 0; i < snapshot->count; i++) {
		if (snapshot->peers[i].id == id) {
			return &snapshot->peers[i];
		}
	}
	return NULL;
}

static int read_id(struct json_span token, uint64_t *id)
{
	int rc = uint_value(token, id);

	return rc == 0 && *id > 0 ? 0 : -EBADMSG;
}

struct peer_ids {
	uint64_t ids[TAILSCALE_PEER_INVENTORY_CAPACITY];
	size_t count;
};

static int remember_id(struct peer_ids *seen, uint64_t id)
{
	if (seen->count == ARRAY_SIZE(seen->ids)) {
		return -E2BIG;
	}
	for (size_t i = 0; i < seen->count; i++) {
		if (seen->ids[i] == id) {
			return -EBADMSG;
		}
	}
	seen->ids[seen->count++] = id;
	return 0;
}

static int apply_peer_array(struct tailscale_peer_snapshot *snapshot, struct json_span array,
			    bool patch, bool policy)
{
	struct json_cursor cursor = json_elements(array);
	struct peer_ids seen = {0};

	while (json_has_element(&cursor)) {
		struct tailscale_peer peer = {0};
		struct json_span fields[PEER_FIELD_COUNT] = {0};
		int rc = json_fields(json_extract(&cursor), peer_names, ARRAY_SIZE(peer_names),
				     fields, policy);

		if (rc == 0) {
			rc = read_id(fields[patch ? PEER_PATCH_ID : PEER_ID], &peer.id);
		}
		if (rc == 0) {
			rc = remember_id(&seen, peer.id);
		}
		if (rc != 0) {
			return rc;
		}
		struct tailscale_peer *existing = find_peer(snapshot, peer.id);

		if (patch && existing != NULL) {
			peer = *existing;
		}
		rc = policy ? policy_peer_fields(&peer, fields, patch, false) :
			      apply_peer_fields(&peer, fields, patch);
		if (rc != 0) {
			return rc;
		}
		if (existing != NULL) {
			*existing = peer;
		} else if (!patch) {
			if (snapshot->count == ARRAY_SIZE(snapshot->peers)) {
				return -E2BIG;
			}
			snapshot->peers[snapshot->count++] = peer;
		}
		json_separator(&cursor);
	}
	return 0;
}

static int remove_peer_array(struct tailscale_peer_snapshot *snapshot, struct json_span array)
{
	struct json_cursor cursor = json_elements(array);
	struct peer_ids seen = {0};

	while (json_has_element(&cursor)) {
		uint64_t id;
		int rc = read_id(json_extract(&cursor), &id);

		if (rc == 0) {
			rc = remember_id(&seen, id);
		}
		if (rc != 0) {
			return rc;
		}
		struct tailscale_peer *peer = find_peer(snapshot, id);

		if (peer != NULL) {
			*peer = snapshot->peers[--snapshot->count];
		}
		json_separator(&cursor);
	}
	return 0;
}

static int apply_online_changes(struct tailscale_peer_snapshot *snapshot, struct json_span object)
{
	struct json_cursor cursor = json_elements(object);
	struct peer_ids seen = {0};

	while (json_has_element(&cursor)) {
		uint64_t id;
		bool online;
		int rc = read_id(string_value(json_extract(&cursor)), &id);

		if (rc == 0) {
			rc = remember_id(&seen, id);
		}
		if (rc != 0) {
			return rc;
		}
		(void)json_pop(&cursor);
		rc = bool_value(json_extract(&cursor), &online);
		if (rc != 0) {
			return rc;
		}
		struct tailscale_peer *peer = find_peer(snapshot, id);

		if (peer != NULL) {
			peer->online = online ? TAILSCALE_PEER_ONLINE : TAILSCALE_PEER_OFFLINE;
		}
		json_separator(&cursor);
	}
	return 0;
}

enum response_field {
	RESPONSE_NODE,
	RESPONSE_KEEPALIVE,
	RESPONSE_SEQ,
	RESPONSE_PEERS,
	RESPONSE_CHANGED,
	RESPONSE_REMOVED,
	RESPONSE_PATCHED,
	RESPONSE_ONLINE,
	RESPONSE_FILTER,
	RESPONSE_NAMED_FILTERS,
	RESPONSE_TKA,
	RESPONSE_DNS,
	RESPONSE_SSH,
	RESPONSE_DEBUG,
	RESPONSE_POP_BROWSER_URL,
	RESPONSE_DERP,
	RESPONSE_SESSION_HANDLE,
	RESPONSE_CONTROL_TIME,
	RESPONSE_DOMAIN,
	RESPONSE_PEER_SEEN,
	RESPONSE_PING,
	RESPONSE_USER_PROFILES,
	RESPONSE_HEALTH,
	RESPONSE_DISPLAY_MESSAGES,
	RESPONSE_COLLECT_SERVICES,
	RESPONSE_DIAL_PLAN,
	RESPONSE_CLIENT_VERSION,
	RESPONSE_AUTO_UPDATE,
	RESPONSE_AUDIT_ID,
	RESPONSE_FIELD_COUNT,
};

static const char *const response_names[] = {
	[RESPONSE_NODE] = "Node",
	[RESPONSE_KEEPALIVE] = "KeepAlive",
	[RESPONSE_SEQ] = "Seq",
	[RESPONSE_PEERS] = "Peers",
	[RESPONSE_CHANGED] = "PeersChanged",
	[RESPONSE_REMOVED] = "PeersRemoved",
	[RESPONSE_PATCHED] = "PeersChangedPatch",
	[RESPONSE_ONLINE] = "OnlineChange",
	[RESPONSE_FILTER] = "PacketFilter",
	[RESPONSE_NAMED_FILTERS] = "PacketFilters",
	[RESPONSE_TKA] = "TKAInfo",
	[RESPONSE_DNS] = "DNSConfig",
	[RESPONSE_SSH] = "SSHPolicy",
	[RESPONSE_DEBUG] = "Debug",
	[RESPONSE_POP_BROWSER_URL] = "PopBrowserURL",
	[RESPONSE_DERP] = "DERPMap",
	[RESPONSE_SESSION_HANDLE] = "MapSessionHandle",
	[RESPONSE_CONTROL_TIME] = "ControlTime",
	[RESPONSE_DOMAIN] = "Domain",
	[RESPONSE_PEER_SEEN] = "PeerSeenChange",
	[RESPONSE_PING] = "PingRequest",
	[RESPONSE_USER_PROFILES] = "UserProfiles",
	[RESPONSE_HEALTH] = "Health",
	[RESPONSE_DISPLAY_MESSAGES] = "DisplayMessages",
	[RESPONSE_COLLECT_SERVICES] = "CollectServices",
	[RESPONSE_DIAL_PLAN] = "ControlDialPlan",
	[RESPONSE_CLIENT_VERSION] = "ClientVersion",
	[RESPONSE_AUTO_UPDATE] = "DefaultAutoUpdate",
	[RESPONSE_AUDIT_ID] = "DomainDataPlaneAuditLogID",
};

static int full_node_marker(struct json_span node)
{
	struct json_span fields[PEER_FIELD_COUNT] = {0};
	uint8_t key[TAILSCALE_PEER_PUBLIC_KEY_SIZE];
	uint64_t id;
	int rc = json_fields(node, peer_names, ARRAY_SIZE(peer_names), fields, false);

	if (rc == 0) {
		rc = read_id(fields[PEER_ID], &id);
	}
	if (rc == 0 && !json_null(fields[PEER_KEY])) {
		rc = key_value(fields[PEER_KEY], "nodekey:", key, false);
	}
	return rc;
}

static int track_sequence(uint64_t *last, struct json_span token, bool update)
{
	uint64_t sequence = 0;

	if (!json_null(token)) {
		int rc = uint_value(token, &sequence);

		if (rc != 0) {
			return rc;
		}
	}
	if (sequence == 0) {
		return update && *last != 0U ? -EILSEQ : 0;
	}
	if (*last != 0U && sequence != *last + 1U) {
		return -EILSEQ;
	}
	*last = sequence;
	return 0;
}

static int peer_frame_fields(const struct json_span fields[], bool nonempty[])
{
	for (enum response_field field = RESPONSE_PEERS; field <= RESPONSE_ONLINE; field++) {
		if (!json_null(fields[field])) {
			if (json_kind(fields[field]) != (field == RESPONSE_ONLINE ? '{' : '[')) {
				return -EBADMSG;
			}
			nonempty[field] = nonempty_container(fields[field]);
		}
	}
	return 0;
}

static int apply_peer_changes(struct tailscale_peer_snapshot *candidate,
			      const struct json_span fields[], bool have_peers, bool policy)
{
	int rc;

	if (have_peers) {
		return apply_peer_array(candidate, fields[RESPONSE_PEERS], false, policy);
	}
	rc = remove_peer_array(candidate, fields[RESPONSE_REMOVED]);
	if (rc == 0) {
		rc = apply_peer_array(candidate, fields[RESPONSE_CHANGED], false, policy);
	}
	if (rc == 0) {
		rc = apply_online_changes(candidate, fields[RESPONSE_ONLINE]);
	}
	if (rc == 0) {
		rc = apply_peer_array(candidate, fields[RESPONSE_PATCHED], true, policy);
	}
	return rc;
}

static int apply_response(struct tailscale_peer_inventory *inventory, struct json_span object,
			  enum tailscale_peer_inventory_frame *frame)
{
	struct json_span fields[RESPONSE_FIELD_COUNT] = {0};
	int rc = json_fields(object, response_names, ARRAY_SIZE(response_names), fields, false);
	bool keepalive = false;

	if (rc == 0 && !json_null(fields[RESPONSE_KEEPALIVE])) {
		rc = bool_value(fields[RESPONSE_KEEPALIVE], &keepalive);
	}
	if (rc != 0) {
		return rc;
	}
	if (keepalive) {
		*frame = TAILSCALE_PEER_INVENTORY_KEEPALIVE;
		return 0;
	}
	if (!json_null(fields[RESPONSE_NODE])) {
		rc = full_node_marker(fields[RESPONSE_NODE]);
		if (rc != 0) {
			return rc;
		}
	}

	bool nonempty[RESPONSE_FIELD_COUNT] = {0};

	rc = peer_frame_fields(fields, nonempty);
	if (rc != 0) {
		return rc;
	}
	bool full = nonempty[RESPONSE_PEERS] ||
		    (!inventory->summary.valid && !json_null(fields[RESPONSE_NODE]));
	bool delta = nonempty[RESPONSE_CHANGED] || nonempty[RESPONSE_REMOVED] ||
		     nonempty[RESPONSE_PATCHED] || nonempty[RESPONSE_ONLINE];

	rc = track_sequence(&inventory->sequence, fields[RESPONSE_SEQ], full || delta);
	if (rc != 0 || (!full && !delta)) {
		return rc;
	}
	if (!inventory->summary.valid && !full) {
		return -EBADMSG;
	}

	uint8_t next = inventory->published ^ 1;
	struct tailscale_peer_snapshot *candidate = &inventory->banks[next];

	if (full) {
		candidate->count = 0;
	} else {
		*candidate = inventory->banks[inventory->published];
	}
	rc = apply_peer_changes(candidate, fields, nonempty[RESPONSE_PEERS], false);
	if (rc == 0) {
		inventory->published = next;
		inventory->summary.peer_count = candidate->count;
		inventory->summary.valid = true;
		inventory->summary.revision++;
		*frame = full ? TAILSCALE_PEER_INVENTORY_FULL : TAILSCALE_PEER_INVENTORY_UPDATE;
	}
	return rc;
}

static int read_frame(uint8_t *json, size_t len, struct json_span *body)
{
	if (len > MAP_JSON_MAX_BYTES) {
		return -E2BIG;
	}
	if (json == NULL || len == 0U) {
		return -EBADMSG;
	}
	struct json_cursor cursor = {(const char *)json, (const char *)json + len};

	json_space(&cursor);
	body->begin = cursor.next;
	int rc = json_value(&cursor, 0);

	body->end = cursor.next;
	return rc == 0 && json_has_element(&cursor) ? -EBADMSG : rc;
}

#ifdef CONFIG_TAILSCALE_POLICY
static int dns_name(const char *text, size_t len, char name[TAILSCALE_DNS_NAME_SIZE])
{
	if (len == 0U) {
		return -EINVAL;
	}
	if (text[len - 1U] == '.') {
		len--;
	}
	if (len >= TAILSCALE_DNS_NAME_SIZE) {
		return -E2BIG;
	}
	for (size_t i = 0, label = 0; i < len; i++) {
		char ch = text[i];

		if (ch == '.') {
			if (label == 0U || i + 1U == len) {
				return -EINVAL;
			}
			label = 0;
		} else if (++label > 63U ||
			   !((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
			     (ch >= '0' && ch <= '9') || ch == '-' || ch == '_')) {
			return -EINVAL;
		}
		name[i] = (char)tolower((unsigned char)ch);
	}
	name[len] = '\0';
	return 0;
}

static int dns_name_value(struct json_span token, char name[TAILSCALE_DNS_NAME_SIZE])
{
	if (json_kind(token) != '"') {
		return -EBADMSG;
	}
	token = string_value(token);
	int rc = dns_name(token.begin, token.end - token.begin, name);

	return rc == -EINVAL ? -EBADMSG : rc;
}

static int dns_query_name(const char *text, char name[TAILSCALE_DNS_NAME_SIZE])
{
	size_t len = 0;

	while (len <= TAILSCALE_DNS_NAME_SIZE && text[len] != '\0') {
		len++;
	}
	return dns_name(text, len, name);
}

static int dns_empty_array(struct json_span token)
{
	if (json_null(token)) {
		return 0;
	}
	if (json_kind(token) != '[') {
		return -EBADMSG;
	}
	return nonempty_container(token) ? -ENOTSUP : 0;
}

static bool dns_ipv6_literal(const char *text)
{
	bool compressed = text[0] == ':' && text[1] == ':';
	size_t groups = 0;
	const char *group = compressed ? text + 2U : text;

	/* net_addr_pton() does not check the width or count of all IPv6 groups. */
	while (*group != '\0') {
		size_t width = strspn(group, "0123456789abcdefABCDEF");
		char separator = group[width];

		if (separator == '.') {
			struct net_in_addr embedded;

			if (net_addr_pton(NET_AF_INET, group, &embedded) != 0) {
				return false;
			}
			groups += 2U;
			break;
		}
		if (width == 0U || width > 4U) {
			return false;
		}
		groups++;
		if (separator == '\0') {
			break;
		}
		if (separator != ':') {
			return false;
		}
		group += width + 1U;
		if (*group == ':') {
			if (compressed) {
				return false;
			}
			compressed = true;
			group++;
		} else if (*group == '\0') {
			return false;
		}
	}
	return compressed ? groups < 8U : groups == 8U;
}

static int dns_server(struct json_span object, struct tailscale_dns_server *server)
{
	static const char *const names[] = {"Addr", "BootstrapResolution", "UseWithExitNode"};
	struct json_span fields[ARRAY_SIZE(names)] = {0};
	char text[NET_INET6_ADDRSTRLEN + 8];
	bool use_with_exit_node = false;
	int rc = json_fields(object, names, ARRAY_SIZE(names), fields, true);

	if (rc == 0) {
		rc = dns_empty_array(fields[1]);
	}
	if (rc == 0 && !json_null(fields[2])) {
		rc = bool_value(fields[2], &use_with_exit_node);
	}
	if (rc != 0 || use_with_exit_node) {
		return rc != 0 ? rc : -ENOTSUP;
	}
	rc = text_value(fields[0], text, sizeof(text));
	if (rc != 0) {
		return rc;
	}
	if (strstr(text, "://") != NULL || strchr(text, '%') != NULL) {
		return -ENOTSUP;
	}
	if (text[0] == '[') {
		char *end = strchr(text, ']');

		if (end == NULL || end[1] != ':') {
			return -EBADMSG;
		}
		*end = '\0';
		if (!dns_ipv6_literal(text + 1U)) {
			return -EBADMSG;
		}
	} else {
		bool ipv6 = strchr(text, ':') != NULL && strchr(text, ':') != strrchr(text, ':');

		server->family = ipv6 ? NET_AF_INET6 : NET_AF_INET;
		if (ipv6 && !dns_ipv6_literal(text)) {
			return -EBADMSG;
		}
		if (net_addr_pton(server->family, text, server->address) == 0) {
			server->port = 53U;
			return 0;
		}
	}

	struct tailscale_peer_endpoint endpoint = {0};

	rc = endpoint_value(fields[0], &endpoint);
	if (rc == 0) {
		memcpy(server->address, endpoint.address, sizeof(server->address));
		server->family = endpoint.family;
		server->port = endpoint.port;
	}
	return rc;
}

static int dns_servers(struct json_span array, struct tailscale_dns_servers *servers)
{
	*servers = (struct tailscale_dns_servers){0};
	if (json_null(array)) {
		return 0;
	}
	if (json_kind(array) != '[') {
		return -EBADMSG;
	}

	struct json_cursor cursor = json_elements(array);

	while (json_has_element(&cursor)) {
		if (servers->count == ARRAY_SIZE(servers->entries)) {
			return -E2BIG;
		}
		int rc = dns_server(json_extract(&cursor), &servers->entries[servers->count]);

		if (rc != 0) {
			return rc;
		}
		servers->count++;
		json_separator(&cursor);
	}
	return 0;
}

static int dns_routes(struct json_span object, struct tailscale_dns_config *dns)
{
	dns->route_count = 0;
	if (json_null(object)) {
		return 0;
	}
	if (json_kind(object) != '{') {
		return -EBADMSG;
	}

	struct json_cursor cursor = json_elements(object);

	while (json_has_element(&cursor)) {
		if (dns->route_count == ARRAY_SIZE(dns->routes)) {
			return -E2BIG;
		}
		struct tailscale_dns_route *route = &dns->routes[dns->route_count];
		int rc = dns_name_value(json_extract(&cursor), route->suffix);

		if (rc != 0) {
			return rc;
		}
		for (size_t i = 0; i < dns->route_count; i++) {
			if (strcmp(dns->routes[i].suffix, route->suffix) == 0) {
				return -EBADMSG;
			}
		}
		(void)json_pop(&cursor);
		rc = dns_servers(json_extract(&cursor), &route->servers);
		if (rc != 0) {
			return rc;
		}
		dns->route_count++;
		json_separator(&cursor);
	}
	return 0;
}

static int dns_domains(struct json_span array, struct tailscale_dns_config *dns, bool search,
		       bool exit_suffix)
{
	if (json_null(array)) {
		return 0;
	}
	if (json_kind(array) != '[') {
		return -EBADMSG;
	}
	struct json_cursor cursor = json_elements(array);

	while (json_has_element(&cursor)) {
		struct json_span token = json_extract(&cursor);
		char name[TAILSCALE_DNS_NAME_SIZE];
		int rc;

		/* A leading dot on an exit DNS exclusion selects subdomains. */
		if (exit_suffix && json_kind(token) == '"' && token.end - token.begin > 2 &&
		    token.begin[1] == '.') {
			struct json_span suffix = {token.begin + 2, token.end - 1};

			rc = dns_name(suffix.begin, suffix.end - suffix.begin, name);
		} else {
			rc = dns_name_value(token, name);
		}
		if (rc != 0 || name[0] == '\0') {
			return rc != 0 ? rc : -EBADMSG;
		}
		if (search) {
			if (dns->search_count == ARRAY_SIZE(dns->search)) {
				return -E2BIG;
			}
			strcpy(dns->search[dns->search_count++], name);
		}
		json_separator(&cursor);
	}
	return 0;
}

static int compile_dns(struct json_span object, struct tailscale_dns_config *dns)
{
	static const char *const names[] = {
		"Resolvers",          "Routes",       "Proxied",
		"FallbackResolvers",  "Domains",      "Nameservers",
		"CertDomains",        "ExtraRecords", "ExitNodeFilteredSet",
		"TempCorpIssue13969",
	};
	struct json_span fields[ARRAY_SIZE(names)] = {0};
	int rc = json_fields(object, names, ARRAY_SIZE(names), fields, true);

	if (rc != 0) {
		return rc;
	}
	dns->search_count = 0;
	for (size_t i = 3; i < ARRAY_SIZE(names) - 1U; i++) {
		if (i == 4U || i == 6U || i == 8U) {
			rc = dns_domains(fields[i], dns, i == 4U, i == 8U);
		} else {
			rc = dns_empty_array(fields[i]);
		}
		if (rc != 0) {
			return rc;
		}
	}
	if (!json_null(fields[9])) {
		if (json_kind(fields[9]) != '"') {
			return -EBADMSG;
		}
		if (fields[9].end - fields[9].begin != 2) {
			return -ENOTSUP;
		}
	}
	dns->proxied = false;
	if (!json_null(fields[2])) {
		rc = bool_value(fields[2], &dns->proxied);
	}
	if (rc == 0) {
		rc = dns_servers(fields[0], &dns->defaults);
	}
	return rc == 0 ? dns_routes(fields[1], dns) : rc;
}

static bool dns_known_name(const struct tailscale_peer_snapshot *peers, const char *name)
{
	if (name[0] == '\0') {
		return false;
	}
	if (strcmp(peers->self.dns_name, name) == 0) {
		return true;
	}
	for (size_t i = 0; i < peers->count; i++) {
		if (strcmp(peers->peers[i].dns_name, name) == 0) {
			return true;
		}
	}
	return false;
}

static bool dns_suffix_match(const char *name, size_t len, const char *suffix, size_t suffix_len)
{
	return suffix_len == 0U ||
	       (len >= suffix_len && (len == suffix_len || name[len - suffix_len - 1U] == '.') &&
		strcmp(name + len - suffix_len, suffix) == 0);
}

static size_t magic_dns_match(const char *name, size_t len, const char *magic_suffix)
{
	static const char ipv6[] = "0.e.1.a.c.5.1.1.a.7.d.f.ip6.arpa";
	static const char ipv4[] = "100.in-addr.arpa";
	size_t suffix_len = strlen(magic_suffix);
	size_t match = dns_suffix_match(name, len, magic_suffix, suffix_len) ? suffix_len : 0U;

	if (dns_suffix_match(name, len, ipv6, sizeof(ipv6) - 1U)) {
		match = MAX(match, sizeof(ipv6) - 1U);
	}
	if (len <= sizeof(ipv4) || !dns_suffix_match(name, len, ipv4, sizeof(ipv4) - 1U)) {
		return match;
	}
	size_t end = len - sizeof(ipv4);
	size_t start = end;

	while (start > 0U && name[start - 1U] != '.') {
		start--;
	}
	uint64_t second_octet;

	if (uint_value((struct json_span){name + start, name + end}, &second_octet) == 0 &&
	    second_octet >= 64U && second_octet <= 127U) {
		match = MAX(match, len - start);
	}
	return match;
}

static void dns_select(const struct tailscale_dns_config *dns,
		       const struct tailscale_peer_snapshot *peers, const char *name,
		       struct tailscale_dns_selection *selection)
{
	const char *suffix = strchr(peers->self.dns_name, '.');
	const struct tailscale_dns_servers *servers = &dns->defaults;
	size_t len = strlen(name);
	size_t longest = dns->proxied ? magic_dns_match(name, len, suffix + 1U) : 0U;

	if (dns->proxied && dns_known_name(peers, name)) {
		selection->source = TAILSCALE_DNS_LOCAL;
		return;
	}

	if (longest != 0U) {
		selection->source = TAILSCALE_DNS_LOCAL;
		servers = NULL;
	} else {
		selection->source = servers->count == 0U ? TAILSCALE_DNS_NONE :
							   TAILSCALE_DNS_DEFAULT;
	}
	for (size_t i = 0; i < dns->route_count; i++) {
		const struct tailscale_dns_route *route = &dns->routes[i];
		size_t suffix_len = strlen(route->suffix);

		if (suffix_len >= longest &&
		    dns_suffix_match(name, len, route->suffix, suffix_len)) {
			longest = suffix_len;
			servers = &route->servers;
			selection->source = servers->count == 0U ? TAILSCALE_DNS_LOCAL :
								   TAILSCALE_DNS_SPLIT;
		}
	}
	if (servers != NULL) {
		selection->server_count = servers->count;
		memcpy(selection->servers, servers->entries,
		       servers->count * sizeof(servers->entries[0]));
	}
}

static bool dns_peer_valid(const struct tailscale_peer_snapshot *peers,
			   const struct tailscale_peer *peer, int64_t now)
{
	return !peer->expired &&
	       (!peer->key_expires ||
		(peers->have_control_time &&
		 now < peer->key_expiry_ms - peers->control_epoch_at_boot_ms));
}

static int dns_resolve_magic(const struct tailscale_peer_snapshot *peers, const char *name,
			     uint8_t family, struct tailscale_dns_answer *answer)
{
	const struct tailscale_peer *match = NULL;
	int64_t now = k_uptime_get();

	if (!peers->have_self || !dns_peer_valid(peers, &peers->self, now)) {
		return -EKEYREJECTED;
	}
	if (name[0] == '\0') {
		return -ENOENT;
	}
	if (strcmp(peers->self.dns_name, name) == 0) {
		match = &peers->self;
	}
	for (size_t i = 0; i < peers->count; i++) {
		const struct tailscale_peer *peer = &peers->peers[i];

		if (strcmp(peer->dns_name, name) == 0 && dns_peer_valid(peers, peer, now)) {
			if (match != NULL) {
				return -EADDRINUSE;
			}
			match = peer;
		}
	}
	if (match == NULL) {
		return -ENOENT;
	}
	answer->self = match == &peers->self;
	answer->node_id = match->id;
	for (size_t i = 0; i < match->address_count; i++) {
		const struct tailscale_peer_prefix *prefix = &match->addresses[i];

		if (prefix->family == family) {
			answer->addresses[answer->address_count++] = *prefix;
		}
	}
	return answer->address_count != 0U ? 0 : -ENOENT;
}

static int filter_address(struct json_span token, struct tailscale_filter_address *range)
{
	char text[2 * NET_INET6_ADDRSTRLEN];
	struct tailscale_peer_prefix prefix = {0};
	int rc = text_value(token, text, sizeof(text));

	if (rc != 0) {
		return rc;
	}
	if (strcmp(text, "*") == 0) {
		range->family = NET_AF_UNSPEC;
		return 0;
	}
	if (strncmp(text, "cap:", 4U) == 0) {
		return -ENOTSUP;
	}
	char *end = strchr(text, '-');

	if (end != NULL) {
		*end++ = '\0';
		range->family = strchr(text, ':') == NULL ? NET_AF_INET : NET_AF_INET6;
		size_t len = range->family == NET_AF_INET ? 4U : 16U;

		if ((range->family == NET_AF_INET6 &&
		     (!dns_ipv6_literal(text) || !dns_ipv6_literal(end))) ||
		    net_addr_pton(range->family, text, range->first) != 0 ||
		    net_addr_pton(range->family, end, range->last) != 0 ||
		    memcmp(range->first, range->last, len) > 0) {
			return -EBADMSG;
		}
		return 0;
	}
	rc = parse_prefix(text, &prefix, true);
	if (rc != 0) {
		return rc;
	}
	if (prefix.family == NET_AF_INET6 && !dns_ipv6_literal(text)) {
		return -EBADMSG;
	}
	range->family = prefix.family;
	memcpy(range->first, prefix.address, sizeof(range->first));
	for (size_t i = 0; i < sizeof(range->last); ++i) {
		unsigned int bits = MIN(prefix.length, 8U);

		range->last[i] = prefix.address[i] | (uint8_t)(UINT8_MAX >> bits);
		prefix.length -= bits;
	}
	return 0;
}

static int filter_destination(struct json_span object, struct tailscale_packet_filter_dst *dst)
{
	static const char *const dst_names[] = {"IP", "Ports"};
	static const char *const port_names[] = {"First", "Last"};
	struct json_span dst_fields[ARRAY_SIZE(dst_names)] = {0};
	struct json_span port_fields[ARRAY_SIZE(port_names)] = {0};
	uint64_t first;
	uint64_t last;
	int rc = json_fields(object, dst_names, ARRAY_SIZE(dst_names), dst_fields, true);

	if (rc == 0) {
		rc = filter_address(dst_fields[0], &dst->address);
	}
	if (rc == 0) {
		rc = json_fields(dst_fields[1], port_names, ARRAY_SIZE(port_names),
				 port_fields, true);
	}
	if (rc != 0) {
		return rc;
	}
	if (uint_value(port_fields[0], &first) != 0 || uint_value(port_fields[1], &last) != 0 ||
	    last > UINT16_MAX || first > last) {
		return -EBADMSG;
	}
	dst->first_port = first;
	dst->last_port = last;
	return 0;
}

static int filter_protocols(struct json_span array, uint8_t *protocols)
{
	struct json_cursor cursor = json_elements(array);

	/* Intersect the control default (TCP, UDP and ICMP) with the
	 * transport protocols this host policy can admit.
	 */
	if (json_null(array) || (json_kind(array) == '[' && !nonempty_container(array))) {
		*protocols = TAILSCALE_FILTER_TCP | TAILSCALE_FILTER_UDP;
		return 0;
	}
	if (json_kind(array) != '[') {
		return -EBADMSG;
	}
	while (json_has_element(&cursor)) {
		uint64_t protocol;
		int rc = uint_value(json_extract(&cursor), &protocol);

		if (rc != 0) {
			return rc;
		}
		if (protocol == NET_IPPROTO_TCP) {
			*protocols |= TAILSCALE_FILTER_TCP;
		} else if (protocol == NET_IPPROTO_UDP) {
			*protocols |= TAILSCALE_FILTER_UDP;
		} else {
			return -ENOTSUP;
		}
		json_separator(&cursor);
	}
	return *protocols == 0U ? -ENOTSUP : 0;
}

static int filter_rule(struct json_span object, struct tailscale_packet_filter_rule *rule)
{
	static const char *const names[] = {"SrcIPs", "DstPorts", "IPProto"};
	struct json_span fields[ARRAY_SIZE(names)] = {0};
	int rc = json_fields(object, names, ARRAY_SIZE(names), fields, true);

	if (rc != 0) {
		return rc;
	}
	if (json_kind(fields[0]) != '[' || json_kind(fields[1]) != '[') {
		return -EBADMSG;
	}
	*rule = (struct tailscale_packet_filter_rule){0};
	rc = filter_protocols(fields[2], &rule->protocols);
	if (rc != 0) {
		return rc;
	}
	struct json_cursor sources = json_elements(fields[0]);
	struct json_cursor destinations = json_elements(fields[1]);

	while (json_has_element(&sources)) {
		if (rule->source_count == ARRAY_SIZE(rule->sources)) {
			return -E2BIG;
		}
		rc = filter_address(json_extract(&sources), &rule->sources[rule->source_count]);
		if (rc != 0) {
			return rc;
		}
		rule->source_count++;
		json_separator(&sources);
	}
	while (json_has_element(&destinations)) {
		if (rule->destination_count == ARRAY_SIZE(rule->destinations)) {
			return -E2BIG;
		}
		rc = filter_destination(json_extract(&destinations),
					&rule->destinations[rule->destination_count]);
		if (rc != 0) {
			return rc;
		}
		rule->destination_count++;
		json_separator(&destinations);
	}
	return 0;
}

static int append_filter_rules(struct json_span array, struct tailscale_packet_filter *filter)
{
	struct json_cursor cursor = json_elements(array);

	while (json_has_element(&cursor)) {
		if (filter->count == ARRAY_SIZE(filter->rules)) {
			return -E2BIG;
		}
		int rc = filter_rule(json_extract(&cursor), &filter->rules[filter->count]);

		if (rc != 0) {
			return rc;
		}
		filter->count++;
		json_separator(&cursor);
	}
	return 0;
}

#define FILTER_MAX_CHANGES (2U * TAILSCALE_PACKET_FILTER_MAX_CHUNKS)

struct filter_change {
	struct tailscale_packet_filter_name name;
	struct json_span rules;
};

static int filter_name_codepoint(const char **next, const char *end, uint32_t *codepoint)
{
	static const char escapes[] = "\"\\/bfnrt";
	static const char values[] = "\"\\/\b\f\n\r\t";
	char escape = *(*next)++;
	uint8_t code[2];

	/* The JSON parser already validates the escape spelling and hex digits. */
	if (escape != 'u') {
		*codepoint = values[strchr(escapes, escape) - escapes];
		return 0;
	}
	if (hex2bin(*next, 4U, code, sizeof(code)) != sizeof(code)) {
		return -EBADMSG;
	}
	*next += 4U;
	*codepoint = ((uint32_t)code[0] << 8) | code[1];
	if (*codepoint < 0xd800U || *codepoint > 0xdfffU) {
		return 0;
	}
	if (*codepoint > 0xdbffU || end - *next < 6 || memcmp(*next, "\\u", 2U) != 0 ||
	    hex2bin(*next + 2, 4U, code, sizeof(code)) != sizeof(code)) {
		return -EBADMSG;
	}
	uint16_t trail = ((uint16_t)code[0] << 8) | code[1];

	if (trail < 0xdc00U || trail > 0xdfffU) {
		return -EBADMSG;
	}
	*next += 6U;
	*codepoint = 0x10000U + ((*codepoint - 0xd800U) << 10) + (trail - 0xdc00U);
	return 0;
}

static int filter_name(struct json_span key, struct tailscale_packet_filter_name *name)
{
	static const uint8_t lead[] = {0U, 0xc0U, 0xe0U, 0xf0U};

	for (const char *p = key.begin; p != key.end;) {
		uint8_t bytes[4] = {(uint8_t)*p};
		size_t length = 1U;

		p++;
		if (bytes[0] == '\\') {
			uint32_t cp;
			int rc = filter_name_codepoint(&p, key.end, &cp);

			if (rc != 0) {
				return rc;
			}
			if (cp >= 0x80U) {
				length = cp < 0x800U ? 2U : cp < 0x10000U ? 3U : 4U;
				for (size_t i = length - 1U; i > 0U; i--) {
					bytes[i] = 0x80U | (cp & 0x3fU);
					cp >>= 6;
				}
			}
			bytes[0] = lead[length - 1U] | cp;
		}
		if (length > sizeof(name->bytes) - name->length) {
			return -E2BIG;
		}
		memcpy(&name->bytes[name->length], bytes, length);
		name->length += length;
	}
	return utf8_is_valid(name->bytes, name->length) ? 0 : -EBADMSG;
}

static int filter_name_compare(const struct tailscale_packet_filter_name *a,
			       const struct tailscale_packet_filter_name *b)
{
	int rc = memcmp(a->bytes, b->bytes, MIN(a->length, b->length));

	return rc != 0 ? rc : (int)a->length - (int)b->length;
}

static int insert_filter_change(struct filter_change changes[], size_t *count,
				const struct filter_change *change, bool allow_existing)
{
	size_t i = 0U;

	for (; i < *count; i++) {
		int order = filter_name_compare(&change->name, &changes[i].name);

		if (order == 0) {
			return allow_existing ? 0 : -EBADMSG;
		}
		if (order < 0) {
			break;
		}
	}
	memmove(&changes[i + 1U], &changes[i], (*count - i) * sizeof(*changes));
	changes[i] = *change;
	(*count)++;
	return 0;
}

static int read_filter_changes(struct json_span named, struct filter_change changes[],
			       size_t *count, bool *reset)
{
	if (json_null(named)) {
		return 0;
	}
	if (json_kind(named) != '{') {
		return -EBADMSG;
	}
	struct json_cursor cursor = json_elements(named);
	bool have_star = false;

	while (json_has_element(&cursor)) {
		struct filter_change change = {0};
		int rc = filter_name(string_value(json_extract(&cursor)), &change.name);

		if (rc != 0) {
			return rc;
		}
		(void)json_pop(&cursor);
		change.rules = json_extract(&cursor);
		if (!json_null(change.rules) && json_kind(change.rules) != '[') {
			return -EBADMSG;
		}
		if (change.name.length == 1U && change.name.bytes[0] == '*') {
			if (have_star) {
				return -EBADMSG;
			}
			have_star = true;
			*reset = json_null(change.rules);
		} else {
			if (*count == FILTER_MAX_CHANGES) {
				return -E2BIG;
			}
			rc = insert_filter_change(changes, count, &change, false);
			if (rc != 0) {
				return rc;
			}
		}
		json_separator(&cursor);
	}
	return 0;
}

static int stage_filter(struct tailscale_policy *policy, uint8_t next, bool current,
			struct json_span legacy, struct json_span named)
{
	struct filter_change changes[FILTER_MAX_CHANGES + 1U];
	size_t changed = 0U;
	bool reset = false;
	int rc = read_filter_changes(named, changes, &changed, &reset);

	if (rc != 0) {
		return rc;
	}
	if (!json_null(legacy)) {
		if (json_kind(legacy) != '[') {
			return -EBADMSG;
		}
		if (!reset) {
			const struct filter_change base = {
				.name = {.bytes = "base", .length = 4U},
				.rules = legacy,
			};

			/* Retain a named base entry when it supersedes the legacy value. */
			(void)insert_filter_change(changes, &changed, &base, true);
		}
	}

	const struct tailscale_packet_filter_directory *old =
		&policy->filter_directories[policy->published];
	const struct tailscale_packet_filter *published = &policy->filters[policy->published];
	struct tailscale_packet_filter_directory *directory = &policy->filter_directories[next];
	struct tailscale_packet_filter *filter = &policy->filters[next];
	size_t retained = current && !reset ? old->count : 0U;
	size_t first = 0U;

	directory->count = 0U;
	filter->count = 0U;
	/* Merge the two name lists into the inactive rule bank. */
	for (size_t i = 0U, j = 0U; i < retained || j < changed;) {
		const struct tailscale_packet_filter_chunk *chunk =
			i < retained ? &old->chunks[i] : NULL;
		const struct filter_change *change = j < changed ? &changes[j] : NULL;
		int order = chunk == NULL ? 1 : change == NULL ? -1 :
			    filter_name_compare(&chunk->name, &change->name);
		size_t previous_first = first;
		uint8_t start = filter->count;
		const struct tailscale_packet_filter_name *name;

		if (order <= 0) {
			first += chunk->rule_count;
			i++;
		}
		if (order >= 0) {
			j++;
			if (json_null(change->rules)) {
				continue;
			}
			rc = append_filter_rules(change->rules, filter);
			if (rc != 0) {
				return rc;
			}
			name = &change->name;
		} else {
			if (chunk->rule_count > ARRAY_SIZE(filter->rules) - filter->count) {
				return -E2BIG;
			}
			memcpy(&filter->rules[start], &published->rules[previous_first],
			       chunk->rule_count * sizeof(filter->rules[0]));
			filter->count += chunk->rule_count;
			name = &chunk->name;
		}
		if (directory->count == ARRAY_SIZE(directory->chunks)) {
			return -E2BIG;
		}
		directory->chunks[directory->count++] = (struct tailscale_packet_filter_chunk){
			.name = *name, .rule_count = filter->count - start,
		};
	}
	return 0;
}

static int policy_self(struct tailscale_peer_snapshot *snapshot, struct json_span node)
{
	struct json_span fields[PEER_FIELD_COUNT] = {0};
	int rc = json_fields(node, peer_names, ARRAY_SIZE(peer_names), fields, true);

	memset(&snapshot->self, 0, sizeof(snapshot->self));
	if (rc == 0) {
		rc = read_id(fields[PEER_ID], &snapshot->self.id);
	}
	if (rc == 0) {
		rc = policy_peer_fields(&snapshot->self, fields, false, true);
	}
	if (rc == 0) {
		snapshot->have_self = true;
	}
	return rc;
}

static bool same_prefix(const struct tailscale_peer_prefix *a,
			const struct tailscale_peer_prefix *b)
{
	return a->family == b->family && a->length == b->length &&
	       memcmp(a->address, b->address, sizeof(a->address)) == 0;
}

static bool prefix_conflict(const struct tailscale_peer_prefix *a, size_t a_count,
			    const struct tailscale_peer_prefix *b, size_t b_count)
{
	for (size_t i = 0; i < a_count; i++) {
		for (size_t j = a == b ? i + 1U : 0U; j < b_count; j++) {
			if (same_prefix(&a[i], &b[j])) {
				return true;
			}
		}
	}
	return false;
}

static bool peer_conflict(const struct tailscale_peer *a, const struct tailscale_peer *b)
{
	return a->id == b->id || memcmp(a->key, b->key, sizeof(a->key)) == 0 ||
	       (a->stable_id[0] && strcmp(a->stable_id, b->stable_id) == 0) ||
	       prefix_conflict(a->addresses, a->address_count,
			       b->addresses, b->address_count) ||
	       prefix_conflict(a->addresses, a->address_count,
			       b->allowed_ips, b->allowed_ip_count) ||
	       prefix_conflict(a->allowed_ips, a->allowed_ip_count,
			       b->addresses, b->address_count) ||
	       (!IS_ENABLED(CONFIG_TAILSCALE_ROUTE_POLICY) &&
		prefix_conflict(a->allowed_ips, a->allowed_ip_count,
				b->allowed_ips, b->allowed_ip_count));
}

static bool peer_prefix_duplicates(const struct tailscale_peer *peer)
{
	return prefix_conflict(peer->addresses, peer->address_count,
			       peer->addresses, peer->address_count) ||
	       prefix_conflict(peer->allowed_ips, peer->allowed_ip_count,
			       peer->allowed_ips, peer->allowed_ip_count);
}

static int policy_validate_peers(const struct tailscale_peer_snapshot *snapshot)
{
	if (snapshot->have_self && peer_prefix_duplicates(&snapshot->self)) {
		return -EBADMSG;
	}
	for (size_t i = 0; i < snapshot->count; i++) {
		const struct tailscale_peer *peer = &snapshot->peers[i];

		if (peer_prefix_duplicates(peer) ||
		    (snapshot->have_self && peer_conflict(peer, &snapshot->self))) {
			return -EBADMSG;
		}
		for (size_t j = 0; j < i; j++) {
			if (peer_conflict(peer, &snapshot->peers[j])) {
				return -EBADMSG;
			}
		}
	}
	return 0;
}

#ifdef CONFIG_TAILSCALE_DERP_MAP
static int derp_ip(struct json_span span, int family, char *out, size_t size)
{
	uint8_t addr[16];
	int rc;

	if (json_null(span)) {
		return 0;
	}
	rc = text_value(span, out, size);
	if (rc == 0 && out[0] != '\0' && strcmp(out, "none") != 0 &&
	    ((family == NET_AF_INET6 && !dns_ipv6_literal(out)) ||
	     net_addr_pton(family, out, addr) != 0)) {
		return -EBADMSG;
	}
	return rc;
}

static int derp_node(struct tailscale_derp_map *map, struct json_span span, uint16_t region,
		     bool no_home)
{
	enum { ID, HOST, CERT, V4, V6, PORT, STUN_ONLY, INSECURE, STUN_PORT };
	static const char *const names[] = {
		"RegionID", "HostName", "CertName", "IPv4", "IPv6", "DERPPort", "STUNOnly",
		"InsecureForTests", "STUNPort",
	};
	struct json_span f[ARRAY_SIZE(names)] = {0};
	uint64_t id, port = 443, stun_port = 3478;
	bool stun_only = false;
	bool insecure = false;
	int rc = json_fields(span, names, ARRAY_SIZE(names), f, false);

	if (rc != 0 || uint_value(f[ID], &id) != 0 || id != region ||
	    (!json_null(f[STUN_ONLY]) && bool_value(f[STUN_ONLY], &stun_only) != 0) ||
	    (!json_null(f[INSECURE]) && bool_value(f[INSECURE], &insecure) != 0)) {
		return -EBADMSG;
	}
	if (stun_only) {
		return 0;
	}
	if (insecure) {
		return -ENOTSUP;
	}
	if (!json_null(f[PORT]) && (uint_value(f[PORT], &port) != 0 || port > UINT16_MAX)) {
		return -EBADMSG;
	}
	if (!json_null(f[STUN_PORT])) {
		if (f[STUN_PORT].end - f[STUN_PORT].begin == 2 &&
		    memcmp(f[STUN_PORT].begin, "-1", 2) == 0) {
			stun_port = 0;
		} else if (uint_value(f[STUN_PORT], &stun_port) != 0 || stun_port > UINT16_MAX) {
			return -EBADMSG;
		} else if (stun_port == 0) {
			stun_port = 3478;
		}
	}
	if (map->count == ARRAY_SIZE(map->nodes)) {
		return -ENOSPC;
	}
	struct tailscale_derp_node *node = &map->nodes[map->count];

	*node = (struct tailscale_derp_node){
		.region = region, .port = port == 0 ? 443 : port, .no_home = no_home,
		.stun_port = stun_port,
	};
	rc = dns_name_value(f[HOST], node->host);
	if (rc == 0 && node->host[0] == '\0') {
		return -EBADMSG;
	}
	if (rc == 0 && !json_null(f[CERT])) {
		rc = text_value(f[CERT], node->cert_name, sizeof(node->cert_name));
		if (rc == 0 && node->cert_name[0] != '\0') {
			if (strncmp(node->cert_name, "sha256-raw:", 11) == 0) {
				uint8_t hash[32];

				if (strlen(node->cert_name + 11) != 64 ||
				    hex2bin(node->cert_name + 11, 64, hash, 32) != 32) {
					return -EBADMSG;
				}
			} else {
				rc = dns_name_value(f[CERT], node->cert_name);
			}
		}
	}
	if (rc == 0) {
		rc = derp_ip(f[V4], NET_AF_INET, node->ipv4, sizeof(node->ipv4));
	}
	if (rc == 0) {
		rc = derp_ip(f[V6], NET_AF_INET6, node->ipv6, sizeof(node->ipv6));
	}
	if (rc == 0) {
		map->count++;
	}
	return rc;
}

static int derp_region(struct tailscale_derp_map *map, struct json_span span, uint16_t region)
{
	static const char *const names[] = {"RegionID", "Nodes", "NoMeasureNoHome", "Avoid"};
	struct json_span f[ARRAY_SIZE(names)] = {0};
	bool no_home = false, avoid = false;
	uint64_t id;
	int rc = json_fields(span, names, ARRAY_SIZE(names), f, false);

	if (rc != 0 || uint_value(f[0], &id) != 0 || id != region || json_kind(f[1]) != '[' ||
	    (!json_null(f[2]) && bool_value(f[2], &no_home) != 0) ||
	    (!json_null(f[3]) && bool_value(f[3], &avoid) != 0)) {
		return -EBADMSG;
	}
	struct json_cursor nodes = json_elements(f[1]);

	while (json_has_element(&nodes)) {
		rc = derp_node(map, json_extract(&nodes), region, no_home || avoid);
		if (rc != 0) {
			return rc;
		}
		json_separator(&nodes);
	}
	return 0;
}

static int compile_derp_map(struct json_span span, struct tailscale_derp_map *map)
{
	static const char *const names[] = {
		"Regions", "HomeParams", "omitDefaultRegions", "OmitDefaultRegions",
	};
	struct json_span f[ARRAY_SIZE(names)] = {0};
	uint16_t seen[CONFIG_TAILSCALE_DERP_MAX_NODES];
	size_t count = 0;
	bool omit;
	int rc = json_fields(span, names, ARRAY_SIZE(names), f, true);

	if (rc != 0) {
		return rc;
	}
	/* HomeParams contains latency weights. No active latency selection is performed. */
	if ((!json_null(f[1]) && json_kind(f[1]) != '{') ||
	    (!json_null(f[2]) && bool_value(f[2], &omit) != 0) ||
	    (!json_null(f[3]) && bool_value(f[3], &omit) != 0) ||
	    (!json_null(f[2]) && !json_null(f[3]))) {
		return -EBADMSG;
	}
	if (json_null(f[0])) {
		return 0;
	}
	if (json_kind(f[0]) != '{') {
		return -EBADMSG;
	}
	struct json_cursor regions = json_elements(f[0]);

	map->count = 0;
	while (json_has_element(&regions)) {
		uint64_t id;

		if (uint_value(string_value(json_extract(&regions)), &id) != 0 || id == 0 ||
		    id > UINT16_MAX) {
			return -EBADMSG;
		}
		for (size_t i = 0; i < count; i++) {
			if (seen[i] == id) {
				return -EBADMSG;
			}
		}
		if (count == ARRAY_SIZE(seen)) {
			return -ENOSPC;
		}
		seen[count++] = id;
		(void)json_pop(&regions);
		rc = derp_region(map, json_extract(&regions), id);
		if (rc != 0) {
			return rc;
		}
		json_separator(&regions);
	}
	return 0;
}
#endif /* CONFIG_TAILSCALE_DERP_MAP */

static int policy_response_support(const struct json_span fields[], bool keepalive)
{
	struct json_span browser = fields[RESPONSE_POP_BROWSER_URL];

	if (!json_null(browser)) {
		if (json_kind(browser) != '"') {
			return -EBADMSG;
		}
		if (browser.end - browser.begin > 2) {
			return -EKEYREJECTED;
		}
	}
	if (keepalive) {
		return 0;
	}
	if (!json_null(fields[RESPONSE_SSH])) {
		static const char *const names[] = {"rules"};
		struct json_span rules = {0};
		int rc = json_fields(fields[RESPONSE_SSH], names, 1U, &rules, true);

		if (rc != 0 || (!json_null(rules) && json_kind(rules) != '[')) {
			return rc != 0 ? rc : -EBADMSG;
		}
	}
	if (!json_null(fields[RESPONSE_TKA])) {
		return json_kind(fields[RESPONSE_TKA]) == '{' ? -EKEYREJECTED : -EBADMSG;
	}
	if (!json_null(fields[RESPONSE_DEBUG])) {
		static const char *const names[] = {"DisableLogTail"};
		struct json_span log_tail = {0};
		bool disabled;
		int rc = json_fields(fields[RESPONSE_DEBUG], names, ARRAY_SIZE(names),
				     &log_tail, true);

		/* The native client has no log upload service. */
		if (rc != 0 || (!json_null(log_tail) && bool_value(log_tail, &disabled) != 0)) {
			return rc != 0 ? rc : -EBADMSG;
		}
	}
#ifndef CONFIG_TAILSCALE_DERP_MAP
	/* Direct transport clients can use the rest of the map without relay metadata. */
	if (!json_null(fields[RESPONSE_DERP]) && json_kind(fields[RESPONSE_DERP]) != '{') {
		return -EBADMSG;
	}
#endif
	return 0;
}

static int policy_control_time(struct json_span token, int64_t received_at,
			       int64_t *epoch_at_boot, bool *have_control_time)
{
	char text[TAILSCALE_PEER_KEY_EXPIRY_SIZE];
	int64_t seconds;
	uint32_t ns;
	int rc = text_value(token, text, sizeof(text));

	if (rc == 0) {
		rc = parse_timestamp(text, &seconds, &ns);
	}
	if (rc != 0) {
		return rc;
	}
	if (seconds == GO_ZERO_UNIX_SECONDS && ns == 0U) {
		return 0;
	}
	/* The reference expiry manager discards its pre-2023-01-10 control times. */
	if (seconds < INT64_C(1673373066)) {
		return -EBADMSG;
	}
	int64_t offset = seconds * 1000 + DIV_ROUND_UP(ns, 1000000U) - received_at;

	if (!*have_control_time || offset > *epoch_at_boot) {
		*epoch_at_boot = offset;
		*have_control_time = true;
	}
	return 0;
}

static int apply_policy_response(struct tailscale_policy *policy, struct json_span body,
				 int64_t received_at,
				 tailscale_policy_install_t install, void *context,
				 enum tailscale_peer_inventory_frame *frame)
{
	struct json_span fields[RESPONSE_FIELD_COUNT] = {0};
	bool nonempty[RESPONSE_FIELD_COUNT] = {0};
	bool keepalive = false;
	int64_t epoch_at_boot = policy->control_epoch_at_boot_ms;
	bool have_control_time = policy->have_control_time;
	int rc = json_fields(body, response_names, ARRAY_SIZE(response_names), fields, true);

	if (rc == 0 && !json_null(fields[RESPONSE_KEEPALIVE])) {
		rc = bool_value(fields[RESPONSE_KEEPALIVE], &keepalive);
	}
	if (rc == 0) {
		rc = policy_response_support(fields, keepalive);
	}
	if (rc == 0 && !json_null(fields[RESPONSE_CONTROL_TIME])) {
		rc = policy_control_time(fields[RESPONSE_CONTROL_TIME], received_at,
					 &epoch_at_boot, &have_control_time);
	}
	if (rc != 0) {
		return rc;
	}
	if (keepalive) {
		*frame = TAILSCALE_PEER_INVENTORY_KEEPALIVE;
	} else {
		rc = peer_frame_fields(fields, nonempty);
		if (rc != 0) {
			return rc;
		}
	}

	bool current = policy->summary.valid &&
		       policy->summary.generation == policy->stream_generation;
	bool clock_changed = have_control_time != policy->have_control_time ||
			     epoch_at_boot != policy->control_epoch_at_boot_ms;
	bool node = !keepalive && !json_null(fields[RESPONSE_NODE]);
	bool dns_changed = !keepalive && !json_null(fields[RESPONSE_DNS]);
	bool rules = !keepalive && (!json_null(fields[RESPONSE_FILTER]) ||
				    !json_null(fields[RESPONSE_NAMED_FILTERS]));
	bool peer_delta = nonempty[RESPONSE_CHANGED] || nonempty[RESPONSE_REMOVED] ||
			  nonempty[RESPONSE_PATCHED] || nonempty[RESPONSE_ONLINE];
	bool full = nonempty[RESPONSE_PEERS] ||
		    (!keepalive && !current && (node || !json_null(fields[RESPONSE_PEERS])));
	bool relay_changed = IS_ENABLED(CONFIG_TAILSCALE_DERP_MAP) && !keepalive &&
			     !json_null(fields[RESPONSE_DERP]);
	bool ssh = !keepalive && !json_null(fields[RESPONSE_SSH]);
	bool update = full || node || rules || peer_delta || dns_changed || relay_changed || ssh;
	uint64_t sequence = policy->sequence;

	if (!keepalive) {
		rc = track_sequence(&sequence, fields[RESPONSE_SEQ], update);
		if (rc != 0) {
			return rc;
		}
	}
	if (!update && (!current || !clock_changed)) {
		policy->sequence = sequence;
		policy->control_epoch_at_boot_ms = epoch_at_boot;
		policy->have_control_time = have_control_time;
		return 0;
	}
	if ((!current && !full) || (nonempty[RESPONSE_PEERS] && peer_delta)) {
		return -EBADMSG;
	}

	uint8_t next = policy->published ^ 1U;
	struct tailscale_peer_snapshot *candidate = &policy->peers[next];
	struct tailscale_dns_config *dns = &policy->dns[next];
	struct tailscale_packet_filter *filter = &policy->filters[next];

	if (current) {
		*candidate = policy->peers[policy->published];
		*dns = policy->dns[policy->published];
	} else {
		candidate->have_self = false;
#ifdef CONFIG_TAILSCALE_DERP_MAP
		candidate->derp.count = 0;
#endif
		memset(dns, 0, sizeof(*dns));
	}
	if (full) {
		candidate->count = 0;
	}
	if (node) {
		rc = policy_self(candidate, fields[RESPONSE_NODE]);
	}
	if (rc == 0 && update) {
		rc = apply_peer_changes(candidate, fields, nonempty[RESPONSE_PEERS], true);
	}
	if (rc == 0 && update) {
		rc = policy_validate_peers(candidate);
	}

#ifdef CONFIG_TAILSCALE_DERP_MAP
	if (rc == 0 && relay_changed) {
		rc = compile_derp_map(fields[RESPONSE_DERP], &candidate->derp);
	}
#endif
	if (rc == 0 && dns_changed) {
		rc = compile_dns(fields[RESPONSE_DNS], dns);
	}
	if (rc == 0 && dns->proxied &&
	    (!candidate->have_self || strchr(candidate->self.dns_name, '.') == NULL)) {
		rc = -ENOTSUP;
	}
	if (rc == 0) {
		rc = stage_filter(policy, next, current,
				  rules ? fields[RESPONSE_FILTER] : (struct json_span){0},
				  rules ? fields[RESPONSE_NAMED_FILTERS] : (struct json_span){0});
	}
	if (rc == 0) {
		/* Until an SSH policy consumer exists, reserve its service port
		 * even if a separate network rule permits TCP/22.
		 */
		filter->reserve_ssh =
			ssh || (current && policy->filters[policy->published].reserve_ssh);
		candidate->control_epoch_at_boot_ms = epoch_at_boot;
		candidate->have_control_time = have_control_time;
		rc = install(candidate, filter, context);
		if (rc == -EKEYREJECTED) {
			/* This validated frame proves loss of local authority. */
			policy->control_epoch_at_boot_ms = epoch_at_boot;
			policy->have_control_time = have_control_time;
		}
	}
	if (rc != 0) {
		return rc;
	}
	policy->published = next;
	policy->sequence = sequence;
	policy->control_epoch_at_boot_ms = epoch_at_boot;
	policy->have_control_time = have_control_time;
	if (update) {
		policy->summary.generation = policy->stream_generation;
		policy->summary.peer_count = candidate->count;
		policy->summary.valid = true;
		policy->summary.revision++;
		*frame = full ? TAILSCALE_PEER_INVENTORY_FULL : TAILSCALE_PEER_INVENTORY_UPDATE;
	}
	return 0;
}

void tailscale_policy_init(struct tailscale_policy *policy)
{
	memset(policy, 0, sizeof(*policy));
	k_mutex_init(&policy->lock);
}

int tailscale_policy_apply(struct tailscale_policy *policy, uint32_t generation, uint8_t *json,
			   size_t len, tailscale_policy_install_t install, void *context,
			   enum tailscale_peer_inventory_frame *frame)
{
	enum tailscale_peer_inventory_frame effect = TAILSCALE_PEER_INVENTORY_OTHER;
	struct json_span body;
	int64_t received_at = k_uptime_get();
	int rc;

	if (policy == NULL || install == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&policy->lock, K_FOREVER);
	uint32_t advance = generation - policy->stream_generation;

	if (!policy->have_stream || (advance != 0U && advance <= INT32_MAX)) {
		policy->stream_generation = generation;
		policy->sequence = 0;
		policy->have_stream = true;
		policy->failed = false;
	}
	if (generation != policy->stream_generation || policy->failed) {
		rc = -ESTALE;
	} else {
		rc = read_frame(json, len, &body);
		if (rc == 0) {
			rc = apply_policy_response(policy, body, received_at, install, context,
						   &effect);
		}
		if (rc != 0) {
			policy->failed = true;
		} else if (frame != NULL) {
			*frame = effect;
		}
	}
	k_mutex_unlock(&policy->lock);
	return rc;
}

int tailscale_policy_get_summary(struct tailscale_policy *policy,
				struct tailscale_peer_inventory_summary *summary)
{
	if (policy == NULL || summary == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&policy->lock, K_FOREVER);
	*summary = policy->summary;
	k_mutex_unlock(&policy->lock);
	return 0;
}

int tailscale_policy_dns_authority(struct tailscale_policy *policy, const char *name, bool ready)
{
	static const char magic_suffix[] = "ts.net";
	char normalized[TAILSCALE_DNS_NAME_SIZE];
	struct tailscale_dns_selection selection = {0};
	int rc = dns_query_name(name, normalized);

	if (rc != 0) {
		return rc;
	}

	k_mutex_lock(&policy->lock, K_FOREVER);
	const struct tailscale_dns_config *dns = &policy->dns[policy->published];
	bool short_name = strchr(name, '.') == NULL;
	bool claimed;

	ready = ready && policy->summary.valid;
	if (policy->summary.revision == 0U) {
		/* With no map, protect short names and the reserved MagicDNS zones. */
		size_t length = strlen(normalized);

		claimed = short_name ||
			  (strcmp(normalized, magic_suffix) != 0 &&
			   magic_dns_match(normalized, length, magic_suffix) != 0U);
	} else {
		/* The last committed policy retains DNS ownership across outages. */
		dns_select(dns, &policy->peers[policy->published], normalized, &selection);
		claimed = selection.source != TAILSCALE_DNS_NONE ||
			  (short_name && dns->search_count != 0U);
	}
	k_mutex_unlock(&policy->lock);
	return claimed ? (ready ? 0 : -ENETDOWN) : -ENOTSUP;
}

int tailscale_policy_select_dns(struct tailscale_policy *policy, const char *name,
			       struct tailscale_dns_selection *selection)
{
	char normalized[TAILSCALE_DNS_NAME_SIZE];

	if (policy == NULL || name == NULL || selection == NULL) {
		return -EINVAL;
	}
	int rc = dns_query_name(name, normalized);

	if (rc != 0) {
		return rc;
	}
	k_mutex_lock(&policy->lock, K_FOREVER);
	if (policy->summary.valid) {
		*selection = (struct tailscale_dns_selection){
			.revision = policy->summary.revision,
			.generation = policy->summary.generation,
		};
		dns_select(&policy->dns[policy->published], &policy->peers[policy->published],
			   normalized, selection);
	} else {
		rc = -ENODATA;
	}
	k_mutex_unlock(&policy->lock);
	return rc;
}

int tailscale_policy_dns_search_domain(struct tailscale_policy *policy, uint64_t revision,
				      size_t index, char domain[TAILSCALE_DNS_NAME_SIZE])
{
	if (policy == NULL || domain == NULL) {
		return -EINVAL;
	}
	int rc = 0;

	k_mutex_lock(&policy->lock, K_FOREVER);
	const struct tailscale_dns_config *dns = &policy->dns[policy->published];

	if (!policy->summary.valid) {
		rc = -ENODATA;
	} else if (policy->summary.revision != revision) {
		rc = -ESTALE;
	} else if (index >= dns->search_count) {
		rc = -ENOENT;
	} else {
		memcpy(domain, dns->search[index], strlen(dns->search[index]) + 1U);
	}
	k_mutex_unlock(&policy->lock);
	return rc;
}

int tailscale_policy_resolve_magic_dns(struct tailscale_policy *policy, const char *name,
				      uint8_t family, struct tailscale_dns_answer *answer)
{
	char normalized[TAILSCALE_DNS_NAME_SIZE];

	if (policy == NULL || name == NULL || answer == NULL ||
	    (family != NET_AF_INET && family != NET_AF_INET6)) {
		return -EINVAL;
	}
	int rc = dns_query_name(name, normalized);

	if (rc != 0) {
		return rc;
	}
	k_mutex_lock(&policy->lock, K_FOREVER);
	if (!policy->summary.valid) {
		rc = -ENODATA;
	} else if (!policy->dns[policy->published].proxied) {
		rc = -ENOTSUP;
	} else {
		const struct tailscale_peer_snapshot *peers = &policy->peers[policy->published];
		struct tailscale_dns_answer found = {
			.revision = policy->summary.revision,
			.generation = policy->summary.generation,
		};

		rc = dns_resolve_magic(peers, normalized, family, &found);
		if (rc == -ENOENT && normalized[0] != '\0' && strchr(name, '.') == NULL) {
			const struct tailscale_dns_config *dns = &policy->dns[policy->published];
			size_t label = strlen(normalized);

			normalized[label] = '.';
			for (size_t i = 0; i < dns->search_count && rc == -ENOENT; ++i) {
				size_t suffix = strlen(dns->search[i]);

				if (label + 1U + suffix < sizeof(normalized)) {
					memcpy(normalized + label + 1U, dns->search[i],
					       suffix + 1U);
					rc = dns_resolve_magic(peers, normalized, family, &found);
				}
			}
		}
		if (rc == 0) {
			*answer = found;
		}
	}
	k_mutex_unlock(&policy->lock);
	return rc;
}
#endif /* CONFIG_TAILSCALE_POLICY */

static void invalidate(struct tailscale_peer_inventory *inventory)
{
	if (inventory->summary.valid) {
		inventory->summary.revision++;
	}
	inventory->summary.valid = false;
	inventory->summary.peer_count = 0;
}

static bool generation_is_new(const struct tailscale_peer_inventory *inventory, uint32_t generation)
{
	uint32_t delta = generation - inventory->summary.generation;

	return !inventory->have_generation || (delta != 0 && delta <= INT32_MAX);
}

static void begin_generation(struct tailscale_peer_inventory *inventory, uint32_t generation)
{
	invalidate(inventory);
	inventory->summary.generation = generation;
	inventory->sequence = 0;
	inventory->have_generation = true;
	inventory->failed = false;
}

void tailscale_peer_inventory_init(struct tailscale_peer_inventory *inventory)
{
	memset(inventory, 0, sizeof(*inventory));
	k_mutex_init(&inventory->lock);
}

int tailscale_peer_inventory_apply(struct tailscale_peer_inventory *inventory,
				   uint32_t generation, uint8_t *json, size_t len,
				   enum tailscale_peer_inventory_frame *frame)
{
	enum tailscale_peer_inventory_frame effect = TAILSCALE_PEER_INVENTORY_OTHER;
	struct json_span body;
	int rc;

	if (inventory == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&inventory->lock, K_FOREVER);
	if (generation_is_new(inventory, generation)) {
		begin_generation(inventory, generation);
	}
	if (generation != inventory->summary.generation || inventory->failed) {
		rc = -ESTALE;
		goto out;
	}
	rc = read_frame(json, len, &body);
	if (rc == 0) {
		rc = apply_response(inventory, body, &effect);
	}
	if (rc != 0) {
		invalidate(inventory);
		inventory->failed = true;
	} else if (frame != NULL) {
		*frame = effect;
	}
out:
	k_mutex_unlock(&inventory->lock);
	return rc;
}

void tailscale_peer_inventory_session_lost(struct tailscale_peer_inventory *inventory,
					  uint32_t generation)
{
	k_mutex_lock(&inventory->lock, K_FOREVER);
	if (generation_is_new(inventory, generation)) {
		begin_generation(inventory, generation);
	}
	if (inventory->summary.generation == generation) {
		invalidate(inventory);
		inventory->failed = true;
	}
	k_mutex_unlock(&inventory->lock);
}

int tailscale_peer_inventory_get_summary(struct tailscale_peer_inventory *inventory,
					 struct tailscale_peer_inventory_summary *summary)
{
	if (inventory == NULL || summary == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&inventory->lock, K_FOREVER);
	*summary = inventory->summary;
	k_mutex_unlock(&inventory->lock);
	return 0;
}

int tailscale_peer_inventory_get_peer(struct tailscale_peer_inventory *inventory, uint64_t id,
				      struct tailscale_peer *peer)
{
	int rc = -ENODATA;

	if (inventory == NULL || peer == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&inventory->lock, K_FOREVER);
	if (inventory->summary.valid) {
		struct tailscale_peer *entry =
			find_peer(&inventory->banks[inventory->published], id);

		rc = entry == NULL ? -ENOENT : 0;
		if (entry != NULL) {
			*peer = *entry;
		}
	}
	k_mutex_unlock(&inventory->lock);
	return rc;
}

int tailscale_peer_inventory_get_peer_at(struct tailscale_peer_inventory *inventory,
					 uint64_t revision, size_t index,
					 struct tailscale_peer *peer)
{
	int rc;

	if (inventory == NULL || peer == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&inventory->lock, K_FOREVER);
	if (revision != inventory->summary.revision) {
		rc = -ESTALE;
	} else if (!inventory->summary.valid) {
		rc = -ENODATA;
	} else if (index >= inventory->summary.peer_count) {
		rc = -ENOENT;
	} else {
		*peer = inventory->banks[inventory->published].peers[index];
		rc = 0;
	}
	k_mutex_unlock(&inventory->lock);
	return rc;
}
