/*
 * Copyright 2025 CISPA Helmholtz Center for Information Security gGmbH
 *
 * @file Driver for ha1588 PTP clock device.
 * Compatible with ha1588 Linux driver by Geon Technologies, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ha1588_rtc_1_0

#define LOG_MODULE_NAME ptp_clock_ha1588
#define LOG_LEVEL       CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/drivers/ptp/ptp_clock_ha1588.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#ifdef CONFIG_PTP_CLOCK_HA1588_TSU
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

/* FIXME should have a generic define somewhere */
#define PTP_SOCKET_PORT_EVENT   (319)
#define PTP_SOCKET_PORT_GENERAL (320)
#endif

/* HA1588 register map */
#define HA1588_RTC_CTRL       0x00000000
#define HA1588_RTC_NULL_0x04  0x00000004
#define HA1588_RTC_NULL_0x08  0x00000008
#define HA1588_RTC_NULL_0x0C  0x0000000C
#define HA1588_RTC_TIME_SEC_H 0x00000010
#define HA1588_RTC_TIME_SEC_L 0x00000014
#define HA1588_RTC_TIME_NSC_H 0x00000018
#define HA1588_RTC_TIME_NSC_L 0x0000001C
#define HA1588_RTC_PERIOD_H   0x00000020
#define HA1588_RTC_PERIOD_L   0x00000024
#define HA1588_RTC_ADJPER_H   0x00000028
#define HA1588_RTC_ADJPER_L   0x0000002C
#define HA1588_RTC_ADJNUM     0x00000030
#define HA1588_RTC_NULL_0x34  0x00000034
#define HA1588_RTC_NULL_0x38  0x00000038
#define HA1588_RTC_NULL_0x3C  0x0000003C
#define HA1588_TSU_RXQ_CTRL   0x00000040
#define HA1588_TSU_RXQ_FILTER 0x00000044
#define HA1588_TSU_NULL_0x48  0x00000048
#define HA1588_TSU_NULL_0x4c  0x0000004c
#define HA1588_TSU_RXQ_OUT_0  0x00000050
#define HA1588_TSU_RXQ_OUT_1  0x00000054
#define HA1588_TSU_RXQ_OUT_2  0x00000058
#define HA1588_TSU_RXQ_OUT_3  0x0000005c
#define HA1588_TSU_TXQ_CTRL   0x00000060
#define HA1588_TSU_TXQ_FILTER 0x00000064
#define HA1588_TSU_NULL_0x68  0x00000068
#define HA1588_TSU_NULL_0x6c  0x0000006c
#define HA1588_TSU_TXQ_OUT_0  0x00000070
#define HA1588_TSU_TXQ_OUT_1  0x00000074
#define HA1588_TSU_TXQ_OUT_2  0x00000078
#define HA1588_TSU_TXQ_OUT_3  0x0000007c

/* masks for control register */
#define HA1588_RTC_RESET_CTRL 0x00
#define HA1588_RTC_GET_TIME   0x01
#define HA1588_RTC_SET_ADJ    0x02
#define HA1588_RTC_SET_PERIOD 0x04
#define HA1588_RTC_SET_TIME   0x08
#define HA1588_RTC_SET_RESET  0x10

/* masks for TSU queue control reg */
#define HA1588_TSU_RESET_CTRL    0x0
#define HA1588_TSU_READ          0x01
#define HA1588_TSU_RESET         0x02
#define HA1588_TSU_TIMESTAMP_ALL 0x04

/* mask for TSU filter reg - 8-bit bitmap for PTP message ID */
#define HA1588_PTP_MSGID_ANY 0xff000000

struct ptp_clock_ha1588_config {
	mem_addr_t reg_addr;
	/* nominal period of the RTC clock */
	uint64_t rtc_period;
	bool has_timestamp_everything_option;
};

struct ptp_clock_ha1588_data {
	struct k_spinlock lock;
	bool timestamp_everything_enabled_rx;
	bool timestamp_everything_enabled_tx;
};

