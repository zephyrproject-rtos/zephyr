/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_ethernet

/* Renesas RA ethernet driver */

#define LOG_MODULE_NAME eth_ra_ethernet
#define LOG_LEVEL       CONFIG_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#include <soc.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/phy.h>
#include "r_ether.h"
#include "r_ether_phy.h"

/* Additional configurations to use with hal_renesas */
#define ETHER_DEFAULT          NULL
#define ETHER_CHANNEL0         0
#define ETHER_BUF_SIZE         1536
#define ETHER_PADDING_OFFSET   1
#define ETHER_BROADCAST_FILTER 0
#define ETHER_TOTAL_BUF_NUM    (CONFIG_ETH_RENESAS_TX_BUF_NUM + CONFIG_ETH_RENESAS_RX_BUF_NUM)
#define ETHER_EE_RECEIVE_EVENT_MASK                                                                \
	(ETHER_EESR_EVENT_MASK_RFOF | ETHER_EESR_EVENT_MASK_RDE | ETHER_EESR_EVENT_MASK_FR |       \
	 ETHER_EESR_EVENT_MASK_RFCOF)

BUILD_ASSERT(DT_INST_ENUM_IDX(0, phy_connection_type) <= 1, "Invalid PHY connection setting");

void ether_eint_isr(void);
void renesas_ra_eth_callback(ether_callback_args_t *p_args);
static void renesas_ra_eth_buffer_init(const struct device *dev);

extern void ether_init_buffers(ether_instance_ctrl_t *const p_instance_ctrl);
extern void ether_configure_mac(ether_instance_ctrl_t *const p_instance_ctrl,
				const uint8_t mac_addr[], const uint8_t mode);
extern void ether_do_link(ether_instance_ctrl_t *const p_instance_ctrl, const uint8_t mode);

uint8_t g_ether0_mac_address[6] = DT_INST_PROP(0, local_mac_address);
struct renesas_ra_eth_context {
	struct net_if *iface;
	uint8_t mac[6];

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ETH_RA_RX_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem rx_sem;
	ether_instance_ctrl_t ctrl;
	/** pinctrl configs */
	const struct pinctrl_dev_config *pcfg;
};

struct renesas_ra_eth_config {
	const ether_cfg_t *p_cfg;
	const struct device *phy_dev;
};

/*
 * In some Renesas SoCs, Ethernet peripheral is always a non-secure bus master.
 * In that case, placing the Ethernet buffer in non-secure RAM is necessary.
 */
#define ETHER_BUFFER_ALIGN(s) static __aligned(s)
#if defined(CONFIG_ETH_RENESAS_RA_USE_NS_BUF)
#define ETHER_BUFFER_PLACE_IN_SECTION __attribute__((section(".ns_buffer.eth")))
#else
#define ETHER_BUFFER_PLACE_IN_SECTION
#endif

#define DECLARE_ETHER_RX_BUFFER(idx, _)                                                            \
	ETHER_BUFFER_ALIGN(32)                                                                     \
	uint8_t g_ether0_ether_rx_buffer##idx[ETHER_BUF_SIZE] ETHER_BUFFER_PLACE_IN_SECTION;

#define DECLARE_ETHER_TX_BUFFER(idx, _)                                                            \
	ETHER_BUFFER_ALIGN(32)                                                                     \
	uint8_t g_ether0_ether_tx_buffer##idx[ETHER_BUF_SIZE] ETHER_BUFFER_PLACE_IN_SECTION;

#define DECLARE_ETHER_RX_BUFFER_PTR(idx, _) (uint8_t *)&g_ether0_ether_rx_buffer##idx[0]

#define DECLARE_ETHER_TX_BUFFER_PTR(idx, _) (uint8_t *)&g_ether0_ether_tx_buffer##idx[0]

LISTIFY(CONFIG_ETH_RENESAS_RX_BUF_NUM, DECLARE_ETHER_RX_BUFFER, (;));
LISTIFY(CONFIG_ETH_RENESAS_TX_BUF_NUM, DECLARE_ETHER_TX_BUFFER, (;));

