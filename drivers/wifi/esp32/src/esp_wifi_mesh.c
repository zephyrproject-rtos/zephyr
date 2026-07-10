/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wi-Fi mesh support: mesh bring-up, the application event API, and the
 * esp_event handler registry the vendor mesh stack posts through.
 *
 * The mesh stack registers handlers on WIFI_EVENT and posts MESH_EVENT, and
 * this file registers an internal MESH_EVENT handler that translates the
 * vendor events into the Zephyr-native esp_wifi_mesh_event callback. Every
 * posted event flows through esp_event_post(), which dispatches it to the
 * handlers kept in this registry in addition to the core Wi-Fi driver handler.
 *
 * esp_event_handler_register()/esp_event_handler_unregister() are the event-API
 * entry points the mesh blob links against (it holds undefined references to
 * them). They are implemented here as a minimal Zephyr-side registry; the
 * matching esp_event_post() lives in the core Wi-Fi driver, which owns the
 * event task.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/wifi/esp_wifi_mesh.h>

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"

#include "esp_wifi_mesh_priv.h"

LOG_MODULE_REGISTER(esp_wifi_mesh, CONFIG_WIFI_LOG_LEVEL);

/* The mesh softAP uses WPA2-PSK, which requires an 8..63 character password. */
BUILD_ASSERT(sizeof(CONFIG_WIFI_ESP32_MESH_AP_PSK) - 1 >= 8 &&
		     sizeof(CONFIG_WIFI_ESP32_MESH_AP_PSK) - 1 <= 63,
	     "CONFIG_WIFI_ESP32_MESH_AP_PSK must be 8 to 63 characters");

/*
 * When the mesh stack is active it drives the station and softAP directly.
 * Its softAP start/stop and association events would otherwise take the
 * station interface administratively down, so those transitions are skipped
 * while the mesh runs and the station interface is kept up.
 *
 * Written from the mesh event context and read from the Wi-Fi TX and event
 * paths, so accessed through the atomic API.
 */
static atomic_t mesh_active;

/*
 * Given by the MESH_EVENT_STARTED handler once the mesh stack has brought its
 * internal tasks up. esp_wifi_mesh_start() waits on it before re-delivering the
 * initial station-start event, so the sequencing follows the actual readiness
 * signal instead of a fixed delay.
 */
static K_SEM_DEFINE(mesh_started_sem, 0, 1);

static void esp_wifi_mesh_set_active(bool active)
{
	atomic_set(&mesh_active, active ? 1 : 0);
}

bool esp_wifi_mesh_is_active(void)
{
	return atomic_get(&mesh_active) != 0;
}

#define MESH_EVENT_MAX_HANDLERS CONFIG_WIFI_ESP32_MESH_EVENT_HANDLERS

struct event_handler_entry {
	esp_event_base_t base;
	int32_t id;
	esp_event_handler_t handler;
	void *arg;
	bool used;
};

static struct event_handler_entry event_handlers[MESH_EVENT_MAX_HANDLERS];
static struct k_spinlock event_lock;

/*
 * Serializes the vendor routing-table reads. The read fills a shared static
 * buffer here and is invoked from the mesh send path, the routing-table query,
 * and the IP-over-mesh downlink broadcast, which can run from different threads
 * at once, so the lock and the buffer stay behind this one accessor.
 */
static K_MUTEX_DEFINE(route_lock);

/* Serializes concurrent root senders so they do not overwrite the shared
 * routing-table snapshot buffer in esp_wifi_mesh_send() mid-loop.
 */
static K_MUTEX_DEFINE(send_lock);

void esp_wifi_mesh_read_routing_table(mesh_addr_t *buf, size_t buf_size, int *count)
{
	int table_size = 0;

	k_mutex_lock(&route_lock, K_FOREVER);
	esp_mesh_get_routing_table(buf, (int)buf_size, &table_size);
	k_mutex_unlock(&route_lock);

	*count = table_size;
}

esp_err_t esp_event_handler_register(esp_event_base_t event_base, int32_t event_id,
				     esp_event_handler_t event_handler, void *event_handler_arg)
{
	k_spinlock_key_t key = k_spin_lock(&event_lock);

	for (int i = 0; i < MESH_EVENT_MAX_HANDLERS; i++) {
		if (!event_handlers[i].used) {
			event_handlers[i].base = event_base;
			event_handlers[i].id = event_id;
			event_handlers[i].handler = event_handler;
			event_handlers[i].arg = event_handler_arg;
			event_handlers[i].used = true;
			k_spin_unlock(&event_lock, key);
			return ESP_OK;
		}
	}

	k_spin_unlock(&event_lock, key);
	LOG_WRN("event handler registry full (%d), handler dropped", MESH_EVENT_MAX_HANDLERS);
	return ESP_ERR_NO_MEM;
}

