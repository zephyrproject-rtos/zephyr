#include <net/ip_buf.h>

#ifdef CONFIG_ETHERNET

typedef int (*ethernet_tx_callback)(struct net_buf *buf);
void net_driver_ethernet_register_tx(ethernet_tx_callback cb);
bool net_driver_ethernet_is_opened(void);
void net_driver_ethernet_recv(struct net_buf *buf);

int net_driver_ethernet_init(void);

#else

#define net_driver_ethernet_init()

#endif
