# Copyright (c) 2023, Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

import socket
import threading
import select
import time
import copy

class TEUDPReceiveSession():
    def __init__(self, address, timeout = 1):
        self.address = address
        self.last_packet_received_at = time.monotonic()
        self.timeout = timeout
        self.packets_received = 0
        self.packets_dropped = 0

    def get_address(self):
        return self.address

    def on_packet_received(self, data):
        if self._validate_packet_(data):
            self.packets_received += 1
        else:
            self.packets_dropped += 1

        self.last_packet_received_at = time.monotonic()

    def update(self):
        if (time.monotonic() - self.last_packet_received_at) > self.timeout:
            return (self.packets_received, self.packets_dropped)
        return None

    def _validate_packet_(self, data: bytes) -> bool:
        prng_state = 1234
        for b in data:
            prng_state = ((1103515245 * prng_state) + 12345) % (1 << 31)
            if prng_state & 0xFF != b:
                return False
        return True

class TEUDPReceive():
    def __init__(self):
        self.running = True
        self.thread = threading.Thread(target=self._target_)
        self.sessions = []

    def start(self):
        self.thread.start()

    def stop(self):
        self.running = False
        self.thread.join(1)

    def _target_(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        sock.bind(('0.0.0.0', 7781))

        while self.running:
            try:
                ready_to_read, _, _ = select.select([sock], [sock], [], 0.5)

                if not ready_to_read:
                    self._update_sessions_(sock)
                    continue

                data, address = sock.recvfrom(4096)

                print(f'udp received {len(data)} bytes -> {address[0]}:{address[1]}')

                session = self._get_session_by_address_(address)
                session.on_packet_received(data)

            except Exception as e:
                print(e)
                break

        sock.close()

    def _get_session_by_address_(self, address) -> TEUDPReceiveSession:
        # Search for existing session
        for session in self.sessions:
            if session.get_address() == address:
                return session

        # Create and return new session
        print(f'Created session for {address[0]}:{address[1]}')
        self.sessions.append(TEUDPReceiveSession(address, 2))
        return self.sessions[-1]

    def _update_sessions_(self, sock):
        sessions = copy.copy(self.sessions)

        for session in sessions:
            result = session.update()

            if result is None:
                continue

            response = bytes([result[0], result[1]])

            print(f'Sending result {response} to address {session.get_address()}')
            sock.sendto(response, session.get_address())

            print(f'Removing session for address {session.get_address()}')
            self.sessions.remove(session)