esp_err_t esp_event_handler_unregister(esp_event_base_t event_base, int32_t event_id,
				       esp_event_handler_t event_handler)
{
	esp_err_t ret = ESP_ERR_NOT_FOUND;
	k_spinlock_key_t key = k_spin_lock(&event_lock);

	for (int i = 0; i < MESH_EVENT_MAX_HANDLERS; i++) {
		if (event_handlers[i].used && event_handlers[i].handler == event_handler &&
		    event_handlers[i].base == event_base && event_handlers[i].id == event_id) {
			event_handlers[i].used = false;
			ret = ESP_OK;
			break;
		}
	}

	k_spin_unlock(&event_lock, key);
	return ret;
}

void esp_wifi_mesh_dispatch_event(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	esp_event_handler_t matched[MESH_EVENT_MAX_HANDLERS];
	void *matched_arg[MESH_EVENT_MAX_HANDLERS];
	int matched_count = 0;

	/*
	 * Snapshot the matching handlers under the lock, then invoke them with
	 * the lock released. A handler may block, so it must not run while the
	 * spinlock is held, and the snapshot avoids racing a concurrent
	 * register/unregister.
	 */
	k_spinlock_key_t key = k_spin_lock(&event_lock);

	for (int i = 0; i < MESH_EVENT_MAX_HANDLERS; i++) {
		struct event_handler_entry *e = &event_handlers[i];

		if (!e->used) {
			continue;
		}
		if (e->base != ESP_EVENT_ANY_BASE && e->base != event_base) {
			continue;
		}
		if (e->id != ESP_EVENT_ANY_ID && e->id != event_id) {
			continue;
		}
		matched[matched_count] = e->handler;
		matched_arg[matched_count] = e->arg;
		matched_count++;
	}

	k_spin_unlock(&event_lock, key);

	for (int i = 0; i < matched_count; i++) {
		matched[i](matched_arg[i], event_base, event_id, event_data);
	}
}

/*
 * Parse the 6-byte mesh id from its "xx:xx:xx:xx:xx:xx" Kconfig string into
 * out. On a malformed value fall back to a fixed default so the mesh still
 * comes up rather than joining an unintended network.
 */
static void mesh_id_parse(uint8_t out[6])
{
	static const uint8_t fallback[6] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};
	const char *p = CONFIG_WIFI_ESP32_MESH_ID;
	uint8_t id[6];

	for (int i = 0; i < 6; i++) {
		uint8_t hi, lo;

		if (char2hex(p[0], &hi) != 0 || char2hex(p[1], &lo) != 0) {
			goto invalid;
		}
		id[i] = (hi << 4) | lo;
		p += 2;

		if (i < 5) {
			if (*p != ':') {
				goto invalid;
			}
			p++;
		}
	}

	if (*p != '\0') {
		goto invalid;
	}

	memcpy(out, id, sizeof(id));
	return;

invalid:
	LOG_WRN("invalid mesh id '%s', using default", CONFIG_WIFI_ESP32_MESH_ID);
	memcpy(out, fallback, sizeof(fallback));
}

static esp_wifi_mesh_event_cb_t app_event_cb;

void esp_wifi_mesh_event_cb_register(esp_wifi_mesh_event_cb_t cb)
{
	app_event_cb = cb;
}

static bool tods_reachable;

void esp_wifi_mesh_set_tods_reachable(bool reachable)
{
	tods_reachable = reachable;
	esp_mesh_post_toDS_state(reachable ? MESH_TODS_REACHABLE : MESH_TODS_UNREACHABLE);
}

void esp_wifi_mesh_get_status(struct esp_wifi_mesh_status *status)
{
	if (status == NULL) {
		return;
	}

	status->layer = esp_mesh_get_layer();
	status->is_root = esp_mesh_is_root();
	status->routing_table_size = esp_mesh_get_routing_table_size();
	status->tods_reachable = tods_reachable;
}