static int ptp_clock_ha1588_set_period(const struct device *dev, uint64_t period)
{
	const uint32_t period_ns = (period >> 32) & 0xff;
	const uint32_t period_fractional_ns = (period & 0xffffffff);
	const struct ptp_clock_ha1588_config *config = dev->config;

	if (!period) {
		LOG_WRN("Expected non-zero period - not setting period!");
		return -EINVAL;
	}

	LOG_DBG("Setting ha1588 period to %" PRIu32 " ns %" PRIu32 " fractional ns!", period_ns,
		period_fractional_ns);

	sys_write32(period_ns, config->reg_addr + HA1588_RTC_PERIOD_H);
	sys_write32(period_fractional_ns, config->reg_addr + HA1588_RTC_PERIOD_L);

	sys_write32(HA1588_RTC_RESET_CTRL, config->reg_addr + HA1588_RTC_CTRL);
	sys_write32(HA1588_RTC_SET_PERIOD, config->reg_addr + HA1588_RTC_CTRL);

	return 0;
}

static int ptp_clock_ha1588_set_unlocked(const struct device *dev, struct net_ptp_time *tm)
{
	const struct ptp_clock_ha1588_config *config = dev->config;
	const uint64_t tval_sec = tm->second;
	const uint32_t sec_lower = tval_sec & 0xffffffff;
	const uint32_t sec_higher = tval_sec >> 32;
	const uint64_t tval_nsec = tm->nanosecond;
	/* fractional value here */
	const uint32_t nsec_lower = 0;
	const uint32_t nsec_higher = tval_nsec & 0x3fffffff;

	sys_write32(sec_higher, config->reg_addr + HA1588_RTC_TIME_SEC_H);
	sys_write32(sec_lower, config->reg_addr + HA1588_RTC_TIME_SEC_L);
	sys_write32(nsec_higher, config->reg_addr + HA1588_RTC_TIME_NSC_H);
	sys_write32(nsec_lower, config->reg_addr + HA1588_RTC_TIME_NSC_L);

	sys_write32(HA1588_RTC_RESET_CTRL, config->reg_addr + HA1588_RTC_CTRL);
	sys_write32(HA1588_RTC_SET_TIME, config->reg_addr + HA1588_RTC_CTRL);

	LOG_DBG("ha1588 setting time to %" PRIu64 ".%" PRIu64 "!", tval_sec, tval_nsec);

	if ((sec_higher & 0xffff0000) != 0) {
		LOG_ERR("ha1588 has 48-bit range for seconds!");
		return -ERANGE;
	}

	return 0;
}

