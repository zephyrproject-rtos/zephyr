#include <board.h>
#include <device.h>
#include <misc/util.h>
#include <nanokernel.h>
#include <net/ip/net_driver_ethernet.h>

#include "fsl_enet.h"
#include "fsl_phy.h"
#include "fsl_port.h"

#define SYS_LOG_DOMAIN "ETH KSDK"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ETHERNET_LEVEL
#include <misc/sys_log.h>


/* todo: Why does KSDK not export prototypes for these, perhaps this is not
 * an appropriate interface?
 */
extern void ENET_Error_IRQHandler(void);
extern void ENET_Receive_IRQHandler(void);
extern void ENET_Transmit_IRQHandler(void);

/* todo: Can we unify the driver buffers with the uip rx and tx buffers ? */
#define ENET_RX_RING_LEN 2
#define ENET_TX_RING_LEN 2

#if CONFIG_ETH_KSDK_0

typedef void (*eth_config_irq_t)(struct device *port);

struct eth_config {
	enet_handle_t enet_handle;
	uint32_t base_addr;
	eth_config_irq_t config_func;
};

struct eth_runtime {
	struct nano_sem tx_buf_sem;
};

static int eth_net_tx(struct net_buf *buf);

static int eth_tx(struct device *port, struct net_buf *buf)
{
	struct eth_runtime *context = port->driver_data;
	struct eth_config *config = port->config->config_info;
	status_t status;

	nano_sem_take(&context->tx_buf_sem, TICKS_UNLIMITED);

	if (uip_len(buf) > UIP_BUFSIZE) {
		SYS_LOG_ERR("Frame too large to TX: %u\n", uip_len(buf));
		return -1;
	}

	status = ENET_SendFrame(ENET, &config->enet_handle, uip_buf(buf),
				uip_len(buf));
	if (status) {
		SYS_LOG_ERR("ENET_SendFrame error: %d\n", status);
		return 0;
	}
	return 1;
}

/* todo: This is device specific setup, it doesn't belong here, it
 * needs to move out to somewhere device specific, but where ?
 *
 * todo: This setup code needs scrutiny by someone familiar with K64F.
 */

static void
k64f_init_eth_hardware(void)
{
	port_pin_config_t config;

	memset(&config, 0, sizeof(config));

	PORT_SetPinMux(PORTA, 5u, kPORT_MuxAlt4);  /* RMII0_RXER/MII0_RXER */
	PORT_SetPinMux(PORTA, 12u, kPORT_MuxAlt4); /* RMII0_RXD1/MII0_RXD1 */
	PORT_SetPinMux(PORTA, 13u, kPORT_MuxAlt4); /* RMII0_RXD0/MII0_RXD0 */
	PORT_SetPinMux(PORTA, 14u, kPORT_MuxAlt4); /* RMII0_CRS_DV/MII0_RXDV */
	PORT_SetPinMux(PORTA, 15u, kPORT_MuxAlt4); /* RMII0_TXEN/MII0_TXEN */
	PORT_SetPinMux(PORTA, 16u, kPORT_MuxAlt4); /* RMII0_TXD0/MII0_TXD0 */
	PORT_SetPinMux(PORTA, 17u, kPORT_MuxAlt4); /* RMII0_TXD1/MII0_TXD1 */
	PORT_SetPinMux(PORTA, 28u, kPORT_MuxAlt4); /* MII0_TXER */
	CLOCK_EnableClock(kCLOCK_PortA);

	config.openDrainEnable = kPORT_OpenDrainEnable;
	config.mux = kPORT_MuxAlt4;
	config.pullSelect = kPORT_PullUp;
	PORT_SetPinConfig(PORTB, 0u, &config); /* RMII0_MDIO/MII0_MDIO */
	PORT_SetPinMux(PORTB, 1u, kPORT_MuxAlt4); /* RMII0_MDC/MII0_MDC */
	CLOCK_EnableClock(kCLOCK_PortB); /* do we need this? */

	PORT_SetPinMux(PORTC, 16u, kPORT_MuxAlt4); /* ENET0_1588_TMR0 */
	PORT_SetPinMux(PORTC, 17u, kPORT_MuxAlt4); /* ENET0_1588_TMR1 */
	PORT_SetPinMux(PORTC, 18u, kPORT_MuxAlt4); /* ENET0_1588_TMR2 */
	PORT_SetPinMux(PORTC, 19u, kPORT_MuxAlt4); /* ENET0_1588_TMR3 */
	CLOCK_EnableClock(kCLOCK_PortC);	   /* do we need this? */

	CLOCK_SetEnetTime0Clock(0x2);
}

static enet_rx_bd_struct_t __aligned(ENET_BUFF_ALIGNMENT)
rx_buffer_desc[ENET_RX_RING_LEN];

static enet_tx_bd_struct_t __aligned(ENET_BUFF_ALIGNMENT)
tx_buffer_desc[ENET_TX_RING_LEN];

