/**
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file nxp_wifi_net.h
 * This provides the calls related to the network layer.
 * Wi-Fi L2 layer
 */

#ifndef _NXP_WIFI_NET_H_
#define _NXP_WIFI_NET_H_

#include <string.h>

#include <osa.h>
#include <wmtypes.h>
#include <wmerrno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
#include "supp_api.h"
#include "supp_main.h"
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
#include "hapd_main.h"
#include "hapd_api.h"
#endif
#endif
#ifdef CONFIG_NET_DHCPV4_SERVER
#include "zephyr/net/dhcpv4_server.h"
#endif
#include <mlan_api.h>
#include <wmlog.h>
#ifdef RW610
#include <wifi-imu.h>
#else
#include <wifi-sdio.h>
#endif
#include <wifi-internal.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NET_MAC_ADDR_LEN 6

#define NET_IPV4_ADDR_U32(x) (x).in_addr.s_addr

#define SIZEOF_ETH_LLC_HDR       (8U)
#define SIZEOF_ETH_HDR           (14U)
#define MAX_INTERFACES_SUPPORTED 3U
#define NETIF_NAMESIZE           6

/* Net Packet Abstraction Layer
 *
 * Control net stack packets in different OS.
 * To compat with OS, common net packet structure is
 * - net_pkt
 *   - net_buf 1
 *     - data (cursor)
 *   - net_buf 2
 *     - data
 *
 * net_pkt manages whole packet and all hooked net_bufs.
 * net_buf carries actual payload.
 */
#define NAL_PKT_2_BUF(p)            ((void *)(((struct net_pkt *)(p))->cursor.buf))
#define NAL_BUF_NEXT(p)             ((void *)(((struct net_buf *)(p))->frags))
#define NAL_BUF_PAYLOAD(p)          ((void *)(((struct net_buf *)(p))->data))
#define NAL_BUF_LEN(p)              (((struct net_buf *)(p))->len)
#define NAL_BUF_TOT_LEN(p)          (net_buf_frags_len((struct net_buf *)(p)))
#define NAL_PKT_HEAD_ADDR(p)        ((void *)(((struct net_pkt *)(p))->cursor.pos))
#define NAL_PKT_SET_IFACE(p, iface) net_pkt_set_iface(p, iface)
#define NAL_SIZE_ALIGN(val, align)  (((t_u32)(val) + align - 1U) & ~(align - 1U))
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* This is an Token-Ring LLC structure */
struct eth_llc_hdr {
	t_u8 dsap;      /* destination SAP */
	t_u8 ssap;      /* source SAP */
	t_u8 llc;       /* LLC control field */
	t_u8 protid[3]; /* protocol id */
	t_u16 type;     /* ether type field */
} __packed;

/* copy zephyr struct net_if */
struct netif {
	/* The struct net_if from zephyr, so this is the same size */
	struct net_if net_iface;
};

/* Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct ethernetif {
	struct net_eth_addr ethaddr;
	/* Interface to bss type identification that tells the FW wherether
	 * the data is for STA for UAP
	 */
	uint8_t interface;
	/* Add whatever per-interface state that is needed here. */
};

struct interface {
	struct net_if *netif;
	uint8_t mac_address[6];
	struct net_addr ipaddr;
	struct net_addr nmask;
	struct net_addr gw;
	struct ethernetif state;
#ifdef CONFIG_NET_STATISTICS_WIFI
	struct net_stats_wifi stats;
#endif
	scan_result_cb_t scan_cb;
	uint16_t max_bss_cnt;
};

typedef struct interface interface_t;

enum net_address_types {
	/** static IP address */
	NET_ADDR_TYPE_STATIC = 0,
	/** Dynamic  IP address*/
	NET_ADDR_TYPE_DHCP = 1,
	/** Link level address */
	NET_ADDR_TYPE_LLA = 2,
};

