# SPDX-FileCopyrightText: 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources(
  rtp.c
  rtp_packet.c
  rtp_transport.c
)

zephyr_library_sources_ifdef(CONFIG_RTP_TRANSPORT_SOCKET
    rtp_transport_socket.c
)

zephyr_library_sources_ifdef(CONFIG_RTP_TRANSPORT_NET_PKT
  rtp_transport_net_pkt.c
)

zephyr_library_sources_ifdef(CONFIG_SRTP
  rtp_srtp.c
  srtp.c
  srtp_crypto.c
)