static uint8_t __aligned(ENET_BUFF_ALIGNMENT) rx_buffer[ENET_RX_RING_LEN][2048];
static uint8_t __aligned(ENET_BUFF_ALIGNMENT) tx_buffer[ENET_TX_RING_LEN][2048];


static void eth_rx(struct device *port)
{
	struct eth_config *config = port->config->config_info;
	struct net_buf *buf;
	uint32_t frame_length = 0;
	status_t status;

	status = ENET_GetRxFrameSize(&config->enet_handle, &frame_length);
	if (status) {
		enet_data_error_stats_t error_stats;

		SYS_LOG_ERR("ENET_GetRxFrameSize return: %d", status);

		ENET_GetRxErrBeforeReadFrame(&config->enet_handle,
					     &error_stats);
		/* todo: error_stats need reporting somehere? */

		/* Flush the current read buffer.  */
		status = ENET_ReadFrame(ENET, &config->enet_handle, NULL, 0);
		if (status) {
			SYS_LOG_ERR("ENET_GetRxFrameSize return: %d\n", status);
		}
		return;
	}

	/* todo: We should be able to reduced this to an __ASSERT
	 * provided the size of the buffers we allocate are hardwired
	 * to an appropriate UIP constant.
	 */
	if (frame_length > UIP_BUFSIZE) {
		SYS_LOG_ERR("frame too large: %u.", frame_length);
		/* Flush the current read buffer.  */
		ENET_ReadFrame(ENET, &config->enet_handle, NULL, 0);
		return;
	}

	buf = ip_buf_get_reserve_rx(0);
	if (buf == NULL) {
		/* We failed to get a receive buffer.  Flush the
		 * current read buffer.  We don't add any further
		 * logging here because the allocator issued a
		 * diagnostic when it failed to allocate.
		 */
		ENET_ReadFrame(ENET, &config->enet_handle, NULL, 0);
		return;
	}

	status = ENET_ReadFrame(ENET, &config->enet_handle,
				net_buf_add(buf, frame_length), frame_length);
	if (status) {
		SYS_LOG_ERR("ENET_ReadFrame failed: %d\n", status);
		return;
	}

	uip_len(buf) = frame_length;
	net_driver_ethernet_recv(buf);
}

void eth_callback(ENET_Type *base, enet_handle_t *handle, enet_event_t event,
		  void *param)
{
	struct device *port = param;
	struct eth_runtime *context = port->driver_data;

	switch (event) {
	case kENET_RxEvent:
		eth_rx(port);
		break;
	case kENET_TxEvent:
		/* Free the TX buffer. */
		nano_sem_give(&context->tx_buf_sem);
		break;
	case kENET_ErrEvent:
		/* Error event: BABR/BABT/EBERR/LC/RL/UN/PLR.  */

		/* todo: What should we do on this event? */

		break;
	case kENET_WakeUpEvent:
		/* Wake up from sleep mode event. */

		/* todo: What should we do on this event? */

		break;
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
	case kENET_TimeStampEvent:
		/* Time stamp event.  */

		/* todo: What should we do on this event? */

		break;
	case kENET_TimeStampAvailEvent:
		/* Time stamp available event.  */

		/* todo: What should we do on this event? */

		break;
#endif
	}
}

#if defined(CONFIG_ETH_KSDK_0_RANDOM_MAC)
static void generate_mac(uint8_t *mac_addr)
{
	uint32_t entropy;

	entropy = sys_rand32_get();

	mac_addr[0] = 0x02;	/* Locally administered, unicast. */
	mac_addr[0] |= ((entropy >> 0) & 0xfe);
	mac_addr[1] = entropy >> 8;
	mac_addr[2] = entropy >> 16;
	mac_addr[3] = entropy >> 24;

	entropy = sys_rand32_get();

	mac_addr[4] = entropy >> 0;
	mac_addr[5] = entropy >> 8;
}
#endif

