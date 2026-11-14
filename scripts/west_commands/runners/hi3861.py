# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Hi3861 ROM serial RAM loader; never writes flash or efuses."""

import binascii
import hashlib
import struct
import time
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner

try:
    import serial
except ImportError:
    serial = None

try:
    from elftools.elf.elffile import ELFFile
except ImportError:
    ELFFile = None


class Hi3861BinaryRunner(ZephyrBinaryRunner):
    RAM_START = 0x10A040
    RAM_END = 0x11A000
    MAGIC = b'\xef\xbe\xad\xde'
    ROM_ACK = b'\xe1\x1e\x5a\x42'

    def __init__(self, cfg, port, no_at_reset=False):
        super().__init__(cfg)
        self.port = port
        self.no_at_reset = no_at_reset
        self.pending = bytearray()

    @classmethod
    def name(cls):
        return 'hi3861'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--port', required=True, help='Hi3861 UART0 serial port')
        parser.add_argument(
            '--no-at-reset',
            action='store_true',
            help='Connect to ROM without sending the stock firmware AT reset',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return cls(cfg, args.port, args.no_at_reset)

    @classmethod
    def frame(cls, payload):
        data = cls.MAGIC + struct.pack('<H', len(payload) + 8) + payload
        return data + struct.pack('<H', binascii.crc_hqx(data, 0))

    @staticmethod
    def ymodem_packet(sequence, data, size):
        payload = data.ljust(size, b'\0')
        return (
            (b'\x01' if size == 128 else b'\x02')
            + bytes([sequence & 255, (~sequence) & 255])
            + payload
            + struct.pack('>H', binascii.crc_hqx(payload, 0))
        )

    @classmethod
    def signed_image(cls, data):
        if not data:
            raise ValueError('Empty Hi3861 RAM image')
        data += b'\0' * (-len(data) % 16)
        if len(data) + 48 > cls.RAM_END - cls.RAM_START:
            raise ValueError('Hi3861 image exceeds the reserved ROM-loader RAM window')
        header = struct.pack(
            '<16I',
            0x55AA55AA,
            56,
            0,
            0,
            0,
            0,
            0,
            0,
            64,
            len(data),
            0x003F0000,
            len(data) + 64,
            32,
            0x33CC33CC,
            0,
            0,
        )
        footer = hashlib.sha256(header + data).digest()
        footer += b'\0' * 8 + struct.pack('<II', len(data) + 112, 0x55AA55AA)
        return header + data + footer

    def check_elf(self):
        with open(self.cfg.elf_file, 'rb') as file:
            elf = ELFFile(file)
            if elf.header.e_machine != 'EM_RISCV' or elf.header.e_entry != self.RAM_START:
                raise ValueError('Hi3861 requires a RISC-V ELF with entry at 0x10a040')
            for segment in elf.iter_segments():
                if segment['p_type'] == 'PT_LOAD' and segment['p_memsz']:
                    start = segment['p_vaddr']
                    if start < self.RAM_START or start + segment['p_memsz'] > self.RAM_END:
                        raise ValueError('Hi3861 ELF load segment is outside the RAM window')

    def wait_for(self, link, expected, timeout=5):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            at = self.pending.find(expected)
            if at >= 0:
                del self.pending[: at + len(expected)]
                return
            self.pending.extend(link.read(4096))
        raise TimeoutError(
            f'Hi3861: expected {expected.hex()}; UART tail: {bytes(self.pending[-256:])!r}'
        )

    def connect_rom(self, link):
        if not self.no_at_reset:
            link.write(b'AT+RST=1000000\r\n')
            link.flush()
            self.wait_for(link, b'OK', timeout=4)
        self.logger.info('Waiting for Hi3861 ROM; press RESET if it is not already active')
        request = self.frame(struct.pack('<HII', 0x0FF0, 115200, 0x0108))
        expected = self.frame(self.ROM_ACK)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            link.write(request)
            link.flush()
            self.pending.extend(link.read(4096))
            if expected in self.pending:
                self.wait_for(link, expected)
                link.write(b'\x03')
                link.flush()
                time.sleep(0.1)
                link.write(b'\0')
                link.flush()
                self.wait_for(link, b'C')
                return
        raise TimeoutError('Hi3861 ROM did not respond; reset the board and use --no-at-reset')

    def send_packet(self, link, packet):
        for attempt in range(3):
            link.write(packet)
            link.flush()
            try:
                self.wait_for(link, b'\x06')
                return
            except TimeoutError:
                if attempt == 2:
                    raise
                self.logger.warning('Hi3861 YMODEM acknowledgment timeout; retrying packet')

    def transfer(self, link, data):
        name = f'zephyr.bin\0{len(data)}\0'.encode()
        self.send_packet(link, self.ymodem_packet(0, name, 128))
        for sequence, offset in enumerate(range(0, len(data), 1024), 1):
            self.send_packet(link, self.ymodem_packet(sequence, data[offset : offset + 1024], 1024))
        link.write(b'\x04')
        link.flush()
        self.wait_for(link, b'\x06\x06C', timeout=10)
        self.send_packet(link, self.ymodem_packet(0, b'', 128))

    def do_run(self, command, **kwargs):
        if serial is None or ELFFile is None:
            raise RuntimeError('Hi3861 runner requires pyserial and pyelftools')
        self.ensure_output('bin')
        self.check_elf()
        data = self.signed_image(Path(self.cfg.bin_file).read_bytes())
        link = serial.Serial(port=None, baudrate=115200, timeout=0.02, write_timeout=5)
        link.port = self.port
        link.dtr = False
        link.rts = False
        try:
            link.open()
            link.reset_input_buffer()
            self.connect_rom(link)
            self.transfer(link, data)
            self.logger.info(
                'Transferred %d bytes to RAM; flash and efuses were not changed', len(data)
            )
            deadline = time.monotonic() + 2
            output = bytearray(self.pending)
            self.pending.clear()
            while time.monotonic() < deadline:
                output.extend(link.read(4096))
            if output:
                self.logger.info('UART:\n%s', output.decode('utf-8', errors='backslashreplace'))
        finally:
            link.close()