/** This data structure represents an IPv4 address */
struct net_ipv4_config {
	/* Set to \ref ADDR_TYPE_DHCP to use DHCP to obtain the IP address or
	 * \ref ADDR_TYPE_STATIC to use a static IP. In case of static IP
	 * address ip, gw, netmask and dns members must be specified.  When
	 * using DHCP, the ip, gw, netmask and dns are overwritten by the
	 * values obtained from the DHCP server. They should be zeroed out if
	 * not used.
	 */
	enum net_address_types addr_type;
	/** The system's IP address in network order. */
	unsigned int address;
	/** The system's default gateway in network order. */
	unsigned int gw;
	/** The system's subnet mask in network order. */
	unsigned int netmask;
	/** The system's primary dns server in network order. */
	unsigned int dns1;
	/** The system's secondary dns server in network order. */
	unsigned int dns2;
};

#ifdef CONFIG_NXP_WIFI_IPV6
/** This data structure represents an IPv6 address */
struct net_ipv6_config {
	/** The system's IPv6 address in network order. */
	unsigned int address[4];
	/** The address type: linklocal, site-local or global. */
	unsigned char addr_type;
	/** The state of IPv6 address (Tentative, Preferred, etc). */
	unsigned char addr_state;
};
#endif

/* Network IP configuration.
 *
 * This data structure represents the network IP configuration
 * for IPv4 as well as IPv6 addresses
 */
struct net_ip_config {
#ifdef CONFIG_NXP_WIFI_IPV6
	/** The network IPv6 address configuration that should be
	 * associated with this interface.
	 */
	struct net_ipv6_config ipv6[CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT];
	/* The network IPv6 valid addresses count */
	size_t ipv6_count;
#endif
	/* The network IPv4 address configuration that should be
	 * associated with this interface.
	 */
	struct net_ipv4_config ipv4;
};

extern int wlan_get_mac_address(uint8_t *dest);
extern void wlan_wake_up_card(void);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P
/* Send a generic SDIO command to the WLAN firmware
 *
 * \param[in] buf Buffer containing the command data
 * \param[in] buflen Length of the command buffer
 *
 * \return MLAN_STATUS_SUCCESS on success; error code otherwise
 */
mlan_status wlan_send_gen_sdio_cmd(uint8_t *buf, uint32_t buflen);
extern int wlan_get_wfd_mac_address(unsigned char *dest);
#endif

#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
/* Update SoftAP RX rate information from received packet descriptor
 *
 * \param[in] rxpd Pointer to receive packet descriptor
 *
 * \return None
 */
void wrapper_wlan_update_uap_rxrate_info(RxPD * rxpd);
#endif


/* Handle received WLAN packet from driver
 *
 * This API processes a received packet from firmware/driver and
 * forwards it to the upper network stack or further handling logic.
 *
 * \param[in] datalen Length of received data
 * \param[in] rxpd Pointer to receive packet descriptor
 * \param[in] p Driver-specific buffer/context pointer
 * \param[in] payload Pointer to packet payload
 *
 * \return 0 on success; negative value on failure
 */
int wrapper_wlan_handle_rx_packet(t_u16 datalen, RxPD * rxpd, void *p, void *payload);

#ifdef CONFIG_NXP_WIFI_NET_MONITOR
/* Receive and process monitor mode data
 *
 * This API is used in network monitor mode to handle raw packets
 * along with their metadata information.
 *
 * \param[in] p Net packet
 * \param[in] rxpd Pointer to receive packet descriptor
 * \param[in] intf_pkt_len Length of interface packet
 *
 * \return None
 */
void user_recv_monitor_data(void *p, RxPD * rxpd, t_u16 intf_pkt_len);
#endif

#if defined(CONFIG_NET_DHCPV4)
/* Deactivate the dhcp timer
 */
void net_stop_dhcp_timer(void);
#endif

