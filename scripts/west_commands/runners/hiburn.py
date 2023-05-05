# Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing HiSilicon devices'''

import struct
from serial import Serial
from time import sleep
from os.path import basename
from progress.bar import Bar

from runners.core import ZephyrBinaryRunner, RunnerCaps

DEADBEEF = struct.pack('<I', 0xdeadbeef)

CMD_DL_IMAGE = 0xD2
CMD_RUN_RAM = 0xF0

CMD_ACK_SUCCESS_ROM = b'\xe1\x1e\x5a\x42'
CMD_ACK_SUCCESS = b'\xe1\x1e\x5a\xa5'

Y_SOH = b'\x01'
Y_STX = b'\x02'
Y_EOT = b'\x04'
Y_ACK = b'\x06'


def make_cmd(payload):
    cmd = bytearray()
    cmd += DEADBEEF
    cmd += struct.pack('<H', 4 + 2 + len(payload) + 2)
    cmd += payload
    cmd += struct.pack('<H', crc16(cmd))
    return cmd


def cmd_run_ram(baud_rate):
    return make_cmd(struct.pack(
        '<HII', palindrome(CMD_RUN_RAM),
        baud_rate, 0x0108))


def cmd_download_flash(address, write_size, erase_size):
    return make_cmd(struct.pack(
        '<HIIIH', palindrome(CMD_DL_IMAGE),
        address, write_size, erase_size, 0x00))


def loady(ser, name, data):
    def _soh(seq, name='', size=''):
        payload = f'{name}\x00{size}\x00'.encode().ljust(128, b'\x00')
        pkt = bytearray(Y_SOH)
        pkt += struct.pack('<H', palindrome(seq))
        pkt += payload
        pkt += struct.pack('>H', crc16(payload))
        return pkt

    def _stx(seq, data):
        payload = data.ljust(1024, b'\x00')
        pkt = bytearray(Y_STX)
        pkt += struct.pack('<H', palindrome(seq))
        pkt += payload
        pkt += struct.pack('>H', crc16(payload))
        return pkt

    def _read(expected):
        while True:
            n = ser.read(1024)
            if len(n) == 0:
                continue
            elif len(n) > len(expected):
                print(n.decode('ascii', errors='replace').strip())
                return False
            return n == expected

    ser.write(_soh(0, name, len(data)))
    if not _read(Y_ACK):
        return False

    with Bar(max=len(data)) as bar:
        for i in range(0, len(data), 1024):
            idx = i // 1024 + 1
            block = data[i:i + 1024]
            ser.write(_stx(idx, block))
            if not _read(Y_ACK):
                return False
            bar.next(len(block))

    ser.write(Y_EOT)
    if not _read(Y_ACK + Y_ACK + b'C'):
        return False

    ser.write(_soh(0))
    if not _read(Y_ACK):
        return False

    return True


def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = (crc << 1) ^ 0x1021 if crc & 0x8000 else crc << 1
    return crc & 0xFFFF


def palindrome(n):
    return ((~n & 0xff) << 8) | (n & 0xff)


def align_up(x: int, align: int) -> int:
    return (x + align - 1) & ~(align - 1)


class HiburnBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for HiBurn'''

    def __init__(self, cfg, port, baudrate, loaderboot_bin, flashboot_bin, app_address, logs):
        super().__init__(cfg)
        self.port = port
        self.baudrate = baudrate
        self.loaderboot_bin = loaderboot_bin
        self.flashboot_bin = flashboot_bin
        self.app_bin = cfg.bin_file
        self.app_address = app_address
        self.logs = logs

    @classmethod
    def name(cls):
        return 'hiburn'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--port', required=True,
                            help='serial port to flash')
        parser.add_argument('--baudrate', type=int, default=115200,
                            help='serial baud rate, default 115200')
        parser.add_argument('--loaderboot-bin', required=True,
                            help='path to loaderboot.bin')
        parser.add_argument('--flashboot-bin', required=True,
                            help='path to flashboot.bin')
        parser.add_argument('--app-address', default='0x2000',
                            help='application load address')
        parser.add_argument('--logs', action='store_true',
                            help='print device logs during flashing')

    @classmethod
    def do_create(cls, cfg, args):
        app_address = int(args.app_address, 16) if args.app_address.startswith(
            '0x') else int(args.app_address)

        return HiburnBinaryRunner(cfg, args.port, args.baudrate, args.loaderboot_bin,
                                  args.flashboot_bin, app_address, args.logs)

    def do_run(self, command, **kwargs):
        with open(self.loaderboot_bin, 'rb') as f:
            loaderboot = f.read()

        with open(self.flashboot_bin, 'rb') as f:
            flashboot = f.read()

        with open(self.app_bin, 'rb') as f:
            app = f.read()

        partitions = [
            [0x0000, len(flashboot), basename(self.flashboot_bin), flashboot],
            [self.app_address, len(app), basename(self.app_bin), app],
        ]

        ser = Serial(self.port, 115200, timeout=0.05)

        print('Waiting for device reset...')

        connected = False
        while not connected:
            ser.write(cmd_run_ram(self.baudrate))

            # Check if connected
            for r in self._read_cmd(ser, wait=False):
                if r == CMD_ACK_SUCCESS_ROM:
                    connected = True
                    break
                else:
                    print('Failed to connect to device')
                    return

        print('Device connected')

        if ser.baudrate != self.baudrate:
            ser.close()
            ser.baudrate = self.baudrate
            ser.open()

        sleep(0.1)

        ser.write(b'\03')

        sleep(0.1)

        ser.write(b'\00')

        # Read logs
        for _ in self._read_cmd(ser):
            continue

        print(f'Run {basename(self.loaderboot_bin)}...')

        if not loady(ser, basename(self.loaderboot_bin), loaderboot):
            print('Failed to load loaderboot')
            return

        # Read logs and check
        for r in self._read_cmd(ser):
            if r != CMD_ACK_SUCCESS:
                print('Failed to load loaderboot')
                return

        for [addr, size, name, data] in partitions:
            print(f'Download {name}...')

            ser.write(cmd_download_flash(
                addr, size, align_up(size, 4096)))

            sleep(0.2)

            # Read logs and check
            for r in self._read_cmd(ser):
                if r != CMD_ACK_SUCCESS:
                    print(f'Failed to start download of {name}')
                    return

            if not loady(ser, name, data):
                print(f'Failed to download {name}')
                return

            # Read logs and check
            for r in self._read_cmd(ser):
                if r != CMD_ACK_SUCCESS:
                    print(f'Failed to finish download of {name}')
                    return

        print('Done')

    def _print_device_logs(self, data):
        if self.logs and len(data) > 0:
            print(data.decode('ascii', errors='replace').strip())

    def _read_cmd(self, ser, wait=True):
        result = []
        data = bytearray()

        while True:
            read = ser.read(1024)
            data += read
            if len(read) == 0:
                if len(data) > 0:
                    break
                elif not wait:
                    return result

        while len(data) > 0:
            offset = data.find(DEADBEEF)
            if offset == -1:
                self._print_device_logs(data)
                break

            if offset > 0:
                self._print_device_logs(data[:offset])
                data = data[offset:]

            while len(data) < 6:
                data += ser.read(1024)

            length = struct.unpack('<H', data[4:6])[0]

            while len(data) < length:
                data += ser.read(1024)

            packet = data[:length]
            data = data[length:]

            if crc16(packet[:-2]) == struct.unpack('<H', packet[-2:])[0]:
                result.append(packet[6:-2])

        return result
