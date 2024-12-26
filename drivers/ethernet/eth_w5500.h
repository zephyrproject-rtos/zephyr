/* W5500 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _W5500_
#define _W5500_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/ethernet.h>

#define W5500_MAX_SOCK_NUM   8
#define W5500_SOCK_PORT_BASE 50000

/*
 * W5500 common registers
 */
#define W5500_COMMON_REGS 0x0000
#define W5500_MR          0x0000 /* Mode Register */
#define W5500_SHAR        0x0009 /* Source MAC address */
#define W5500_SIPR        0x000F /* Source IP address */
#define W5500_SUBR        0x0005 /* Source subnet mask */
#define W5500_GAR         0x0001 /* Source gateway address */
#define W5500_IR          0x0015 /* Interrupt Register */
#define W5500_PHYCFGR     0x002E /* PHY Configuration register */
#define W5500_SIR         0x0018 /* Socket Interrupt Register */
#define W5500_SIMR        0x0018 /* Socket Interrupt Mask Register */
#define W5500_RTR         0x0019 /* Retry Time-value Register */

#define W5500_RTR_DEFAULT 2000

/*
 * W5500 socket registers
 */

#define W5500_Sn_SREGS(N) ((1 + 4 * N) << 16)

#define W5500_Sn_MR(N)     (0x0000 + W5500_Sn_SREGS(N)) /* Sn Mode Register */
#define W5500_Sn_CR(N)     (0x0001 + W5500_Sn_SREGS(N)) /* Sn Command Register */
#define W5500_Sn_IR(N)     (0x0002 + W5500_Sn_SREGS(N)) /* Sn Interrupt Register */
#define W5500_Sn_SR(N)     (0x0003 + W5500_Sn_SREGS(N)) /* Sn Status Register */
#define W5500_Sn_PORT(N)   (0x0004 + W5500_Sn_SREGS(N)) /* Sn Source Port Register */
#define W5500_Sn_DIPR(N)   (0x000C + W5500_Sn_SREGS(N)) /* Sn Peer IP Register */
#define W5500_Sn_DPORT(N)  (0x0010 + W5500_Sn_SREGS(N)) /* Sn Peer IP Register */
#define W5500_Sn_TX_FSR(N) (0x0020 + W5500_Sn_SREGS(N)) /* Sn Transmit free memory size */
#define W5500_Sn_TX_RD(N)  (0x0022 + W5500_Sn_SREGS(N)) /* Sn Transmit memory read pointer */
#define W5500_Sn_TX_WR(N)  (0x0024 + W5500_Sn_SREGS(N)) /* Sn Transmit memory write pointer */
#define W5500_Sn_RX_RSR(N) (0x0026 + W5500_Sn_SREGS(N)) /* Sn Receive free memory size */
#define W5500_Sn_RX_RD(N)  (0x0028 + W5500_Sn_SREGS(N)) /* Sn Receive memory read pointer */
#define W5500_Sn_IMR(N)    (0x002C + W5500_Sn_SREGS(N)) /* Sn Interrupt Mask Register */

#define W5500_Sn_TXBUFS(N)     ((2 + 4 * N) << 16)
#define W5500_Sn_RXBUFS(N)     ((3 + 4 * N) << 16)
#define W5500_Sn_RXMEM_SIZE(N) (0x001E + W5500_Sn_SREGS(N)) /* Sn RX Memory Size */
#define W5500_Sn_TXMEM_SIZE(N) (0x001F + W5500_Sn_SREGS(N)) /* Sn TX Memory Size */

#define W5500_TX_MEM_SIZE 0x04000
#define W5500_RX_MEM_SIZE 0x04000

/* MR values */
#define W5500_MR_RST 0x80 /* S/W reset */
#define W5500_MR_PB  0x10 /* Ping block */
#define W5500_MR_AI  0x02 /* Address Auto-Increment */
#define W5500_MR_IND 0x01 /* Indirect mode */