/* Converts Internet host address from the IPv4 dotted-decimal notation into
 * binary form (in network byte order)
 *
 * \param[in] cp IPv4 host address in dotted-decimal notation.
 *
 * \return IPv4 address in binary form
 */
static inline uint32_t net_inet_aton(const char *cp)
{
	struct in_addr addr;

	addr.s_addr = 0;
	(void)net_addr_pton(AF_INET, cp, &addr);
	return addr.s_addr;
}

/* set MAC hardware address to network interface
 *
 * \param[in] stamac sta MAC address.
 * \param[in] uapmac uap MAC address.
 *
 */
void net_wlan_set_mac_address(unsigned char *stamac, unsigned char *uapmac);

/* Skip a number of bytes at the start of a stack buffer
 *
 * \param[in] buf input stack buffer.
 * \param[in] in_offset offset to skip.
 *
 * \return the payload pointer after skip a number of bytes
 */
static inline uint8_t *net_stack_buffer_skip(void *buf, uint16_t in_offset)
{
	uint16_t offset_left = in_offset;
	struct net_buf *frag = ((struct net_pkt *)buf)->frags;

	while (frag && (frag->len <= offset_left)) {
		offset_left = offset_left - frag->len;
		frag = frag->frags;
	}
	return (uint8_t *)(frag->data + offset_left);
}

/* Free a buffer allocated from stack memory
 *
 * \param[in] buf stack buffer pointer.
 *
 */
static inline void net_stack_buffer_free(void *buf)
{
	net_pkt_unref((struct net_pkt *)buf);
}

/* Copy (part of) the contents of a packet buffer to an application supplied buffer
 *
 * \param[in] stack_buffer the stack buffer from which to copy data.
 * \param[in] dst the destination buffer.
 * \param[in] len length of data to copy.
 * \param[in] offset offset into the stack buffer from where to begin copying
 * \return copy status based on stack definition.
 */
int net_stack_buffer_copy_partial(void *stack_buffer, void *dst, uint16_t len, uint16_t offset);

/* Get the data payload inside the stack buffer.
 *
 * \param[in] buf input stack buffer.
 *
 * \return the payload pointer of the stack buffer.
 */
static inline void *net_stack_buffer_get_payload(void *buf)
{
	return net_pkt_data((struct net_pkt *)buf);
}

#ifdef CONFIG_NXP_WIFI_PKT_FWD
/* Send packet from Wi-Fi driver
 *
 * \param[in] interface Wi-Fi interface.
 * \param[in] stack_buffer net stack buffer pointer.
 *
 * \return WM_SUCCESS on success
 * \return -WM_FAIL otherwise
 */
int net_wifi_packet_send(uint8_t interface, void *stack_buffer);

/* Generate TX net packet buffer
 *
 * \param[in] interface Wi-Fi interface.
 * \param[in] payload source data payload pointer.
 * \param[in] datalen data length.
 *
 * \return net packet
 */
struct net_pkt *gen_tx_pkt_from_data(uint8_t interface, uint8_t *payload, uint16_t datalen);
#endif

/* Allocate net stack buffer in RX path
 *
 * \param[in] offset reserved headroom size
 * \param[in] len payload size
 *
 * \return net pkt ptr on success
 * \return zero ptr on failure
 */
void *net_stack_buffer_alloc_rx(int offset, int len);

/* Allocate and reserve headroom, and copy partial net packet in TX path
 *
 * \param[in] pkt net stack packet
 * \param[in] p buffer hooked in packet, carrying src payload
 * \param[in] offset reserved headroom size
 *
 * \return net pkt ptr on success
 * \return zero ptr on failure
 */
void *net_stack_buffer_clone_tx_frag(void *pkt, void *p, int offset);

/* Extend net buf header size and decrease start address
 *
 * \param[in] p net buf hooked in net pkt
 * \param[in] len extending header size
 *
 * \return 0 on success
 * \return non 0 on failure
 */
int net_stack_buffer_push(void *p, int len);

