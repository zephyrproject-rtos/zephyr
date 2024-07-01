# Copyright (c) 2023 Trackunit Corporation
# SPDX-License-Identifier: Apache-2.0

import socket
import threading
import select
import time
import copy

class TEUDPTransmitSession():
    def __init__(self, address, sock: socket.socket, request: dict):
        self.address = address
        self.sock = sock
        self.packets_remaining = request['packets_to_send']
        self.packet_interval = request['packet_interval']
        self.packet = request['packet']
        self.last_packet_transmit_at = time.monotonic()

    def get_address(self):
        return self.address

    def update(self) -> bool:
        if not self._packet_interval_reached_():
            return True

        self._transmit_packet()
        return self._has_remaining_packets_()

    def _packet_interval_reached_(self) -> bool:
        return (time.monotonic() - self.last_packet_transmit_at) > self.packet_interval

    def _transmit_packet(self):
        self.sock.sendto(self.packet, self.address)
        self.last_packet_transmit_at = time.monotonic()
        self.packets_remaining -= 1

    def _has_remaining_packets_(self) -> bool:
        return self.packets_remaining > 0

class TEUDPTransmit():
    def __init__(self):
        self.running = True
        self.on_terminated = None
        self.thread = threading.Thread(target=self._target_)
        self.sessions = []

        self._prepare_prng_data_()

    def start(self):
        self.thread.start()

    def stop(self):
        if self.thread == threading.current_thread():
            return
        self.running = False
        self.thread.join(1)

    def signal_terminated(self, on_terminated):
        self.on_terminated = on_terminated

    def _prepare_prng_data_(self):
        data = []
        prng_state = 1234
        for _ in range(4096):
            prng_state = ((1103515245 * prng_state) + 12345) % (1 << 31)
            data.append(prng_state & 0xFF)
        self.prng_data = bytes(data)

    def _get_prng_packet_(self, size: int) -> bytes:
        return self.prng_data[0:size]

    def _target_(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        sock.bind(('0.0.0.0', 7782))

        while self.running:
            try:
                timeout = 0.5 if not self.sessions else 0.015
                ready_to_read, _, _ = select.select([sock], [], [], timeout)

                if ready_to_read:
                    self._create_session_(sock)

                self._update_sessions_()

            except Exception as e:
                print(e)
                if self.on_terminated is not None:
                    self.on_terminated()
                break

        sock.close()

    def _parse_session_request_(self, data: bytes) -> dict:
        assert len(data) == 4, 'Session request has incorrect size'
        packets_to_send = int(data[0])
        packet_interval = float(data[1]) / 100
        packet_size = int.from_bytes(data[2:4], 'big')
        assert packet_size > 0, f'Packet size {packet_size} too small'
        assert packet_size <= 4096, f'Packet size {packet_size} too large'
        return {
            'packets_to_send': packets_to_send,
            'packet_interval': packet_interval,
            'packet': self._get_prng_packet_(packet_size),
        }

    def _create_session_(self, sock: socket.socket):
        data, address = sock.recvfrom(4096)
        try:
            request = self._parse_session_request_(data)
            self.sessions.append(TEUDPTransmitSession(address, sock, request))
        except Exception as e:
            print(f'Failed to create session for address: {address} due to {e}')

    def _update_sessions_(self):
        sessions = copy.copy(self.sessions)

        for session in sessions:
            if session.update():
                continue

            self.sessions.remove(session)