static int ptp_clock_ha1588_set(const struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_clock_ha1588_data *data = dev->data;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = ptp_clock_ha1588_set_unlocked(dev, tm);

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ptp_clock_ha1588_get_unlocked(const struct device *dev, struct net_ptp_time *tm)
{
	const struct ptp_clock_ha1588_config *config = dev->config;

	uint64_t tval_sec;
	uint32_t sec_lower;
	uint32_t sec_higher;
	/* fractional value here */
	uint32_t nsec_lower;
	uint32_t nsec_higher;
	uint32_t ctrl_val;

	/*
	 * procedure for reading time: need to request snapshot of current time via control
	 * ha1588 will synchronize a timestamp from the RTC to the register interface
	 * and confirm with a control bit that the time value is now available
	 * after that, control registers will be frozen until next timestamp is requested
	 */
	sys_write32(HA1588_RTC_RESET_CTRL, config->reg_addr + HA1588_RTC_CTRL);
	sys_write32(HA1588_RTC_GET_TIME, config->reg_addr + HA1588_RTC_CTRL);

	do {
		ctrl_val = sys_read32(config->reg_addr + HA1588_RTC_CTRL);
	} while ((ctrl_val & HA1588_RTC_GET_TIME) == 0);

	nsec_lower = sys_read32(config->reg_addr + HA1588_RTC_TIME_NSC_L);
	nsec_higher = sys_read32(config->reg_addr + HA1588_RTC_TIME_NSC_H);
	sec_lower = sys_read32(config->reg_addr + HA1588_RTC_TIME_SEC_L);
	sec_higher = sys_read32(config->reg_addr + HA1588_RTC_TIME_SEC_H);

	/* correction 1: round nsec up if fractional */
	nsec_higher += (nsec_lower < 127) ? 1 : 0;

	tval_sec = sec_lower;
	tval_sec |= (uint64_t)sec_higher << 32;

	/* correction 2: nanoseconds overflow seconds */
	if (nsec_higher >= NSEC_PER_SEC) {
		nsec_higher = 0;
		tval_sec++;
	}

	tm->nanosecond = nsec_higher;
	tm->second = tval_sec;

	LOG_DBG("ha1588 read time %" PRIu64 ".%" PRIu32 "!", tval_sec, nsec_higher);

	return 0;
}

static int ptp_clock_ha1588_get(const struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_clock_ha1588_data *data = dev->data;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = ptp_clock_ha1588_get_unlocked(dev, tm);

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ptp_clock_ha1588_adjust(const struct device *dev, int increment)
{
	struct ptp_clock_ha1588_data *data = dev->data;
	struct net_ptp_time tm;
	const int64_t adj_sec = increment / NSEC_PER_SEC;
	const int64_t adj_nsec = increment & NSEC_PER_SEC;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	LOG_DBG("Adjusting ha1588 by %d ns!", increment);

	ret = ptp_clock_ha1588_get(dev, &tm);

	if (ret) {
		goto out;
	}

	tm.nanosecond += adj_nsec;
	tm.second += adj_sec;

	ret = ptp_clock_ha1588_set(dev, &tm);

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ptp_clock_ha1588_rate_adjust(const struct device *dev, double ratio)
{
	const struct ptp_clock_ha1588_config *config = dev->config;
	struct ptp_clock_ha1588_data *data = dev->data;
	double period_adjust = ratio ? 1.0 / ratio : 1.0;
	double new_period = config->rtc_period * period_adjust;
	int ret;

	if (ratio == 1.0) {
		/* nothing needs to be done */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = ptp_clock_ha1588_set_period(dev, (uint64_t)new_period);

	k_spin_unlock(&data->lock, key);

	return ret;
}

#ifdef CONFIG_PTP_CLOCK_HA1588_TSU

static int ptp_tsu_ha1588_get_tstamp_unlocked(const struct device *dev,
					      struct ha1588_tsu_timestamp *tstamp, bool is_rx)
{
	const struct ptp_clock_ha1588_config *config = dev->config;
	const struct ptp_clock_ha1588_data *data = dev->data;
	uint32_t readval;
	uint8_t num_avail;
	/*
	 * tsu_out_0 consists of upper 16 bit of seconds and 16 bits zeros
	 * tsu_out_1 consists of lower 32 bit of seconds
	 * tsu_out_2 consists of 32-bit nanosecond value
	 * tsu_out_3 consists of 4 bit msgid + 12 bit checksum + 16 bit sequence id
	 */
	uint32_t tsu_out_0, tsu_out_1, tsu_out_2, tsu_out_3;
	uint64_t secval;
	bool timestamp_everything_enabled = is_rx ? data->timestamp_everything_enabled_rx
						  : data->timestamp_everything_enabled_tx;

	int ret = 0;

	/*
	 * the value is ready orders of magnitude before the corresponding packet
	 * is processed, so do not try again
	 */
	readval = sys_read32(config->reg_addr +
			     (is_rx ? HA1588_TSU_RXQ_FILTER : HA1588_TSU_TXQ_FILTER));

	num_avail = readval & 0xff;

	__ASSERT(num_avail < 16, "ha1588 cannot store more than 15 packets!");

	if (!num_avail) {
		LOG_ERR("Could not get %s timestamp from ha1588!", is_rx ? "rx" : "tx");
		return -EIO;
	}

	sys_write32(timestamp_everything_enabled ? HA1588_TSU_TIMESTAMP_ALL : HA1588_TSU_RESET_CTRL,
		    config->reg_addr + (is_rx ? HA1588_TSU_RXQ_CTRL : HA1588_TSU_TXQ_CTRL));
	sys_write32(timestamp_everything_enabled ? HA1588_TSU_TIMESTAMP_ALL | HA1588_TSU_READ
						 : HA1588_TSU_READ,
		    config->reg_addr + (is_rx ? HA1588_TSU_RXQ_CTRL : HA1588_TSU_TXQ_CTRL));

	do {
		readval = sys_read32(config->reg_addr +
				     (is_rx ? HA1588_TSU_RXQ_CTRL : HA1588_TSU_TXQ_CTRL));
	} while (!(readval & HA1588_TSU_READ));

	if (is_rx) {
		tsu_out_0 = sys_read32(config->reg_addr + HA1588_TSU_RXQ_OUT_0);
		tsu_out_1 = sys_read32(config->reg_addr + HA1588_TSU_RXQ_OUT_1);
		tsu_out_2 = sys_read32(config->reg_addr + HA1588_TSU_RXQ_OUT_2);
		tsu_out_3 = sys_read32(config->reg_addr + HA1588_TSU_RXQ_OUT_3);
	} else {
		tsu_out_0 = sys_read32(config->reg_addr + HA1588_TSU_TXQ_OUT_0);
		tsu_out_1 = sys_read32(config->reg_addr + HA1588_TSU_TXQ_OUT_1);
		tsu_out_2 = sys_read32(config->reg_addr + HA1588_TSU_TXQ_OUT_2);
		tsu_out_3 = sys_read32(config->reg_addr + HA1588_TSU_TXQ_OUT_3);
	}

	tstamp->ptp_seqid = tsu_out_3 & 0xffff;
	tstamp->ptp_checksum = (tsu_out_3 >> 16) & 0xfff;
	tstamp->ptp_msgid = tsu_out_3 >> 28;

	tstamp->tm.nanosecond = tsu_out_2;
	secval = tsu_out_1;
	secval |= ((uint64_t)tsu_out_0 & 0xffff) << 32;
	tstamp->tm.second = secval;

	LOG_DBG("Got %s timestamp %" PRIu64 ".%" PRIu32 " with sequence ID %" PRIu16
		" checksum %" PRIu16 " message id %" PRIu16,
		is_rx ? "rx" : "tx", secval, tsu_out_2, tstamp->ptp_seqid, tstamp->ptp_checksum,
		tstamp->ptp_msgid);

	LOG_DBG("Got TSU raw values %" PRIx32 " %" PRIx32 " %" PRIx32 " %" PRIx32, tsu_out_3,
		tsu_out_2, tsu_out_1, tsu_out_0);

	return ret;
}

int ptp_tsu_ha1588_get_rx_tstamp(const struct device *dev, struct ha1588_tsu_timestamp *tstamp)
{
	struct ptp_clock_ha1588_data *data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = ptp_tsu_ha1588_get_tstamp_unlocked(dev, tstamp, true);

	k_spin_unlock(&data->lock, key);

	return ret;
}

int ptp_tsu_ha1588_get_tx_tstamp(const struct device *dev, struct ha1588_tsu_timestamp *tstamp)
{
	struct ptp_clock_ha1588_data *data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = ptp_tsu_ha1588_get_tstamp_unlocked(dev, tstamp, false);

	k_spin_unlock(&data->lock, key);

	return ret;
}

int ptp_tsu_ha1588_set_timestamp_all_rx(const struct device *dev, bool enable)
{
	const struct ptp_clock_ha1588_config *config = dev->config;
	struct ptp_clock_ha1588_data *data = dev->data;

	if (config->has_timestamp_everything_option) {
		sys_write32(enable ? HA1588_TSU_TIMESTAMP_ALL : HA1588_TSU_RESET_CTRL,
			    config->reg_addr + HA1588_TSU_RXQ_CTRL);
		data->timestamp_everything_enabled_rx = enable;
		return 0;
	}

	return enable ? -EOPNOTSUPP : 0;
}

int ptp_tsu_ha1588_set_timestamp_all_tx(const struct device *dev, bool enable)
{
	const struct ptp_clock_ha1588_config *config = dev->config;
	struct ptp_clock_ha1588_data *data = dev->data;

	if (config->has_timestamp_everything_option) {
		sys_write32(enable ? HA1588_TSU_TIMESTAMP_ALL : HA1588_TSU_RESET_CTRL,
			    config->reg_addr + HA1588_TSU_TXQ_CTRL);
		data->timestamp_everything_enabled_tx = enable;
		return 0;
	}

	return enable ? -EOPNOTSUPP : 0;
}

bool ptp_tsu_ha1588_packet_matches_rx_filter(struct net_pkt *pkt)
{
	struct net_pkt_cursor backup;
	struct net_eth_hdr eth_hdr;
	struct net_ipv4_hdr ip_hdr;
	struct net_ipv6_hdr ip6_hdr;
	struct net_udp_hdr udp_header;
	uint16_t first_word;

	int ret;

	net_pkt_cursor_backup(pkt, &backup);
	pkt->cursor.buf = pkt->buffer;
	pkt->cursor.pos = pkt->buffer->data;

	ret = net_pkt_read(pkt, &eth_hdr, sizeof(eth_hdr));

	if (ret) {
		LOG_ERR("Could not read Eth header!");
		goto out_false;
	}

	if (ntohs(eth_hdr.type) == NET_ETH_PTYPE_PTP) {
		/* L2 PTP */
		return true;
	}

	if (ntohs(eth_hdr.type) == NET_ETH_PTYPE_VLAN) {
		const struct net_eth_vlan_hdr *vlan = (struct net_eth_vlan_hdr *)net_pkt_data(pkt);
		/* L2 PTP over VLAN */
		return vlan->type == NET_ETH_PTYPE_PTP;
	}

	/* FIXME ha1588 also supports timestamps over MPLS - this is ignored currently */

	/* can only be PTP over UDP */
	if (ntohs(eth_hdr.type) == NET_ETH_PTYPE_IP || ntohs(eth_hdr.type) == NET_ETH_PTYPE_IPV6) {
		bool is_ipv6 = ntohs(eth_hdr.type) == NET_ETH_PTYPE_IPV6;

		if (is_ipv6) {
			/* IPV6 header */
			ret = net_pkt_read(pkt, &ip6_hdr, sizeof(ip6_hdr));

			if (ret) {
				LOG_ERR("Could not read IP header!");
				goto out_false;
			}

			/* ha1588 assumes there are NO optional headers */
			if (ip6_hdr.nexthdr != IPPROTO_UDP) {
				return false;
			}
		} else {
			/* IPv4 header */
			ret = net_pkt_read(pkt, &ip_hdr, sizeof(ip_hdr));

			if (ret) {
				LOG_ERR("Could not read IP header!");
				goto out_false;
			}

			if (ip_hdr.proto != IPPROTO_UDP) {
				return false;
			}
		}

		ret = net_pkt_read(pkt, &udp_header, sizeof(udp_header));

		if (ret) {
			LOG_ERR("Could not read UDP header!");
			goto out_false;
		}

		if (!(ntohs(udp_header.dst_port) == PTP_SOCKET_PORT_EVENT ||
		      ntohs(udp_header.dst_port) == PTP_SOCKET_PORT_GENERAL)) {
			goto out_false;
		}

		/* one final check: message ID in PTP header must match */
		ret = net_pkt_read(pkt, &first_word, sizeof(first_word));

		if (ret) {
			LOG_ERR("Could not read PTP header!");
			goto out_false;
		}

		/*
		 * this is an exact replication of what the parser in ha1588 does;
		 * this prevents difficult to debug problems that arise when zephyr and ha1588
		 * disagree on the layout of PTP messages
		 */
		first_word = htons(first_word);
		ret = 0xf;

		if (ret) {
			goto out_success;
		}
		goto out_false;
	}

out_false:
	net_pkt_cursor_restore(pkt, &backup);
	return false;
out_success:
	net_pkt_cursor_restore(pkt, &backup);
	return true;
}

void ptp_tsu_ha1588_reset(const struct device *dev)
{
	const struct ptp_clock_ha1588_config *config = dev->config;

	sys_write32(HA1588_TSU_RESET, config->reg_addr + HA1588_TSU_RXQ_CTRL);
	sys_write32(HA1588_TSU_RESET, config->reg_addr + HA1588_TSU_TXQ_CTRL);

	sys_write32(HA1588_TSU_RESET_CTRL, config->reg_addr + HA1588_TSU_RXQ_CTRL);
	sys_write32(HA1588_TSU_RESET_CTRL, config->reg_addr + HA1588_TSU_TXQ_CTRL);

	/* filtering by message ID not needed - trigger for ALL PTP messages */
	sys_write32(HA1588_PTP_MSGID_ANY, config->reg_addr + HA1588_TSU_RXQ_FILTER);
	sys_write32(HA1588_PTP_MSGID_ANY, config->reg_addr + HA1588_TSU_TXQ_FILTER);
}

static int ptp_tsu_ha1588_init(const struct device *dev)
{
	const struct ptp_clock_ha1588_config *config = dev->config;

	LOG_DBG("Reset ha1588's FIFOs!");

	if (config->has_timestamp_everything_option) {
		sys_write32(HA1588_TSU_TIMESTAMP_ALL, config->reg_addr + HA1588_TSU_RXQ_CTRL);
	}

	return 0;
}
#else

int ptp_tsu_ha1588_get_tx_tstamp(const struct device *dev, struct ha1588_tsu_timestamp *tstamp)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tstamp);
	return -EIO;
}

int ptp_tsu_ha1588_get_rx_tstamp(const struct device *dev, struct ha1588_tsu_timestamp *tstamp)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tstamp);
	return -EIO;
}

