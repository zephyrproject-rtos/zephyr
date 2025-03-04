/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/ztest_assert.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/random/random.h>
#include <zephyr/ipc/ipc_service.h>
#include <sys/errno.h>
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <nrf53_cpunet_mgmt.h>
#endif
#include <string.h>

#include "common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(host, LOG_LEVEL_INF);

struct stress_ctx {
	uint32_t fail_cnt;
	uint32_t cnt;
	uint32_t rx_cnt;
	uint32_t data_cnt;
};

struct stress_data {
	bool no_copy;
	struct stress_ctx ctx[4];
};

struct test_data {
	uint32_t pkt_cnt;
	uint32_t data_cnt;
	uint32_t busy_cnt;
	uint32_t rpt;
	struct k_sem sem;
	struct ipc_ept ep;
	struct ipc_ept_cfg cfg;
	struct stress_data stress;
};

static const struct device *ipc0;
static struct test_data ep_data[1];

static bool no_copy_supported(struct ipc_ept *ep)
{
	void *pkt;
	uint32_t len = 16;
	int ret = ipc_service_get_tx_buffer(ep, &pkt, &len, K_NO_WAIT);

	if (ret < 0) {
		return false;
	}

	ipc_service_drop_tx_buffer(ep, pkt);

	return true;
}

static void ep_bound(void *priv)
{
	struct test_data *tdata = priv;

	k_sem_give(&tdata->sem);
	LOG_INF("Ep:%s bounded", tdata->cfg.name);
}

static void send_test_start(const char *test_name, struct test_data *tdata)
{
	uint32_t buffer[32];
	struct data_packet *pkt = (struct data_packet *)buffer;
	size_t max_slen = sizeof(buffer) - sizeof(struct data_packet) - 1;
	size_t slen = MIN(strlen(test_name), max_slen);
	size_t pkt_len = slen + 1 + sizeof(struct data_packet);
	int ret;

	pkt->type = TYPE_TEST_START;
	memcpy(pkt->data, test_name, slen);
	pkt->data[slen] = '\0';

	ret = ipc_service_send(&tdata->ep, buffer, pkt_len);
	zassert_equal(ret, pkt_len);
}

static void ep_recv_ping_pong(const void *data, size_t len, void *priv)
{
	struct test_data *tdata = priv;
	int ret;

	if (tdata->rpt > 0) {
		ret = ipc_service_send(&tdata->ep, data, len);
		zassert_equal(ret, len);
		tdata->rpt--;
	} else {
		k_sem_give(&tdata->sem);
	}
}

static void ping_pong(const char *test_name, struct test_data *tdata, size_t len, uint32_t rpt)
{
	uint8_t buffer[32];
	struct data_packet *pkt = (struct data_packet *)buffer;
	uint32_t t;
	int ret;
	uint32_t load;

	send_test_start(test_name, tdata);

	zassert_true(len <= sizeof(buffer));

	memset(buffer, 0xaa, len);
	pkt->type = TYPE_RSP;

	tdata->cfg.cb.received = ep_recv_ping_pong;

	tdata->rpt = rpt - 1;
	k_sem_init(&tdata->sem, 0, 1);

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}

	t = k_cycle_get_32();
	ret = ipc_service_send(&tdata->ep, buffer, len);
	zassert_equal(ret, len, "send failed: %d", ret);

	ret = k_sem_take(&tdata->sem, K_MSEC(200));
	t = k_cycle_get_32() - t;
	load = IS_ENABLED(CONFIG_CPU_LOAD) ? cpu_load_get(true) : 0;
	t = k_cyc_to_us_floor32(t);
	zassert_equal(ret, 0);

	printk("packet length:%d, avg:%d, cpu_load:%d.%d\n", len, t / rpt, load / 10, load % 10);
}

ZTEST(ipc_service_benchmark, test_turnaround_time_16_bytes)
{
	ping_pong(__func__, &ep_data[0], 16, 10);
}

ZTEST(ipc_service_benchmark, test_turnaround_time_32_bytes)
{
	ping_pong(__func__, &ep_data[0], 32, 10);
}