uint8_t *pp_g_ether0_ether_buffers[ETHER_TOTAL_BUF_NUM] = {
	LISTIFY(CONFIG_ETH_RENESAS_RX_BUF_NUM, DECLARE_ETHER_RX_BUFFER_PTR, (,)),
		LISTIFY(CONFIG_ETH_RENESAS_TX_BUF_NUM, DECLARE_ETHER_TX_BUFFER_PTR, (,)) };

ETHER_BUFFER_ALIGN(16)
ether_instance_descriptor_t
	g_ether0_tx_descriptors[CONFIG_ETH_RENESAS_TX_BUF_NUM] ETHER_BUFFER_PLACE_IN_SECTION;
ETHER_BUFFER_ALIGN(16)
ether_instance_descriptor_t
	g_ether0_rx_descriptors[CONFIG_ETH_RENESAS_RX_BUF_NUM] ETHER_BUFFER_PLACE_IN_SECTION;

const ether_extended_cfg_t g_ether0_extended_cfg_t = {
	.p_rx_descriptors = g_ether0_rx_descriptors,
	.p_tx_descriptors = g_ether0_tx_descriptors,
	.eesr_event_filter = ETHER_EE_RECEIVE_EVENT_MASK,
};

/* Dummy configuration for ether phy as hal layer require */
const ether_phy_extended_cfg_t g_ether_phy0_extended_cfg = {
	.p_target_init = ETHER_DEFAULT, .p_target_link_partner_ability_get = ETHER_DEFAULT};

const ether_phy_cfg_t g_ether_phy0_cfg;

ether_phy_instance_ctrl_t g_ether_phy0_ctrl;

const ether_phy_instance_t g_ether_phy0 = {.p_ctrl = &g_ether_phy0_ctrl,
					   .p_cfg = &g_ether_phy0_cfg,
					   .p_api = &g_ether_phy_on_ether_phy};

const ether_cfg_t g_ether0_cfg = {
	.channel = ETHER_CHANNEL0,
	.zerocopy = ETHER_ZEROCOPY_DISABLE,
	.multicast = ETHER_MULTICAST_ENABLE,
	.promiscuous = ETHER_PROMISCUOUS_DISABLE,
	.flow_control = ETHER_FLOW_CONTROL_DISABLE,
	.padding = ETHER_PADDING_DISABLE,
	.padding_offset = ETHER_PADDING_OFFSET,
	.broadcast_filter = ETHER_BROADCAST_FILTER,
	.p_mac_address = g_ether0_mac_address,
	.num_tx_descriptors = CONFIG_ETH_RENESAS_TX_BUF_NUM,
	.num_rx_descriptors = CONFIG_ETH_RENESAS_RX_BUF_NUM,
	.pp_ether_buffers = pp_g_ether0_ether_buffers,
	.ether_buffer_size = ETHER_BUF_SIZE,
	.irq = DT_INST_IRQN(0),
	.interrupt_priority = DT_INST_IRQ(0, priority),
	.p_callback = ETHER_DEFAULT,
	.p_ether_phy_instance = &g_ether_phy0,
	.p_context = ETHER_DEFAULT,
	.p_extend = &g_ether0_extended_cfg_t,
};

static struct renesas_ra_eth_config eth_0_config = {
	.p_cfg = &g_ether0_cfg, .phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle))};

/* Driver functions */
static enum ethernet_hw_caps renesas_ra_eth_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
}

void renesas_ra_eth_callback(ether_callback_args_t *p_args)
{
	struct device *dev = (struct device *)p_args->p_context;
	struct renesas_ra_eth_context *ctx = dev->data;

	if (p_args->event == ETHER_EVENT_RX_COMPLETE) {
		k_sem_give(&ctx->rx_sem);
	}
}

static void renesas_ra_eth_buffer_init(const struct device *dev)
{
	struct renesas_ra_eth_context *ctx = dev->data;
	ether_extended_cfg_t *p_ether_extended_cfg =
		(ether_extended_cfg_t *)ctx->ctrl.p_ether_cfg->p_extend;
	/* Initialize the transmit and receive descriptor */
	memset(p_ether_extended_cfg->p_rx_descriptors, 0x00,
	       sizeof(ether_instance_descriptor_t) * ctx->ctrl.p_ether_cfg->num_rx_descriptors);
	memset(p_ether_extended_cfg->p_tx_descriptors, 0x00,
	       sizeof(ether_instance_descriptor_t) * ctx->ctrl.p_ether_cfg->num_tx_descriptors);

	ether_init_buffers(&ctx->ctrl);
}