int esp_wifi_mesh_get_routing_table(struct esp_wifi_mesh_addr *macs, size_t max, size_t *count)
{
	static mesh_addr_t
		route[CONFIG_WIFI_ESP32_MESH_MAX_LAYER * CONFIG_WIFI_ESP32_MESH_MAX_CONNECTIONS +
		      1];
	int table_size = 0;

	if (macs == NULL || count == NULL) {
		return -EINVAL;
	}

	esp_wifi_mesh_read_routing_table(route, sizeof(route), &table_size);

	size_t n = MIN((size_t)table_size, max);

	for (size_t i = 0; i < n; i++) {
		memcpy(macs[i].addr, route[i].addr, sizeof(macs[i].addr));
	}

	*count = n;
	return 0;
}

static void notify_app(enum esp_wifi_mesh_event event, int reason)
{
	if (app_event_cb == NULL) {
		return;
	}

	struct esp_wifi_mesh_event_info info = {
		.layer = esp_mesh_get_layer(),
		.is_root = esp_mesh_is_root(),
		.routing_table_size = esp_mesh_get_routing_table_size(),
		.reason = reason,
	};

	app_event_cb(event, &info);
}

/* Translate the vendor mesh events into the Zephyr-native callback. */
static void mesh_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(base);

	switch (event_id) {
	case MESH_EVENT_STARTED:
		esp_wifi_mesh_set_active(true);
		k_sem_give(&mesh_started_sem);
		notify_app(ESP_WIFI_MESH_EVENT_STARTED, 0);
		break;
	case MESH_EVENT_STOPPED:
		notify_app(ESP_WIFI_MESH_EVENT_STOPPED, 0);
		break;
	case MESH_EVENT_PARENT_CONNECTED:
		notify_app(ESP_WIFI_MESH_EVENT_PARENT_CONNECTED, 0);
		break;
	case MESH_EVENT_PARENT_DISCONNECTED: {
		mesh_event_disconnected_t *disc = event_data;

		notify_app(ESP_WIFI_MESH_EVENT_PARENT_DISCONNECTED,
			   disc != NULL ? disc->reason : 0);
		break;
	}
	case MESH_EVENT_NO_PARENT_FOUND:
		notify_app(ESP_WIFI_MESH_EVENT_NO_PARENT_FOUND, 0);
		break;
	case MESH_EVENT_FIND_NETWORK:
		notify_app(ESP_WIFI_MESH_EVENT_FIND_NETWORK, 0);
		break;
	case MESH_EVENT_CHILD_CONNECTED:
		notify_app(ESP_WIFI_MESH_EVENT_CHILD_CONNECTED, 0);
		break;
	case MESH_EVENT_CHILD_DISCONNECTED:
		notify_app(ESP_WIFI_MESH_EVENT_CHILD_DISCONNECTED, 0);
		break;
	case MESH_EVENT_ROUTING_TABLE_ADD:
	case MESH_EVENT_ROUTING_TABLE_REMOVE:
		notify_app(ESP_WIFI_MESH_EVENT_ROUTING_TABLE_CHANGE, 0);
		break;
	case MESH_EVENT_TODS_STATE: {
		mesh_event_toDS_state_t *state = event_data;

		if (state == NULL) {
			break;
		}

		notify_app((*state == MESH_TODS_REACHABLE) ? ESP_WIFI_MESH_EVENT_TODS_REACHABLE
							   : ESP_WIFI_MESH_EVENT_TODS_UNREACHABLE,
			   0);
		break;
	}
	default:
		LOG_DBG("mesh event %d", event_id);
		break;
	}
}