/* Sn_MR values */
#define W5500_Sn_MR_MULTI  0x80
#define W5500_Sn_MR_BCASTB 0x40
#define W5500_Sn_MR_ND     0x20
#define W5500_Sn_MR_UCASTB 0x10
#define W5500_Sn_MR_MACRAW 0x04
#define W5500_Sn_MR_IPRAW  0x03
#define W5500_Sn_MR_UDP    0x02
#define W5500_Sn_MR_TCP    0x01
#define W5500_Sn_MR_CLOSE  0x00
#define W5500_Sn_MR_MFEN   W5500_Sn_MR_MULTI
#define W5500_Sn_MR_MMB    W5500_Sn_MR_ND
#define W5500_Sn_MR_MIP6B  W5500_Sn_MR_UCASTB
#define W5500_Sn_MR_MC     W5500_Sn_MR_ND
#define W5500_SOCK_STREAM  W5500_Sn_MR_TCP
#define W5500_SOCK_DGRAM   W5500_Sn_MR_UDP

/* Sn_CR values */
#define W5500_Sn_CR_OPEN      0x01
#define W5500_Sn_CR_LISTEN    0x02
#define W5500_Sn_CR_CONNECT   0x04
#define W5500_Sn_CR_DISCON    0x08
#define W5500_Sn_CR_CLOSE     0x10
#define W5500_Sn_CR_SEND      0x20
#define W5500_Sn_CR_SEND_MAC  0x21
#define W5500_Sn_CR_SEND_KEEP 0x22
#define W5500_Sn_CR_RECV      0x40

/* Sn_IR values */
#define W5500_Sn_IR_SENDOK  0x10
#define W5500_Sn_IR_TIMEOUT 0x08
#define W5500_Sn_IR_RECV    0x04
#define W5500_Sn_IR_DISCON  0x02
#define W5500_Sn_IR_CON     0x01

/* Sn_SR values */
#define W5500_SOCK_CLOSED      0x00
#define W5500_SOCK_INIT        0x13
#define W5500_SOCK_LISTEN      0x14
#define W5500_SOCK_SYNSENT     0x15
#define W5500_SOCK_SYNRECV     0x16
#define W5500_SOCK_ESTABLISHED 0x17
#define W5500_SOCK_FIN_WAIT    0x18
#define W5500_SOCK_CLOSING     0x1A
#define W5500_SOCK_TIME_WAIT   0x1B
#define W5500_SOCK_CLOSE_WAIT  0x1C
#define W5500_SOCK_LAST_ACK    0x1D
#define W5500_SOCK_UDP         0x22
#define W5500_SOCK_IPRAW       0x32
#define W5500_SOCK_MACRAW      0x42

/* Delay for PHY write/read operations (25.6 us) */
#define W5500_PHY_ACCESS_DELAY 26U

enum w5500_transport_type {
	W5500_TRANSPORT_UNSPECIFIED,
	W5500_TRANSPORT_MACRAW,
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	W5500_TRANSPORT_TCP,
	W5500_TRANSPORT_UDP
#endif
};

enum w5500_socket_state {
	W5500_SOCKET_STATE_CLOSED,

	/* This socket has been assigned a fd */
	W5500_SOCKET_STATE_ASSIGNED,

	/* This socket is open on w5500 */
	W5500_SOCKET_STATE_OPEN,

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	/* connect() issues */
	W5500_SOCKET_STATE_CONNECTING,

	/* For TCP: Socket has been initialized, and connection had established */
	W5500_SOCKET_STATE_ESTABLISHED,

	/* This socket is listening for incoming connections */
	W5500_SOCKET_STATE_LISTENING
#endif
};

struct w5500_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
	int32_t timeout;
};

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
struct w5500_socket_listening_context {
	bool in_use;
	bool listening_sock_nonblock;
	uint8_t backlog;
	uint8_t listening_socknum;
	uint8_t backlog_socknum_bitmask;
	uint8_t accepted_socknum_bitmask;
	struct k_sem incoming_sem;
};
#endif