/* Set net interface field in net pkt
 *
 * \param[in] p net packet
 * \param[in] interface net interface index
 *
 */
void net_stack_buffer_set_iface(void *p, int interface);

/* Allocate and reserve headroom, and copy whole net packet in TX path
 *
 * \param[in] p net stack packet
 * \param[in] offset reserved headroom size
 *
 * \return net pkt ptr on success
 * \return zero ptr on failure
 */
static inline void *net_stack_buffer_clone_tx(void *p, int offset)
{
	return net_stack_buffer_clone_tx_frag(p, NAL_PKT_2_BUF(p), offset);
}

/* Adjust net packet size to actual payload length
 *
 * \param[in] p net stack packet
 * \param[in] len actual payload length
 *
 */
static inline void net_stack_buffer_size_adjust(void *p, int len)
{
	(void)net_pkt_update_length(p, len);
}

/* Converts Internet host address in network byte order to a string in IPv4
 * dotted-decimal notation
 *
 * \param[in] addr IP address in network byte order.
 * \param[out] cp buffer in which IPv4 dotted-decimal string is returned.
 *
 */
static inline void net_inet_ntoa(unsigned long addr, char *cp)
{
	struct in_addr saddr;

	saddr.s_addr = addr;
	net_addr_ntop(AF_INET, &saddr, cp, NET_IPV4_ADDR_LEN);
}

/* Check whether buffer is IPv4 or IPV6 packet type
 *
 * \param[in] buffer pointer to buffer where packet to be checked located.
 *
 * \return true if buffer packet type matches with IPv4 or IPv6, false otherwise.
 *
 */
static inline bool net_is_ip_or_ipv6(const uint8_t *buffer)
{
	if (((const struct net_eth_hdr *)(const void *)buffer)->type == htons(ETH_P_IP) ||
	    ((const struct net_eth_hdr *)(const void *)buffer)->type == htons(ETH_P_IPV6)) {
		return true;
	}

	return false;
}

/* Initialize TCP/IP networking stack
 *
 * \return WM_SUCCESS on success
 * \return -WM_FAIL otherwise
 */
int net_wlan_init(void);

/* DiInitialize TCP/IP networking stack
 *
 * \return WM_SUCCESS on success
 * \return -WM_FAIL otherwise
 */
int net_wlan_deinit(void);

/** Enable all Wi-Fi network (access point).
 */
void nxp_net_enable_all_networks(void);

/** Stop and remove all Wi-Fi network (access point).
 */
void nxp_net_remove_all_networks(void);

/* Get STA interface netif structure pointer
 *
 * \rerurn A pointer to STA interface netif structure
 *
 */
struct netif *net_get_sta_interface(void);

#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
/* Get uAP interface netif structure pointer
 *
 * \rerurn A pointer to uAP interface netif structure
 *
 */
struct netif *net_get_uap_interface(void);
#endif

/* NXP wrapper to get network interface name.
 *
 * @details If interface name support is not enabled, empty string is returned.
 *
 * @param iface Pointer to network interface
 * @param buf User supplied buffer
 * @param len Length of the user supplied buffer
 *
 * @return Length of the interface name copied to buf,
 *         -EINVAL if invalid parameters,
 *         -ERANGE if name cannot be copied to the user supplied buffer,
 *         -ENOTSUP if interface name support is disabled,
 */
static inline int nxp_net_if_get_name(struct net_if *iface, char *buf, int len)
{
	return net_if_get_name(iface, buf, len);
}

/* Get interface name for given netif
 *
 * \param[out] pif_name Buffer to store interface name
 * \param[in] iface Interface to get the name
 *
 * \return WM_SUCCESS on success
 * \return -WM_FAIL otherwise
 *
 */
int net_get_if_name_netif(char *pif_name, struct netif *iface);

/* Get station interface handle
 *
 * Some APIs require the interface handle to be passed to them. The handle can
 * be retrieved using this API.
 *
 * \return station interface handle
 */
