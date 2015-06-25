
#ifdef CONFIG_NETWORKING_UART
int net_driver_slip_init(void);
#else
#define net_driver_slip_init()
#endif