int esp_wifi_mesh_start(void)
{
	mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();

	/*
	 * Self-organized election is driven by router reachability, so the auto
	 * role needs a router. A fixed root or fixed child does not run an
	 * election and can form a router-less mesh, so the router is optional.
	 */
	if (IS_ENABLED(CONFIG_WIFI_ESP32_MESH_ROLE_AUTO) &&
	    strlen(CONFIG_WIFI_ESP32_MESH_ROUTER_SSID) == 0) {
		LOG_ERR("router SSID must be set (CONFIG_WIFI_ESP32_MESH_ROUTER_SSID)");
		return -EINVAL;
	}

	/* The Wi-Fi driver has already run esp_wifi_init()/esp_wifi_start(). */
	esp_wifi_set_mode(ESP32_WIFI_MODE_APSTA);

	if (esp_mesh_init() != ESP_OK) {
		LOG_ERR("esp_mesh_init failed");
		return -EIO;
	}

	if (esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, mesh_event_handler, NULL) !=
	    ESP_OK) {
		LOG_ERR("mesh event register failed");
		return -EIO;
	}

	esp_mesh_set_topology(MESH_TOPO_TREE);
	esp_mesh_set_max_layer(CONFIG_WIFI_ESP32_MESH_MAX_LAYER);
	esp_mesh_set_vote_percentage((float)CONFIG_WIFI_ESP32_MESH_VOTE_PERCENTAGE / 100.0f);
	esp_mesh_set_xon_qsize(CONFIG_WIFI_ESP32_MESH_XON_QSIZE);

	/* Mesh manages the radio power state; keep power save disabled. */
	esp_mesh_disable_ps();

	esp_mesh_set_ap_assoc_expire(CONFIG_WIFI_ESP32_MESH_ASSOC_EXPIRE);

	cfg.channel = CONFIG_WIFI_ESP32_MESH_CHANNEL;
	if (cfg.channel == 0) {
		/* Scan all channels to locate the router. */
		cfg.allow_channel_switch = true;
	}
	uint8_t mesh_id[6];

	mesh_id_parse(mesh_id);
	memcpy((uint8_t *)&cfg.mesh_id, mesh_id, sizeof(mesh_id));

	/*
	 * Router credentials: only the elected or fixed root associates to the
	 * router. esp_mesh_set_config rejects the configuration if the router
	 * SSID is empty, so a router-less fixed-root mesh supplies a placeholder
	 * SSID the fixed root never actually associates to; its children attach
	 * to the root's softAP and the mesh forms with no router involved.
	 */
	const char *router_ssid = CONFIG_WIFI_ESP32_MESH_ROUTER_SSID;
	const char *router_psk = CONFIG_WIFI_ESP32_MESH_ROUTER_PSK;

	if (strlen(router_ssid) == 0) {
		router_ssid = "esp-mesh-no-router";
		router_psk = "";
	}

	cfg.router.ssid_len = MIN(strlen(router_ssid), sizeof(cfg.router.ssid));
	memcpy(cfg.router.ssid, router_ssid, cfg.router.ssid_len);
	memcpy(cfg.router.password, router_psk,
	       MIN(strlen(router_psk), sizeof(cfg.router.password) - 1));

	esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK);
	cfg.mesh_ap.max_connection = CONFIG_WIFI_ESP32_MESH_MAX_CONNECTIONS;
	memcpy(cfg.mesh_ap.password, CONFIG_WIFI_ESP32_MESH_AP_PSK,
	       MIN(strlen(CONFIG_WIFI_ESP32_MESH_AP_PSK), sizeof(cfg.mesh_ap.password) - 1));

	esp_err_t cfg_ret = esp_mesh_set_config(&cfg);

	if (cfg_ret != ESP_OK) {
		LOG_ERR("esp_mesh_set_config failed (0x%x)", cfg_ret);
		return -EIO;
	}

	/*
	 * Fixed-root network: election is disabled and the root is designated,
	 * not voted for. Every node must share the fixed-root setting; only the
	 * designated node sets its type to root.
	 */
#if defined(CONFIG_WIFI_ESP32_MESH_FORCE_ROOT)
	esp_mesh_fix_root(true);
	esp_mesh_set_type(MESH_ROOT);
	LOG_INF("mesh fixed root");
#elif defined(CONFIG_WIFI_ESP32_MESH_FORCE_CHILD)
	esp_mesh_fix_root(true);
#endif

	k_sem_reset(&mesh_started_sem);

	if (esp_mesh_start() != ESP_OK) {
		LOG_ERR("esp_mesh_start failed");
		return -EIO;
	}

	/*
	 * Mark the mesh active synchronously so the station/softAP event guards
	 * take effect immediately. MESH_EVENT_STARTED confirms it later on the
	 * event task, but that leaves a window where bring-up events would take
	 * the shared station interface down before the flag is set.
	 */
	esp_wifi_mesh_set_active(true);

#if defined(CONFIG_WIFI_ESP32_MESH_FORCE_ROOT)
	/*
	 * Without a router there is no parent for the fixed root to find, so the
	 * self-organized search only keeps scanning and disrupts its softAP,
	 * which stops children from attaching. With a router configured it must
	 * stay enabled: self-organized networking also drives the association to
	 * the router and the reconnection that maintains the root uplink.
	 */
	if (strlen(CONFIG_WIFI_ESP32_MESH_ROUTER_SSID) == 0) {
		esp_mesh_set_self_organized(false, false);
	}