struct w5500_socket {
	enum w5500_transport_type type;
	enum w5500_socket_state state;
	uint8_t tx_buf_size;
	uint8_t rx_buf_size;
	struct k_sem sint_sem;
	uint8_t ir;
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	struct sockaddr_in peer_addr;
	uint16_t lport;
	bool nonblock;
	uint8_t listen_ctx_ind;
#endif
};

struct w5500_runtime {
	struct net_if *iface;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ETH_W5500_RX_THREAD_STACK_SIZE);
	struct k_thread thread;
	uint8_t mac_addr[6];
	struct gpio_callback gpio_cb;
	struct k_sem int_sem;
	bool link_up;
	uint8_t buf[NET_ETH_MAX_FRAME_SIZE];

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	bool net_config_changed;
	struct in_addr local_ip_addr;
	struct w5500_socket sockets[W5500_MAX_SOCK_NUM];
#else
	struct w5500_socket sockets[1];
#endif
};

int w5500_spi_read(const struct device *dev, uint32_t addr, uint8_t *data, uint32_t len);
int w5500_spi_write(const struct device *dev, uint32_t addr, const uint8_t *data, uint32_t len);

static inline uint8_t w5500_spi_read_byte(const struct device *dev, uint32_t addr)
{
	uint8_t data;

	w5500_spi_read(dev, addr, &data, 1);

	return data;
}

static inline uint16_t w5500_spi_read_two_bytes(const struct device *dev, uint32_t addr)
{
	uint16_t val = 0, val2 = 0;

	val2 = (w5500_spi_read_byte(dev, addr)) << 8;
	val2 += w5500_spi_read_byte(dev, addr + 1);

	do {
		val = val2;

		val2 = (w5500_spi_read_byte(dev, addr)) << 8;
		val2 += w5500_spi_read_byte(dev, addr + 1);
	} while (val != val2);

	return val;
}

static inline int w5500_spi_write_byte(const struct device *dev, uint32_t addr, uint8_t data)
{
	return w5500_spi_write(dev, addr, &data, 1);
}

static inline int w5500_spi_write_two_bytes(const struct device *dev, uint32_t addr, uint16_t data)
{
	w5500_spi_write_byte(dev, addr, (uint8_t)(data >> 8));

	return w5500_spi_write_byte(dev, addr + 1, (uint8_t)(data));
}

int w5500_socket_readbuf(const struct device *dev, uint8_t sn, uint16_t offset, uint8_t *buf,
			 size_t len);
int w5500_socket_writebuf(const struct device *dev, uint8_t sn, uint16_t offset, const uint8_t *buf,
			  size_t len);
int w5500_socket_tx(const struct device *dev, uint8_t sn, const uint8_t *buf, size_t len);

uint16_t w5500_socket_rx(const struct device *dev, uint8_t sn, uint8_t *buf, size_t len);

int w5500_socket_command(const struct device *dev, uint8_t sn, uint8_t cmd);

static inline uint8_t w5500_socket_status(const struct device *dev, uint8_t sn)
{
	return w5500_spi_read_byte(dev, W5500_Sn_SR(sn));
}

static inline int w5500_socket_interrupt_clear(const struct device *dev, uint8_t sn,
					       uint8_t interrupt_mask)
{
	return w5500_spi_write_byte(dev, W5500_Sn_IR(sn), interrupt_mask);
}

static inline uint8_t w5500_socket_interrupt_status(const struct device *dev, uint8_t sn)
{
	return w5500_spi_read_byte(dev, W5500_Sn_IR(sn));
}

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
void w5500_hw_net_config(const struct device *dev);

int w5500_socket_offload_init(const struct device *w5500_dev);

int w5500_socket_create(int family, int type, int proto);

void __w5500_handle_incoming_conn_established(uint8_t socknum);

void __w5500_handle_incoming_conn_closed(uint8_t socknum);
#endif

#endif /*_W5500_*/
