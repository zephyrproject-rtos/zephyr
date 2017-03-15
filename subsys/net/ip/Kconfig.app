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

endif # NET_APP_SETTINGS
