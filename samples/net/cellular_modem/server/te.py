# Copyright (c) 2023, Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

import signal
from te_udp_echo import TEUDPEcho
from te_udp_receive import TEUDPReceive
from te_udp_transmit import TEUDPTransmit

udp_echo = TEUDPEcho()
udp_receive = TEUDPReceive()
udp_transmit = TEUDPTransmit()

def terminate():
    udp_echo.stop()
    udp_receive.stop()
    udp_transmit.stop()
    print("stopped")

udp_echo.signal_terminated(terminate)
udp_receive.signal_terminated(terminate)
udp_transmit.signal_terminated(terminate)

udp_echo.start()
udp_receive.start()
udp_transmit.start()

print("started")

def terminate_handler(a, b):
    terminate()

signal.signal(signal.SIGTERM, terminate_handler)
signal.signal(signal.SIGINT, terminate_handler)