void *net_get_sta_handle(void);
#define net_get_mlan_handle() net_get_sta_handle()

#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
/* Get micro-AP interface handle
 *
 * Some APIs require the interface handle to be passed to them. The handle can
 * be retrieved using this API.
 *
 * \return micro-AP interface handle
 */
void *net_get_uap_handle(void);
#endif

/* Take interface up
 *
 * Change interface state to up. Use net_get_sta_handle(),
 * net_get_uap_handle() to get interface handle.
 *
 * \param[in] intrfc_handle interface handle
 *
 * \return void
 */
void net_interface_up(void *intrfc_handle);

/* Take interface down
 *
 * Change interface state to down. Use net_get_sta_handle(),
 * net_get_uap_handle() to get interface handle.
 *
 * \param[in] intrfc_handle interface handle
 *
 * \return void
 */
void net_interface_down(void *intrfc_handle);

/* Stop DHCP client on given interface
 *
 * Stop the DHCP client on given interface state. Use net_get_sta_handle(),
 * net_get_uap_handle() to get interface handle.
 *
 * \param[in] intrfc_handle interface handle
 *
 * \return void
 */
void net_interface_dhcp_stop(void *intrfc_handle);

/* Configure IP address for interface
 *
 * \param[in] addr Address that needs to be configured.
 * \param[in] intrfc_handle Handle for network interface to be configured.
 *
 * \return WM_SUCCESS on success or an error code.
 */
int net_configure_address(struct net_ip_config *addr, void *intrfc_handle);

/* Configure DNS server address
 *
 * \param[in] ip IP address of the DNS server to set
 * \param[in] role Network wireless BSS Role
 *
 */
void net_configure_dns(struct net_ip_config *ip, unsigned int role);

/* Get interface IP Address in \ref net_ip_config
 *
 * This function will get the IP address of a given interface. Use
 * net_get_sta_handle(), net_get_uap_handle() to get
 * interface handle.
 *
 * \param[out] addr \ref net_ip_config
 * \param[in] intrfc_handle interface handle
 *
 * \return WM_SUCCESS on success or error code.
 */
int net_get_if_addr(struct net_ip_config *addr, void *intrfc_handle);

#ifdef CONFIG_NET_DHCPV4_SERVER
/*  NXP wrapper to stop DHCPv4 server instance on an iface
 *
 *  @param iface A valid pointer on an interface
 *
 *  @return 0 on success, a negative error code otherwise.
 */
static inline int nxp_net_dhcpv4_server_stop(struct net_if *iface)
{
	return net_dhcpv4_server_stop(iface);
}
#endif

#ifdef CONFIG_NXP_WIFI_IPV6
/* Get interface IPv6 Addresses & their states in \ref net_ip_config
 *
 * This function will get the IPv6 addresses & address states of a given
 * interface. Use net_get_sta_handle() to get interface handle.
 *
 * \param[out] addr \ref net_ip_config
 * \param[in] intrfc_handle interface handle
 *
 * \return WM_SUCCESS on success or error code.
 */
int net_get_if_ipv6_addr(struct net_ip_config *addr, void *intrfc_handle);

/* Get list of preferred IPv6 Addresses of a given interface
 * in \ref net_ip_config
 *
 * This function will get the list of IPv6 addresses whose address state
 * is Preferred.
 * Use net_get_sta_handle() to get interface handle.
 *
 * \param[out] addr \ref net_ip_config
 * \param[in] intrfc_handle interface handle
 *
 * \return Number of IPv6 addresses whose address state is Preferred
 */
int net_get_if_ipv6_pref_addr(struct net_ip_config *addr, void *intrfc_handle);

/* Get the description of IPv6 address state
 *
 * This function will get the IPv6 address state description like -
 * Invalid, Preferred, Deprecated
 *
 * \param[in] addr_state Address state
 *
 * \return IPv6 address state description
 */