static void phy_link_state_changed(const struct device *pdev, struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (struct device *)user_data;
	struct renesas_ra_eth_context *ctx = dev->data;
	R_ETHERC0_Type *p_reg_etherc;

	p_reg_etherc = (R_ETHERC0_Type *)ctx->ctrl.p_reg_etherc;

	ARG_UNUSED(pdev);

	if (state->is_up) {

		ctx->ctrl.link_change = ETHER_LINK_CHANGE_LINK_UP;
		ctx->ctrl.previous_link_status = ETHER_PREVIOUS_LINK_STATUS_UP;

		renesas_ra_eth_buffer_init(dev);

		/*
		 * ETHERC and EDMAC are set after ETHERC and EDMAC are reset in software
		 * and sending and receiving is permitted.
		 */
		ether_configure_mac(&ctx->ctrl, ctx->ctrl.p_ether_cfg->p_mac_address, 0);

		switch (state->speed) {
		/* Half duplex link */
		case LINK_HALF_100BASE: {
			ctx->ctrl.link_speed_duplex = ETHER_PHY_LINK_SPEED_100H;
			break;
		}

		case LINK_HALF_10BASE: {
			ctx->ctrl.link_speed_duplex = ETHER_PHY_LINK_SPEED_10H;
			break;
		}

		/* Full duplex link */
		case LINK_FULL_100BASE: {
			ctx->ctrl.link_speed_duplex = ETHER_PHY_LINK_SPEED_100F;
			break;
		}

		case LINK_FULL_10BASE: {
			ctx->ctrl.link_speed_duplex = ETHER_PHY_LINK_SPEED_10F;
			break;
		}

		default: {
			ctx->ctrl.link_speed_duplex = ETHER_PHY_LINK_SPEED_100F;
			break;
		}
		}

		ether_do_link(&ctx->ctrl, 0);
		ctx->ctrl.link_change = ETHER_LINK_CHANGE_LINK_UP;
		ctx->ctrl.previous_link_status = ETHER_PREVIOUS_LINK_STATUS_UP;
		ctx->ctrl.link_establish_status = ETHER_LINK_ESTABLISH_STATUS_UP;
		LOG_DBG("Link up");
		net_eth_carrier_on(ctx->iface);
	} else {
		ctx->ctrl.link_change = ETHER_LINK_CHANGE_LINK_DOWN;
		ctx->ctrl.previous_link_status = ETHER_PREVIOUS_LINK_STATUS_DOWN;
		ctx->ctrl.link_establish_status = ETHER_LINK_ESTABLISH_STATUS_DOWN;
		net_eth_carrier_off(ctx->iface);
	}
}

static void renesas_ra_eth_initialize(struct net_if *iface)
{
	fsp_err_t err;
	const struct device *dev = net_if_get_device(iface);
	struct renesas_ra_eth_context *ctx = dev->data;
	const struct renesas_ra_eth_config *cfg = dev->config;

	LOG_DBG("eth_initialize");

	net_if_set_link_addr(iface, ctx->mac, sizeof(ctx->mac), NET_LINK_ETHERNET);

	if (ctx->iface == NULL) {
		ctx->iface = iface;
	}

	ethernet_init(iface);

	err = R_ETHER_Open(&ctx->ctrl, cfg->p_cfg);

	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to init ether - R_ETHER_Open fail");
	}

	err = R_ETHER_CallbackSet(&ctx->ctrl, renesas_ra_eth_callback, (void *const)dev, NULL);

	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to init ether - R_ETHER_CallbackSet fail");
	}

	phy_link_callback_set(cfg->phy_dev, &phy_link_state_changed, (void *)dev);
	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(ctx->iface);
}

static int renesas_ra_eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	fsp_err_t err = FSP_SUCCESS;
	struct renesas_ra_eth_context *ctx = dev->data;
	uint16_t len = net_pkt_get_len(pkt);
	static uint8_t tx_buf[NET_ETH_MAX_FRAME_SIZE];

	if (net_pkt_read(pkt, tx_buf, len)) {
		goto error;
	}

	/* Check if packet length is less than minimum Ethernet frame size */
	if (len < NET_ETH_MINIMAL_FRAME_SIZE) {
		/* Add padding to meet the minimum frame size */
		memset(tx_buf + len, 0, NET_ETH_MINIMAL_FRAME_SIZE - len);
		len = NET_ETH_MINIMAL_FRAME_SIZE;
	}

	err = R_ETHER_Write(&ctx->ctrl, tx_buf, len);
	if (err != FSP_SUCCESS) {
		goto error;
	}

	return 0;