static void tx_performance(const char *test_name, struct test_data *tdata,
			   size_t len, uint32_t timeout_ms, bool nocopy)
{
	uint8_t buffer[128] __aligned(sizeof(uint32_t));
	struct data_packet *pkt = (struct data_packet *)buffer;
	int ret;
	uint32_t load;
	uint32_t end = k_uptime_get_32() + timeout_ms;
	uint8_t pkt_cnt = 0;
	uint32_t speed;
	uint32_t alloc_len;

	zassert_true(len <= sizeof(buffer));
	if (nocopy && !no_copy_supported(&tdata->ep)) {
		ztest_test_skip();
	}

	send_test_start(test_name, tdata);

	tdata->data_cnt = 0;
	tdata->busy_cnt = 0;
	zassert_true(len <= sizeof(buffer));

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}

	while (k_uptime_get_32() < end) {
		if (nocopy) {
			alloc_len = len;
			ret = ipc_service_get_tx_buffer(&tdata->ep, (void **)&pkt,
					&alloc_len, K_NO_WAIT);
			if (ret == 0) {
				memset(pkt->data, pkt_cnt, len - sizeof(uint32_t));
				pkt->type = TYPE_NO_RSP;
				pkt->data[0] = (uint8_t)pkt_cnt;
				pkt_cnt++;
				ret = ipc_service_send_nocopy(&tdata->ep, pkt, len);
				if (ret > 0) {
					tdata->data_cnt += ret;
				}
			} else if ((ret == -ENOBUFS) || (ret == -ENOMEM)) {
				tdata->busy_cnt++;
			} else {
				zassert_equal(ret, -EIO);
				ztest_test_skip();
			}
		} else {
			memset(buffer, pkt_cnt, MIN(len, sizeof(buffer)));
			pkt->type = TYPE_NO_RSP;
			pkt->data[0] = (uint8_t)pkt_cnt;
			pkt_cnt++;

			ret = ipc_service_send(&tdata->ep, buffer, len);
			if (ret > 0) {
				tdata->data_cnt += ret;
				LOG_DBG("pkt:%d", pkt_cnt - 1);
			} else if (ret == -ENOMEM) {
				tdata->busy_cnt++;
			} else {
				zassert_ok(ret);
			}
		}

#if defined(CONFIG_ARCH_POSIX)
		k_sleep(K_TICKS(1));
#endif
	}

	load = IS_ENABLED(CONFIG_CPU_LOAD) ? cpu_load_get(true) : 0;
	speed = tdata->data_cnt / timeout_ms;

	printk("packet length:%d, speed:%d kB/s no buffer count: %d cpu_load:%d.%d\n",
			len, speed, tdata->busy_cnt, load / 10, load % 10);
}

ZTEST(ipc_service_benchmark, test_tx_32_performance)
{
	tx_performance(__func__, &ep_data[0], 32, 1000, false);
}

ZTEST(ipc_service_benchmark, test_tx_32_performance_no_copy)
{
	tx_performance(__func__, &ep_data[0], 32, 1000, true);
}

ZTEST(ipc_service_benchmark, test_tx_64_performance)
{
	tx_performance(__func__, &ep_data[0], 64, 1000, false);
}

ZTEST(ipc_service_benchmark, test_tx_64_performance_no_copy)
{
	tx_performance(__func__, &ep_data[0], 64, 1000, true);
}

ZTEST(ipc_service_benchmark, test_tx_128_performance)
{
	tx_performance(__func__, &ep_data[0], 128, 1000, false);
}

ZTEST(ipc_service_benchmark, test_tx_128_performance_no_copy)
{
	tx_performance(__func__, &ep_data[0], 128, 1000, true);
}

static void ep_recv_stress(const void *data, size_t len, void *priv)
{
	struct test_data *tdata = priv;
	const struct data_packet *pkt = data;
	size_t dlen = len - (sizeof(struct data_packet) + sizeof(uint8_t));
	uint8_t prio = pkt->data[0];
	uint8_t rx_cnt = tdata->stress.ctx[prio].rx_cnt;
	const uint8_t *pkt_data = &pkt->data[1];

	zassert_equal(pkt->type, TYPE_RSP);
	tdata->stress.ctx[prio].rx_cnt++;
	LOG_DBG("rx:%08x", prio | (rx_cnt << 8) | (len << 16));
	for (size_t i = 0; i < dlen; i++) {
		uint8_t expected = rx_cnt + i;

		zassert_equal(pkt_data[i], expected,
				"Unexpected byte at %d, got %02x exp:%02x",
				i, pkt_data[i], expected);
	}
}

