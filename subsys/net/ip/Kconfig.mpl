# MPL Options

menuconfig NET_MCAST_MPL
	bool "MPL"
	default n
	help
	  Enable MPL-routing support.

if NET_MCAST_MPL
config NET_MCAST_MPL_FLOODING
	bool "Flood routing"
	default n
	help
		Enable flooding based routing
		
config NET_STATISTICS_MPL
	bool "MPL related statistics"
	default n
	help
	  Enable to collect MPL statistics

config NET_MCAST_MPL_PROACTIVE
	bool "Proactive MPL routing"
	default n
	help
	  Enable or disable proactive MPL routing

config NET_MCAST_MPL_SEED_SET_ENTRY_LIFETIME
	int "lifetime in minutes"
	default 30
	help
	  Minimum lifetime for an entry in the Seed Set

config NET_MCAST_MPL_DATA_MESSAGE_IMIN
	int "intervall size"
	default 32
	help
	  Trickle timer minimum intervall size

config NET_MCAST_MPL_DATA_MESSAGE_IMAX
	int "intervall size"
	default 1
	help
	  Trickle timer maximum intervall size

config NET_MCAST_MPL_DATA_MESSAGE_K
	int "constant"
	default 1
	help
	  Trickle timer redundancy constant

config NET_MCAST_MPL_DATA_MESSAGE_TIMER_EXPIRATION
	int "number of timer expirations"
	default 3
	help
	  Number of expiration events that occur before Trickle
	  Timer termination for a given MPL-Data-Message


config NET_MCAST_MPL_CONTROL_MESSAGE_IMIN
	int "intervall size"
	default 32
	help
	  Trickle timer minimum intervall size

config NET_MCAST_MPL_CONTROL_MESSAGE_IMAX
	int "intervall size"
	default 1
	help
	  Trickle timer maximum intervall size

config NET_MCAST_MPL_CONTROL_MESSAGE_K
	int "constant"
	default 1
	help
	  Trickle timer redundancy constant

config NET_MCAST_MPL_CONTROL_MESSAGE_TIMER_EXPIRATION
	int "number of timer expirations"
	default 1
	help
	  Number of expiration events that occur before Trickle
	  Timer termination for a given MPL-Control-Message

config NET_MCAST_MPL_ALL_FORWARDERS_SUB
	bool "sub to all forwarders domain"
	default y
	help
	  MPL forwarder by default subscribes to ALL_MPL_FORWARDER
	  realm local scope FF03:FC

config NET_MCAST_MPL_DOMAIN_SET_SIZE
	int "number of domains subscribed to"
	default 1
	help
	  Number of domains the MPL forwarder subscribes to and participates in

config NET_MCAST_MPL_SEED_SET_SIZE
	int "number of seed entries"
	default 2
	help
	  number of seed entries in the Seed Set to keep track of messages a
	  specific seed has sent recently. Entries are removed after
	  NET_MCAST_MPL_SEED_SET_ENTRY_LIFETIME

config NET_MCAST_MPL_BUFFERED_MESSAGE_SET_SIZE
	int "number of messages to be buffered"
	default 6
	help
	  messages stored in the Buffered Message Set are periodically forwarded by
	  expiring trickle timers until the NET_MCAST_MPL_XXX_TIMER_EXPIRATIONS are
	  reached
	  
config NET_MCAST_MPL_SEED_ID_LENGTH
	int "lenght of Seed Identifier"
	default 0
	help
	  Lenght of the Seed Identifier in MPL Seed Control and Info

config NET_MCAST_MPL_SEED_ID_L
	int "lower seed id byte"
	default 0
	help
	  lower byte of the Seed Identifier

config NET_MCAST_MPL_SEED_ID_H
	int "higher seed id byte"
	default 0
	help
	  higher byte of the Seed Identifier
endif # NET_MCAST_ROUTE_MPL
