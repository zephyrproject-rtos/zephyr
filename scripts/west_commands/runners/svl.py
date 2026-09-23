# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with the SparkFun Variable Loader (SVL) serial bootloader.'''

import time

from runners.core import RunnerCaps, ZephyrBinaryRunner

try:
    import serial

    MISSING_REQUIREMENTS = False
except ImportError:
    MISSING_REQUIREMENTS = True

SVL_CMD_VERSION = 0x01
SVL_CMD_BLMODE = 0x02
SVL_CMD_NEXT = 0x03
SVL_CMD_FRAME = 0x04
SVL_CMD_RETRY = 0x05
SVL_CMD_DONE = 0x06

SVL_LOAD_ADDRESS = 0x10000
SVL_FRAME_SIZE = 2048

RESET_PULSE_S = 0.01

SVL_BAUD_RATES = (57600, 115200, 230400, 460800, 921600)
DEFAULT_BAUD = 115200
PORT_TIMEOUT_S = 0.5
SESSION_ATTEMPTS = 3

CRC16_POLY = 0x8005


class SvlError(RuntimeError):
    '''Error in an SVL protocol exchange.'''


def crc16(data: bytes) -> int:
    '''Return the CRC-16/BUYPASS of data.'''
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ CRC16_POLY) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def encode_packet(cmd: int, data: bytes = b'') -> bytes:
    '''Return the wire encoding of an SVL packet.'''
    body = bytes([cmd]) + data
    return (len(body) + 2).to_bytes(2, 'big') + body + crc16(body).to_bytes(2, 'big')


def read_packet(port) -> tuple[int, bytes]:
    '''Read one SVL packet from port and return (cmd, data).'''
    header = port.read(2)
    if len(header) != 2:
        raise SvlError('timeout waiting for packet header')
    length = int.from_bytes(header, 'big')
    if length < 3:
        raise SvlError(f'invalid packet length {length}')
    body = port.read(length)
    if len(body) != length:
        raise SvlError(f'timeout: got {len(body)} of {length} packet bytes')
    if crc16(body) != 0:
        raise SvlError('packet CRC mismatch')
    return body[0], body[1:-2]


class SvlUploader:
    '''One SVL upload session over an open serial port.'''

    def __init__(self, port, logger, reset_delay=0.15, max_frame_retries=4):
        self.port = port
        self.logger = logger
        self.reset_delay = reset_delay
        self.max_frame_retries = max_frame_retries

    def enter_bootloader(self) -> int:
        '''Reset the target into SVL, lock it in bootload mode, and return the SVL version.'''
        self.port.dtr = False
        time.sleep(RESET_PULSE_S)
        self.port.dtr = True
        time.sleep(self.reset_delay)
        self.port.reset_input_buffer()
        self.port.write(b'U')
        cmd, data = read_packet(self.port)
        if cmd != SVL_CMD_VERSION:
            raise SvlError(f'expected VERSION packet, got command 0x{cmd:02x}')
        if len(data) != 1:
            raise SvlError(f'VERSION packet has {len(data)} payload bytes, expected 1')
        self.port.write(encode_packet(SVL_CMD_BLMODE))
        return data[0]

    def upload(self, image: bytes) -> None:
        '''Send image frame by frame at the target's request, then end the session.'''
        image += b'\xff' * (-len(image) % 4)
        frames = [image[i : i + SVL_FRAME_SIZE] for i in range(0, len(image), SVL_FRAME_SIZE)]
        index = -1
        retries = 0
        while True:
            cmd, _ = read_packet(self.port)
            if cmd == SVL_CMD_NEXT:
                index += 1
                retries = 0
            elif cmd == SVL_CMD_RETRY and index >= 0:
                retries += 1
                if retries > self.max_frame_retries:
                    raise SvlError(f'frame {index} rejected {retries} times')
            else:
                raise SvlError(f'unexpected command 0x{cmd:02x} at frame {index}')
            if index == len(frames):
                self.port.write(encode_packet(SVL_CMD_DONE))
                return
            self.logger.debug(f'sending frame {index + 1}/{len(frames)}')
            self.port.write(encode_packet(SVL_CMD_FRAME, frames[index]))


class SvlBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the SparkFun Variable Loader.'''

    def __init__(self, cfg, port, baud=DEFAULT_BAUD):
        super().__init__(cfg)
        if MISSING_REQUIREMENTS:
            raise RuntimeError('pyserial is missing; install the Zephyr Python requirements')
        self.port = port
        self.baud = baud

    @classmethod
    def name(cls):
        return 'svl'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--port', required=True, help='serial port of the board')
        parser.add_argument(
            '--baud',
            type=int,
            default=DEFAULT_BAUD,
            choices=SVL_BAUD_RATES,
            help=f'upload baud rate, default {DEFAULT_BAUD}',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return cls(cfg, args.port, baud=args.baud)

    def do_run(self, command, **kwargs):
        self.ensure_output('bin')
        with open(self.cfg.bin_file, 'rb') as f:
            image = f.read()
        self.check_image(image)
        with serial.Serial(self.port, self.baud, timeout=PORT_TIMEOUT_S) as port:
            uploader = SvlUploader(port, self.logger)
            version = self.enter_bootloader(uploader)
            self.logger.info(f'SVL version {version}, uploading {len(image)} bytes')
            uploader.upload(image)
        self.logger.info('upload complete')

    def check_image(self, image: bytes) -> None:
        address = self.flash_address_from_build_conf(self.build_conf)
        if address != SVL_LOAD_ADDRESS:
            raise RuntimeError(
                f'image is linked at 0x{address:x}, but SVL loads at 0x{SVL_LOAD_ADDRESS:x}'
            )
        capacity = self.build_conf['CONFIG_FLASH_SIZE'] * 1024 - SVL_LOAD_ADDRESS
        if len(image) > capacity:
            raise RuntimeError(f'image is {len(image)} bytes, but SVL has room for {capacity}')

    def enter_bootloader(self, uploader: SvlUploader) -> int:
        for attempt in range(1, SESSION_ATTEMPTS + 1):
            try:
                return uploader.enter_bootloader()
            except SvlError as e:
                self.logger.warning(f'no SVL response, attempt {attempt}/{SESSION_ATTEMPTS}: {e}')
        raise SvlError('target did not enter SVL; check --port and that the board runs SVL')