char *ipv6_addr_state_to_desc(unsigned char addr_state);

/* Get the description of IPv6 address
 *
 * This function will get the IPv6 address type description like -
 * Linklocal, Global, Sitelocal, Uniquelocal
 *
 * \param[in] ipv6_conf Pointer to IPv6 configuration of type \ref net_ipv6_config
 *
 * \return IPv6 address description
 */
char *ipv6_addr_addr_to_desc(struct net_ipv6_config *ipv6_conf);

/* Get the description of IPv6 address type
 *
 * This function will get the IPv6 address type description like -
 * Linklocal, Global, Sitelocal, Uniquelocal
 *
 * \param[in] ipv6_conf Pointer to IPv6 configuration of type \ref net_ipv6_config
 *
 * \return IPv6 address type description
 */
char *ipv6_addr_type_to_desc(struct net_ipv6_config *ipv6_conf);

/* Get all interface IPv6 Addresses and count
 *
 * This function will get the IPv6 addresses and the count of all interfaces.
 *
 * \param[out] buf buffer to saving the IPv6 addresses.
 * \param[in] buf_size the size of buffer can save the IPv6 address.
 *
 * \return IPv6 address count.
 */
uint8_t net_get_all_if_ipv6_addr_and_cnt(char *buf, uint32_t buf_size);
#endif

/* Get interface IP Address
 *
 * This function will get the IP Address of a given interface. Use
 * net_get_sta_handle(), net_get_uap_handle() to get
 * interface handle.
 *
 * \param[out] ip ip address pointer
 * \param[in] intrfc_handle interface handle
 *
 * \return WM_SUCCESS on success or error code.
 */
int net_get_if_ip_addr(uint32_t *ip, void *intrfc_handle);

/* Get interface IP Subnet-Mask
 *
 * This function will get the Subnet-Mask of a given interface. Use
 * net_get_sta_handle(), net_get_uap_handle() to get
 * interface handle.
 *
 * \param[in] mask Subnet Mask pointer
 * \param[in] intrfc_handle interface
 *
 * \return WM_SUCCESS on success or error code.
 */
int net_get_if_ip_mask(uint32_t *nm, void *intrfc_handle);

/* Display network statistics */
void net_stat(void);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
/* Get supplicant ready state */
bool get_supp_ready_state(void);

/* NXP wrapper to get Wi-Fi connection parameters recently used
 *
 * @param iface Network interface to use
 * @param params the Wi-Fi connection parameters recently used
 *
 * @return 0 if ok, < 0 if error
 */
static inline int nxp_supp_get_wifi_conn_params(struct net_if *iface,
						struct wifi_connect_req_params *params)
{
	return supplicant_get_wifi_conn_params(net_if_get_device(iface), iface, params);
}

/* NXP wrapper for station to disconnect and stops any subsequent scan
 *  or connection attempts
 *
 * @param iface Network interface to use
 *
 * @return: 0 for OK; -1 for ERROR
 */
static inline int nxp_supp_disconnect(struct net_if *iface)
{
	return supplicant_disconnect(net_if_get_device(iface), iface);
}

/* NXP wrapper to get station status.
 *
 * @param iface Network interface to use
 * @param status: Status structure to fill
 *
 * @return: 0 for OK; -1 for ERROR
 */
static inline int nxp_supp_status(struct net_if *iface, struct wifi_iface_status *status)
{
	return supplicant_status(net_if_get_device(iface), iface, status);
}

/* NXP wrapper to flush PMKSA cache entries
 *
 * @param iface Network interface to use
 *
 * @return 0 if ok, < 0 if error
 */
static inline int nxp_supp_pmksa_flush(struct net_if *iface)
{
	return supplicant_pmksa_flush(net_if_get_device(iface), iface);
}

