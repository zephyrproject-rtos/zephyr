# Kconfig.app - Options for sample applications

#
# Copyright (c) 2016 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

menuconfig NET_APP_SETTINGS
	bool "Set network settings for sample applications"
	default n
	help
	  Allow IP addresses to be set in config file for
	  networking client/server sample applications, or
	  some link-layer dedicated settings like the channel.
	  Beware this is not meant to be used for proper
	  provisioning but quick sampling/testing.

if NET_APP_SETTINGS

if NET_IPV6

config NET_APP_MY_IPV6_ADDR
	string "My IPv6 address"
	help
	  Use 2001:db8::1 here if uncertain.

config NET_APP_PEER_IPV6_ADDR
	string "Peer IPv6 address"
	help
	  This is only applicable in client side applications that try
	  to establish a connection to peer host.
	  Use 2001:db8::2 here if uncertain.

endif # NET_IPV6

if NET_IPV4

config NET_APP_MY_IPV4_ADDR
	string "My IPv4 address"
	help
	  Use 192.0.2.1 here if uncertain.

config NET_APP_PEER_IPV4_ADDR
	string "Peer IPv4 address"
	help
	  This is only applicable in client side applications that try
	  to establish a connection to peer host.
	  Use 192.0.2.2 here if uncertain.

endif # NET_IPV4

if NET_L2_IEEE802154 || NET_L2_RAW_CHANNEL

config NET_APP_IEEE802154_DEV_NAME
	string "IEEE 802.15.4 device name"
	help
	  The device name to get bindings from in the sample application.

config NET_APP_IEEE802154_PAN_ID
	hex "IEEE 802.15.4 PAN ID"
	default 0xabcd
	help
	  The PAN ID to use by default in the sample.

config NET_APP_IEEE802154_CHANNEL
	int "IEEE 802.15.4 channel"
	default 26
	help
	  The channel to use by default in the sample application.

config NET_APP_IEEE802154_RADIO_TX_POWER
	int "IEEE 802.15.4 TX power in dbm"
	default 0
	help
	  The TX power to use by default in the sample application.
	  See NET_L2_IEEE802154_RADIO_DFLT_TX_POWER for more info.

config NET_APP_IEEE802154_SECURITY_KEY
	string "IEEE 802.15.4 security key"
	default "moooh!"
	depends on NET_L2_IEEE802154_SECURITY
	help
	  The key string to use for the link-layer security part.

config NET_APP_IEEE802154_SECURITY_KEY_MODE
	int "IEEE 802.15.4 security key mode"
	default 0
	range 0 0
	depends on NET_L2_IEEE802154_SECURITY
	help
	  The key mode to use for the link-layer security part.
	  Only implicit mode is supported, thus 0.

config NET_APP_IEEE802154_SECURITY_LEVEL
	int "IEEE 802.15.4 security level (0-7)"
	default 0
	range 0 7
	depends on NET_L2_IEEE802154_SECURITY
	help
	  The security level to use for the link-layer security part.
	  0 means no security
	  1 authentication only with a 4 bytes length tag
	  2 authentication only with a 8 bytes length tag
	  3 authentication only with a 16 bytes length tag
	  4 encryption only
	  5 encryption/authentication with a 4 bytes length tag
	  6 encryption/authentication with a 8 bytes length tag
	  7 encryption/authentication with a 16 bytes length tag

endif # NET_L2_IEEE802154 || NET_L2_RAW_CHANNEL

endif # NET_APP_SETTINGS