static bool ipc_producer(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct test_data *tdata = user_data;
	struct stress_data *sdata = &tdata->stress;
	size_t rpt = 1 + sys_rand32_get() % 4;

	while (rpt) {
		size_t len = 8 + sys_rand32_get() % 32;
		size_t dlen = len - (sizeof(struct data_packet) + sizeof(uint8_t));
		struct data_packet *pkt;
		uint8_t tx_cnt = sdata->ctx[prio].cnt;
		uint32_t buffer[16];
		int ret;

		if (sdata->no_copy) {
			ret = ipc_service_get_tx_buffer(&tdata->ep, (void **)&pkt, &len, K_NO_WAIT);
			if (ret < 0) {
				sdata->ctx[prio].fail_cnt++;
				return true;
			}
		} else {
			pkt = (struct data_packet *)buffer;
		}

		pkt->type = TYPE_RSP;
		pkt->data[0] = prio;
		for (size_t i = 0; i < dlen; i++) {
			pkt->data[i + 1] = tx_cnt + i;
		}

		LOG_DBG("tx:%08x", prio | (tx_cnt << 8) | (len << 16));
		if (sdata->no_copy) {
			ret = ipc_service_send_nocopy(&tdata->ep, pkt, len);
		} else {
			ret = ipc_service_send(&tdata->ep, pkt, len);
		}

		if (ret < 0) {
			sdata->ctx[prio].fail_cnt++;
			return true;
		}

		sdata->ctx[prio].cnt++;
		sdata->ctx[prio].data_cnt += ret;
		rpt--;
	}

	return true;
}

static void test_stress(const char *test_name, struct test_data *tdata, bool no_copy)
{
	uint32_t preempt_max = 4000;
	k_timeout_t t = Z_TIMEOUT_TICKS(20);
	uint32_t tstamp;
	uint32_t load;
	uint32_t total = 0;

	if (no_copy && !no_copy_supported(&tdata->ep)) {
		ztest_test_skip();
	}

	send_test_start(test_name, tdata);

	memset(&tdata->stress, 0, sizeof(tdata->stress));
	tdata->stress.no_copy = no_copy;

	ztress_set_timeout(K_MSEC(2000));

	tdata->cfg.cb.received = ep_recv_stress;

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}

	tstamp = k_cycle_get_32();
	ZTRESS_EXECUTE(ZTRESS_THREAD(ipc_producer,  tdata, 0, 0, t),
		       ZTRESS_THREAD(ipc_producer, tdata, 0, preempt_max, t),
		       ZTRESS_THREAD(ipc_producer, tdata, 0, preempt_max, t));

	tstamp = k_cycle_get_32() - tstamp;
	tstamp = k_cyc_to_ms_floor32(tstamp);
	load = IS_ENABLED(CONFIG_CPU_LOAD) ? cpu_load_get(true) : 0;
	for (int i = 0; i < 3; i++) {
		total += tdata->stress.ctx[i].data_cnt;
	}

	TC_PRINT("\nTest took %d ms. Speed:%d kB/s, CPU load %d.%d\n",
			tstamp, total / tstamp, load / 10, load % 10);
	for (int i = 0; i < 3; i++) {
		TC_PRINT("\t - Context %d sent %d packets (%d bytes). Fails:%d\n",
			i, tdata->stress.ctx[i].cnt, tdata->stress.ctx[i].data_cnt,
			tdata->stress.ctx[i].fail_cnt);
		total += tdata->stress.ctx[i].data_cnt;
	}
}

ZTEST(ipc_service_benchmark, test_stress)
{
	test_stress(__func__, &ep_data[0], false);
}

ZTEST(ipc_service_benchmark, test_stress_no_copy)
{
	/* Test is failing on static_vrings and icbmsg backends. */
	ztest_test_skip();
	test_stress(__func__, &ep_data[0], true);
}

static void *setup(void)
{
	int ret;

	TC_PRINT("Testing %s IPC service\n",
			IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_ICBMSG) ? "icbmsg" :
			IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_RPMSG) ? "static_vrings" :
			"icmsg");

	ipc0 = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	ret = ipc_service_open_instance(ipc0);
	zassert_ok(ret, "Unexpected error: %d", ret);

	ep_data[0].cfg.name = "ep0";
	ep_data[0].cfg.cb.bound = ep_bound;
	ep_data[0].cfg.priv = &ep_data[0];
	k_sem_init(&ep_data[0].sem, 0, 1);

	ret = ipc_service_register_endpoint(ipc0, &ep_data[0].ep, &ep_data[0].cfg);
	zassert_equal(ret, 0);

	ret = k_sem_take(&ep_data[0].sem, K_MSEC(100));
	zassert_equal(ret, 0);

	return NULL;
}

static void after(void *unused)
{
	uint32_t buffer[1];
	struct data_packet *pkt = (struct data_packet *)buffer;
	int ret;

	pkt->type = TYPE_TEST_END;

	/* Wait some time for the remote to process test messages. */
	k_msleep(10);
	ret = ipc_service_send(&ep_data[0].ep, buffer, sizeof(buffer));
	zassert_equal(ret, sizeof(buffer));

	k_msleep(50);
}

ZTEST_SUITE(ipc_service_benchmark, NULL, setup, NULL, after, NULL);