#endif

	/* Let mesh sends block under congestion instead of failing fast. */
	esp_mesh_send_block_time(CONFIG_WIFI_ESP32_MESH_SEND_BLOCK_TIME_MS);

	/*
	 * The Wi-Fi station is already started by the driver before the mesh
	 * stack registers its event handlers, so the mesh misses the initial
	 * WIFI_EVENT_STA_START that would kick off parent scanning. Wait for the
	 * mesh stack to report MESH_EVENT_STARTED (its internal tasks are then
	 * up), then re-deliver the event so the mesh begins its self-organizing
	 * sequence. Waiting on the readiness event makes the sequencing
	 * deterministic instead of depending on a fixed delay.
	 */
	if (k_sem_take(&mesh_started_sem, K_MSEC(CONFIG_WIFI_ESP32_MESH_START_TIMEOUT)) != 0) {
		LOG_WRN("mesh start not confirmed in %d ms, proceeding anyway",
			CONFIG_WIFI_ESP32_MESH_START_TIMEOUT);
	}

	esp_wifi_mesh_dispatch_event(WIFI_EVENT, WIFI_EVENT_STA_START, NULL);

	return 0;
}

#if !defined(CONFIG_WIFI_ESP32_MESH_IP)
/*
 * Raw application messaging path. It reads the mesh receive queue directly with
 * esp_wifi_mesh_recv(). The IP-over-mesh interface (esp_wifi_mesh_netif.c) reads
 * the same single queue from its own task, and esp_mesh_recv() does not demux by
 * destination, so with both active a frame goes to whichever reader pops it
 * first. The two paths are therefore kept mutually exclusive at build time.
 *
 * TODO: fold both readers into one receive task that demultiplexes on the frame
 * protocol tag (MESH_PROTO_BIN for this raw path, MESH_PROTO_STA/_AP for IP) and
 * routes each frame to the right consumer, which would let BIN and IP share the
 * mesh instead of being mutually exclusive.
 */
int esp_wifi_mesh_send(const uint8_t *data, size_t len)
{
	mesh_data_t msg = {
		.data = (uint8_t *)data,
		.size = len,
		.proto = MESH_PROTO_BIN,
		.tos = MESH_TOS_P2P,
	};

	if (esp_mesh_is_root()) {
		/* The root routing table spans the whole sub-network. */
		static mesh_addr_t route[CONFIG_WIFI_ESP32_MESH_MAX_LAYER *
						 CONFIG_WIFI_ESP32_MESH_MAX_CONNECTIONS +
					 1];
		int table_size = 0;
		int ret = 0;

		/*
		 * Snapshot the whole routing table once (the accessor takes the
		 * routing-table lock), then send with that lock released, since
		 * esp_mesh_send() can block for up to the configured send block
		 * time. send_lock serializes concurrent senders so the shared
		 * snapshot buffer is not overwritten mid-loop.
		 */
		k_mutex_lock(&send_lock, K_FOREVER);
		esp_wifi_mesh_read_routing_table(route, sizeof(route), &table_size);
		table_size = CLAMP(table_size, 0, (int)ARRAY_SIZE(route));

		for (int i = 0; i < table_size; i++) {
			if (esp_mesh_send(&route[i], &msg, MESH_DATA_P2P, NULL, 0) != ESP_OK) {
				ret = -EIO;
			}
		}
		k_mutex_unlock(&send_lock);
		return ret;
	}

	/* Non-root: to == NULL routes the packet up to the root. */
	if (esp_mesh_send(NULL, &msg, MESH_DATA_P2P, NULL, 0) != ESP_OK) {
		return -EIO;
	}

	return 0;
}

int esp_wifi_mesh_recv(uint8_t *data, size_t *len, int timeout_ms)
{
	static uint8_t rx_bounce[MESH_MPS];
	mesh_addr_t from;
	mesh_data_t msg = {
		.data = rx_bounce,
		.size = sizeof(rx_bounce),
	};
	int flag = 0;
	size_t cap;

	if (data == NULL || len == NULL) {
		return -EINVAL;
	}
	cap = *len;

	/*
	 * The vendor receive writes the whole frame into the buffer and only
	 * then reports its length, so it must be given a buffer sized for the
	 * largest possible payload. Receive into a full-size bounce buffer and
	 * copy back no more than the caller's capacity to avoid overflowing a
	 * smaller caller buffer.
	 */
	if (esp_mesh_recv(&from, &msg, timeout_ms, &flag, NULL, 0) != ESP_OK) {
		return -EAGAIN;
	}

	*len = MIN((size_t)msg.size, cap);
	memcpy(data, rx_bounce, *len);
	return 0;
}
#endif /* !CONFIG_WIFI_ESP32_MESH_IP */