int ptp_tsu_ha1588_set_timestamp_all_tx(const struct device *dev, bool enable)
{
	ARG_UNUSED(dev);
	return enable ? -EOPNOTSUPP : 0;
}

int ptp_tsu_ha1588_set_timestamp_all_rx(const struct device *dev, bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	return enable ? -EOPNOTSUPP : 0;
}

static int ptp_tsu_ha1588_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

bool ptp_tsu_ha1588_packet_matches_rx_filter(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

#endif

static int ptp_clock_ha1588_init(const struct device *dev)
{
	const struct ptp_clock_ha1588_config *config = dev->config;
	int ret;

	sys_write32(HA1588_RTC_RESET_CTRL, config->reg_addr + HA1588_RTC_CTRL);
	sys_write32(HA1588_RTC_SET_RESET, config->reg_addr + HA1588_RTC_CTRL);

	LOG_DBG("ha1588 reset complete!");

	ret = ptp_clock_ha1588_set_period(dev, config->rtc_period);

	return ret ? ret : ptp_tsu_ha1588_init(dev);
}

static const struct ptp_clock_driver_api ptp_clock_ha1588_api = {
	.set = ptp_clock_ha1588_set,
	.get = ptp_clock_ha1588_get,
	.adjust = ptp_clock_ha1588_adjust,
	.rate_adjust = ptp_clock_ha1588_rate_adjust,
};

#define PTP_CLOCK_HA1588_INIT(n)                                                                   \
                                                                                                   \
	static const struct ptp_clock_ha1588_config ptp_clock_ha1588_##n##_config = {              \
		.reg_addr = DT_INST_REG_ADDR(n),                                                   \
		.rtc_period = ((uint64_t)DT_INST_PROP_BY_IDX(n, ha1588_period, 0)) << 32 |         \
			      ((uint64_t)DT_INST_PROP_BY_IDX(n, ha1588_period, 1)),                \
		.has_timestamp_everything_option =                                                 \
			DT_INST_PROP_OR(n, ha1588_ts_all_supported, false)};                       \
                                                                                                   \
	static struct ptp_clock_ha1588_data ptp_clock_ha1588_##n##_data;                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ptp_clock_ha1588_init, NULL, &ptp_clock_ha1588_##n##_data,       \
			      &ptp_clock_ha1588_##n##_config, POST_KERNEL,                         \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &ptp_clock_ha1588_api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_HA1588_INIT)