/* NXP wrapper to get wpa_supplicant handle by interface name
 *
 * This API retrieves the corresponding wpa_supplicant instance
 * based on the given network interface name.
 *
 * @param ifname Network interface name
 *
 * @return Pointer to wpa_supplicant handle on success; NULL if not found
 */
static inline struct wpa_supplicant *nxp_zep_get_handle_by_ifname(const char *ifname)
{
	return zephyr_get_handle_by_ifname(ifname);
}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
/* NXP wrapper to get hostapd interface handle by interface name
 *
 * This API retrieves the corresponding hostapd interface instance
 * based on the given network interface name.
 *
 * @param ifname Network interface name
 *
 * @return Pointer to hostapd_iface handle on success; NULL if not found
 */
static inline struct hostapd_iface *nxp_zep_get_hapd_handle_by_ifname(const char *ifname)
{
	return zephyr_get_hapd_handle_by_ifname(ifname);
}

/* Disable AP via the L2 mgmt API */
void nxp_net_request_ap_disable(void);

/* NXP wrapper to enable or disable 11N for AP
 *
 * @param iface Network interface to use
 * @param enable Enable or disable
 *
 * @return 0 for OK; -1 for ERROR
 */
static inline int nxp_hapd_11n_cfg(struct net_if *iface, uint8_t enable)
{
	return hostapd_11n_cfg(net_if_get_device(iface), iface, enable);
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_11AC
/* NXP wrapper to enable or disable 11AC for AP
 *
 * @param iface Network interface to use
 * @param enable Enable or disable
 *
 * @return 0 for OK; -1 for ERROR
 */
static inline int nxp_hapd_11ac_cfg(struct net_if *iface, uint8_t enable)
{
	return hostapd_11ac_cfg(net_if_get_device(iface), iface, enable);
}
#endif

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_11AX
/* NXP wrapper to enable or disable 11AX for AP
 *
 * @param net_iface Network interface to use
 * @param enable Enable or disable
 *
 * @return 0 for OK; -1 for ERROR
 */
static inline int nxp_hapd_11ax_cfg(struct net_if *iface, uint8_t enable)
{
	return hostapd_11ax_cfg(net_if_get_device(iface), iface, enable);
}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_11AX */
#endif /* CONFIG_WIFI_NM_HOSTAPD_AP */
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT */

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P

/* Get current BSS type (e.g. STA / AP / P2P)
 *
 * This API is typically used to query the current interface role.
 *
 * @return BSS type on success
 */
int netif_get_bss_type(void);
#endif

/* NXP internal TX wrapper for Wi-Fi driver
 *
 * This API is used to transmit a network packet through the underlying
 * Wi-Fi driver. It can optionally indicate whether the packet is being
 * forwarded or from net stack.
 *
 * @param dev     Pointer to Wi-Fi device
 * @param pkt     Network packet to transmit
 * @param pkt_fwd Whether the packet is forwarded (true) or from stack (false)
 *
 * @return 0 for OK; negative value for ERROR
 */
int nxp_wifi_internal_tx(const struct device *dev, struct net_pkt *pkt, bool pkt_fwd);

/* Register RX callback for NXP Wi-Fi driver
 *
 * This API allows upper layers (e.g. network stack) to
 * register a callback function which will be invoked when a packet
 * is received from the Wi-Fi driver.
 *
 * @param rx_cb_fn Callback function pointer
 *                 - iface: Network interface on which packet is received
 *                 - pkt  : Received network packet
 *
 * @return None
 */
void nxp_wifi_internal_register_rx_cb(int (*rx_cb_fn)(struct net_if *iface, struct net_pkt *pkt));

/* Get network interface by name
 *
 * This API searches for a network interface based on its name
 * and returns the corresponding net_if instance.
 *
 * @param ifname Name of the network interface
 *
 * @return Pointer to net_iface if found; NULL otherwise
 */
const struct netif *net_if_get_binding(const char *ifname);
#endif /* _NXP_WIFI_NET_H_ */