error:
	LOG_ERR("Writing to FIFO failed");
	return -1;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init = renesas_ra_eth_initialize,
	.get_capabilities = renesas_ra_eth_get_capabilities,
	.send = renesas_ra_eth_tx,
};

static void renesas_ra_eth_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	ether_eint_isr();
}

static struct net_pkt *renesas_ra_eth_rx(const struct device *dev)
{
	fsp_err_t err = FSP_SUCCESS;
	struct renesas_ra_eth_context *ctx;
	struct net_pkt *pkt = NULL;
	uint32_t len = 0;
	static uint8_t rx_buf[NET_ETH_MAX_FRAME_SIZE];

	__ASSERT_NO_MSG(dev != NULL);
	ctx = dev->data;
	__ASSERT_NO_MSG(ctx != NULL);

	err = R_ETHER_Read(&ctx->ctrl, rx_buf, &len);
	if ((err != FSP_SUCCESS) && (err != FSP_ERR_ETHER_ERROR_NO_DATA)) {
		LOG_ERR("Failed to read packets");
		goto out;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto out;
	}

	if (net_pkt_write(pkt, rx_buf, len)) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		pkt = NULL;
		goto out;
	}

out:
	if (pkt == NULL) {
		eth_stats_update_errors_rx(ctx->iface);
	}

	return pkt;
}

static void renesas_ra_eth_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct net_if *iface;
	int res;
	struct net_pkt *pkt = NULL;
	struct renesas_ra_eth_context *ctx = dev->data;

	while (true) {
		res = k_sem_take(&ctx->rx_sem, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
		if (res == 0) {
			pkt = renesas_ra_eth_rx(dev);

			if (pkt != NULL) {
				iface = net_pkt_iface(pkt);
				res = net_recv_data(iface, pkt);
				if (res < 0) {
					net_pkt_unref(pkt);
				}
			}
		}
	}
}

#define EVENT_EDMAC_EINT(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_EDMAC, channel, _EINT))

/* Bindings to the platform */
int renesas_ra_eth_init(const struct device *dev)
{
	struct renesas_ra_eth_context *ctx = dev->data;

	switch (DT_INST_ENUM_IDX(0, phy_connection_type)) {
	case 0: /* mii */
		R_PMISC->PFENET = (uint8_t)(0x1 << R_PMISC_PFENET_PHYMODE0_Pos);
		break;
	case 1: /* rmii */
		R_PMISC->PFENET = (uint8_t)(0x0 << R_PMISC_PFENET_PHYMODE0_Pos);
		break;
	default:
		/* Build assert at top of file should catch this case */
		LOG_ERR("Failed to init Ethernet driver - phy-connection-type not support");

		return -EINVAL;
	}

	R_ICU->IELSR[DT_INST_IRQN(0)] = EVENT_EDMAC_EINT(0);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), renesas_ra_eth_isr,
		    DEVICE_DT_INST_GET(0), 0);

	k_thread_create(&ctx->thread, ctx->thread_stack, CONFIG_ETH_RA_RX_THREAD_STACK_SIZE,
			renesas_ra_eth_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_RA_RX_THREAD_PRIORITY), 0, K_NO_WAIT);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

#define ETHER_RA_INIT(idx)                                                                         \
	PINCTRL_DT_INST_DEFINE(0);                                                                 \
	static struct renesas_ra_eth_context eth_0_context = {                                     \
		.mac = DT_INST_PROP(0, local_mac_address),                                         \
		.rx_sem = Z_SEM_INITIALIZER(eth_0_context.rx_sem, 0, UINT_MAX),                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),                                         \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(0, renesas_ra_eth_init, NULL, &eth_0_context, &eth_0_config, \
				      CONFIG_ETH_INIT_PRIORITY, &api_funcs, NET_ETH_MTU /*MTU*/);

DT_INST_FOREACH_STATUS_OKAY(ETHER_RA_INIT);