static int eth_initialize(struct device *port)
{
	struct eth_runtime *context = port->driver_data;
	struct eth_config *config = port->config->config_info;
	enet_config_t enet_config;
	uint32_t sys_clock;
	const uint32_t phy_addr = 0;
	bool link;
	status_t status;
	uint8_t mac_addr[6];
	int result;
	int i;
	enet_buffer_config_t buffer_config = {
		ENET_RX_RING_LEN,
		ENET_TX_RING_LEN,
		/* todo: Hardwired buffer sizes need cleaning up ? */
		ROUND_UP(1522, ENET_BUFF_ALIGNMENT),
		ROUND_UP(1522, ENET_BUFF_ALIGNMENT),
		rx_buffer_desc,
		tx_buffer_desc,
		rx_buffer[0],
		tx_buffer[0],
	};

	nano_sem_init(&context->tx_buf_sem);
	for (i = 0; i < ENET_TX_RING_LEN; i++) {
		nano_sem_give(&context->tx_buf_sem);
	}

	k64f_init_eth_hardware();
	sys_clock = CLOCK_GetFreq(kCLOCK_CoreSysClk);

	ENET_GetDefaultConfig(&enet_config);
	enet_config.interrupt |= kENET_RxFrameInterrupt;
	enet_config.interrupt |= kENET_TxFrameInterrupt;

	status = PHY_Init(ENET, 0, sys_clock);
	if (status) {
		SYS_LOG_ERR("PHY_Init() failed: %d", status);
		return 1;
	}

	/* todo: This PHY interaction is polled and *slow*, it blocks
	 * the init thread for an unpleasant amount of time.  Maybe
	 * this should sit in its own thread, or perhaps there is an
	 * async mechanism to drive the PHY?
	 *
	 * todo: We should also handle arbitrary link up / down dynamically etc ?
	 * todo: Investigate if there is plumbing to allow dhcp clients etc to detect link up/down.
	 */
	PHY_GetLinkStatus(ENET, phy_addr, &link);
	if (link) {
		phy_speed_t phy_speed;
		phy_duplex_t phy_duplex;

		PHY_GetLinkSpeedDuplex(ENET, phy_addr, &phy_speed, &phy_duplex);
		enet_config.miiSpeed = (enet_mii_speed_t) phy_speed;
		enet_config.miiDuplex = (enet_mii_duplex_t) phy_duplex;

		SYS_LOG_INF("Enabled %dM %s-duplex mode.",
			    (phy_speed ? 100 : 10),
			    (phy_duplex ? "full" : "half"));
	}

#if defined(CONFIG_ETH_KSDK_0_RANDOM_MAC)
	/* ? convenient (for the author), but should be abstracted
	 * across drivers, or maybe random MAC generation is not
	 * appropriate at all ?
	 */
	generate_mac(mac_addr);
#else
	mac_addr[0] = CONFIG_ETH_KSDK_0_MAC0;
	mac_addr[1] = CONFIG_ETH_KSDK_0_MAC1;
	mac_addr[2] = CONFIG_ETH_KSDK_0_MAC2;
	mac_addr[3] = CONFIG_ETH_KSDK_0_MAC3;
	mac_addr[4] = CONFIG_ETH_KSDK_0_MAC4;
	mac_addr[5] = CONFIG_ETH_KSDK_0_MAC5;
#endif

	ENET_Init(ENET,
		  &config->enet_handle,
		  &enet_config,
		  &buffer_config,
		  mac_addr,
		  sys_clock);

	SYS_LOG_DBG("MAC %2x:%2x:%2x:%2x:%2x:%2x",
		    mac_addr[0], mac_addr[1],
		    mac_addr[2], mac_addr[3],
		    mac_addr[4], mac_addr[5]);


	result = net_set_mac(mac_addr, sizeof(mac_addr));
	if (result) {
		return 1;
	}

	ENET_SetCallback(&config->enet_handle, eth_callback, port);
	net_driver_ethernet_register_tx(eth_net_tx);
	config->config_func(port);
	ENET_ActiveRead(ENET);
	return 0;
}

static void eth_ksdk_rx_isr(void *port)
{
	ENET_Receive_IRQHandler();
}

static void eth_ksdk_tx_isr(void *port)
{
	ENET_Transmit_IRQHandler();
}

static void eth_ksdk_error_isr(void *port)
{
	ENET_Error_IRQHandler();
}

static void eth_config_0_irq(struct device *);

static struct eth_config eth_config_0 = {
	.config_func = eth_config_0_irq,
};

static struct eth_runtime eth_0_runtime;

DEVICE_INIT(eth_ksdk_0, CONFIG_ETH_KSDK_0_NAME, eth_initialize,
	    &eth_0_runtime, &eth_config_0,
	    NANOKERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static int eth_net_tx(struct net_buf *buf)
{
	return eth_tx(DEVICE_GET(eth_ksdk_0), buf);
}

static void eth_config_0_irq(struct device *port)
{
	IRQ_CONNECT(IRQ_ETH_RX, CONFIG_ETH_KSDK_0_IRQ_PRI, eth_ksdk_rx_isr,
		    DEVICE_GET(eth_ksdk_0), 0);
	irq_enable(IRQ_ETH_RX);

	IRQ_CONNECT(IRQ_ETH_TX, CONFIG_ETH_KSDK_0_IRQ_PRI, eth_ksdk_tx_isr,
		    DEVICE_GET(eth_ksdk_0), 0);
	irq_enable(IRQ_ETH_TX);

	IRQ_CONNECT(IRQ_ETH_ERR_MISC, CONFIG_ETH_KSDK_0_IRQ_PRI,
		    eth_ksdk_error_isr,
		    DEVICE_GET(eth_ksdk_0), 0);
	irq_enable(IRQ_ETH_ERR_MISC);
}
#endif
