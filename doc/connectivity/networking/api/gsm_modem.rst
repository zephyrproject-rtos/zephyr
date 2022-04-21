.. _gsm_modem:

Generic GSM Modem
#################

Overview
********

The generic GSM modem driver allows the user to connect Zephyr to a GSM modem
which provides a data connection to cellular operator's network.
The Zephyr uses :ref:`PPP (Point-to-Point Protocol) <ppp>` to connect
to the GSM modem using UART. Note that some cellular modems have proprietary
offloading support using AT commands, but usually those modems also support
3GPP standards and provide PPP connection to them.
See :ref:`GSM modem sample application <gsm-modem-sample>` how to setup Zephyr
to use the GSM modem.

The GSM muxing, that is defined in
`GSM 07.10 <https://www.etsi.org/deliver/etsi_ts/127000_127099/127010/15.00.00_60/ts_127010v150000p.pdf>`__,
and which allows mixing of AT commands and PPP traffic, is also supported in
this version of Zephyr. One needs to enable :kconfig:option:`CONFIG_GSM_MUX` and
:kconfig:option:`CONFIG_UART_MUX` configuration options to enable muxing.
