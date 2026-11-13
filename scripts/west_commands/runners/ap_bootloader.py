# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Runner for flight controllers with the ArduPilot bootloader.

The ArduPilot bootloader (AP_Bootloader) speaks the PX4 bootloader protocol over
USB CDC ACM. It only erases and programs the application area that follows it in
flash, so the bootloader itself is never overwritten.
'''

import contextlib
import struct
import sys
import time
import zlib

from runners.core import RunnerCaps, ZephyrBinaryRunner

try:
    import serial
    import serial.tools.list_ports

    MISSING_REQUIREMENTS = False
except ImportError:
    MISSING_REQUIREMENTS = True

# A port that vanishes while it is open, as when the board reboots, raises
# termios.error from pyserial on POSIX hosts, which is not an OSError.
TERMIOS_ERRORS: tuple[type[Exception], ...] = ()
if sys.platform != 'win32':
    import termios

    TERMIOS_ERRORS = (termios.error,)

# Protocol constants, see Tools/AP_Bootloader/bl_protocol.cpp in ArduPilot
INSYNC = 0x12
EOC = 0x20
OK = 0x10
FAILED = 0x11
INVALID = 0x13
GET_SYNC = 0x21
GET_DEVICE = 0x22
CHIP_ERASE = 0x23
PROG_MULTI = 0x27
GET_CRC = 0x29
BOOT = 0x30
INFO_BL_REV = 1
INFO_BOARD_ID = 2
INFO_FW_SIZE = 4
PROG_MULTI_MAX = 252
BL_REV_MIN = 3
BL_REV_MAX = 5

# MAVLink v1 COMMAND_LONG for MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN with param1 = 3
# (reboot into the bootloader), addressed to system 1 and to system 0.
MAVLINK_REBOOT_TO_BOOTLOADER = (
    bytes([254, 33, 114, 255, 0, 76, 0, 0, 64, 64] + [0] * 24 + [246, 0, 1, 0, 0, 83, 107]),
    bytes([254, 33, 69, 255, 0, 76, 0, 0, 64, 64] + [0] * 24 + [246, 0, 0, 0, 0, 204, 55]),
)


def bootloader_crc(data, crc=0):
    '''CRC-32 as computed by the bootloader: no initial or final inversion.'''
    return zlib.crc32(data, crc ^ 0xFFFFFFFF) ^ 0xFFFFFFFF


def parse_usb_id(usb_id):
    '''Parse a hexadecimal VID:PID pair.'''
    if not usb_id:
        return None
    vid, _, pid = usb_id.partition(':')
    return (int(vid, 16), int(pid, 16))


class APBootloaderBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the ArduPilot bootloader.'''

    def __init__(self, cfg, board_id, port=None, usb_id=None, app_usb_id=None, timeout=60.0):
        super().__init__(cfg)
        self.board_id = board_id
        self.port = port
        self.usb_id = usb_id
        self.app_usb_id = app_usb_id
        self.timeout = timeout

    @classmethod
    def name(cls):
        return 'ap_bootloader'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--board-id', type=int, required=True, help='board ID the bootloader must report'
        )
        parser.add_argument(
            '--port', help='serial port of the bootloader, found from --usb-id if omitted'
        )
        parser.add_argument('--usb-id', help='USB VID:PID of the bootloader, in hexadecimal')
        parser.add_argument(
            '--app-usb-id',
            help='USB VID:PID of an ArduPilot application to reboot into the bootloader, '
            'in hexadecimal',
        )
        parser.add_argument(
            '--timeout',
            type=float,
            default=60.0,
            help='seconds to wait for the bootloader (default: %(default)s)',
        )

    @classmethod
    def do_create(cls, cfg, args):
        usb_id = parse_usb_id(args.usb_id)
        if args.port is None and usb_id is None:
            raise ValueError('--port or --usb-id is required')
        return cls(
            cfg,
            args.board_id,
            port=args.port,
            usb_id=usb_id,
            app_usb_id=parse_usb_id(args.app_usb_id),
            timeout=args.timeout,
        )

    def do_run(self, command, **kwargs):
        if MISSING_REQUIREMENTS:
            raise RuntimeError('could not import pyserial; install the Zephyr Python requirements')

        self.ensure_output('bin')
        load_address = self.flash_address_from_build_conf(self.build_conf)
        if load_address == self.build_conf['CONFIG_FLASH_BASE_ADDRESS']:
            raise RuntimeError(
                'the application is linked at the start of flash; '
                'build it to run after the bootloader'
            )

        with open(self.cfg.bin_file, 'rb') as f:
            image = f.read()
        image += b'\xff' * (-len(image) % 4)

        ser = self._connect()
        try:
            self._flash(ser, image)
        finally:
            ser.close()

    @staticmethod
    def _ports_with_usb_id(usb_id):
        return [
            port.device
            for port in serial.tools.list_ports.comports()
            if (port.vid, port.pid) == usb_id
        ]

    def _find_port(self):
        if self.port is not None:
            return self.port
        return next(iter(self._ports_with_usb_id(self.usb_id)), None)

    def _connect(self):
        deadline = time.monotonic() + self.timeout
        rebooted = set()
        waiting = False
        while time.monotonic() < deadline:
            port = self._find_port()
            app_ports = []
            if port is not None:
                ser = self._open_synced(port)
                if ser is not None:
                    return ser
                # The port answers but not as the bootloader, as when
                # ArduPilot runs: ask it to reboot into the bootloader.
                app_ports.append(port)
            elif not waiting:
                self.logger.info('waiting for the bootloader, reset or replug the board')
                waiting = True
            if self.app_usb_id is not None:
                app_ports += self._ports_with_usb_id(self.app_usb_id)
            for app_port in app_ports:
                if app_port not in rebooted:
                    self.logger.info('rebooting the board on %s into the bootloader', app_port)
                    self._send_reboot(app_port)
                    rebooted.add(app_port)
            time.sleep(0.1)
        raise RuntimeError('timed out waiting for the bootloader')

    def _open_synced(self, port):
        try:
            ser = serial.Serial(port, 115200, timeout=0.1, exclusive=True)
        except (serial.SerialException, OSError, *TERMIOS_ERRORS):
            return None
        for _ in range(3):
            try:
                ser.reset_input_buffer()
                ser.write(bytes([GET_SYNC, EOC]))
                self._get_sync(ser, 0.5)
                return ser
            except (RuntimeError, serial.SerialException, OSError, *TERMIOS_ERRORS):
                pass
        ser.close()
        return None

    def _send_reboot(self, port):
        try:
            with serial.Serial(port, 115200, timeout=0.1, exclusive=True) as ser:
                for packet in MAVLINK_REBOOT_TO_BOOTLOADER:
                    ser.write(packet)
                ser.flush()
        except (serial.SerialException, OSError, *TERMIOS_ERRORS):
            pass

    @staticmethod
    def _read(ser, count, timeout):
        deadline = time.monotonic() + timeout
        data = b''
        while len(data) < count:
            if time.monotonic() > deadline:
                raise RuntimeError('no answer from the bootloader')
            data += ser.read(count - len(data))
        return data

    def _get_sync(self, ser, timeout=2.0):
        insync, status = self._read(ser, 2, timeout)
        if insync != INSYNC:
            raise RuntimeError(f'unexpected answer 0x{insync:02x} from the bootloader')
        if status == INVALID:
            raise RuntimeError('the bootloader rejected the command')
        if status == FAILED:
            raise RuntimeError('the bootloader failed to execute the command')
        if status != OK:
            raise RuntimeError(f'unexpected status 0x{status:02x} from the bootloader')

    def _command(self, ser, data, timeout=2.0):
        ser.write(bytes(data))
        self._get_sync(ser, timeout)

    def _get_word(self, ser, data, timeout=2.0):
        ser.write(bytes(data))
        value = struct.unpack('<I', self._read(ser, 4, timeout))[0]
        self._get_sync(ser, timeout)
        return value

    def _flash(self, ser, image):
        bl_rev = self._get_word(ser, [GET_DEVICE, INFO_BL_REV, EOC])
        board_id = self._get_word(ser, [GET_DEVICE, INFO_BOARD_ID, EOC])
        fw_size = self._get_word(ser, [GET_DEVICE, INFO_FW_SIZE, EOC])
        self.logger.info(
            f'bootloader revision {bl_rev}, board ID {board_id}, '
            f'{fw_size} bytes of application flash'
        )

        if not BL_REV_MIN <= bl_rev <= BL_REV_MAX:
            raise RuntimeError(f'unsupported bootloader revision {bl_rev}')
        if board_id != self.board_id:
            raise RuntimeError(f'board ID {board_id} does not match {self.board_id}')
        if len(image) > fw_size:
            raise RuntimeError(f'image of {len(image)} bytes does not fit in {fw_size} bytes')

        self.logger.info('erasing the application area')
        self._command(ser, [CHIP_ERASE, EOC], timeout=60.0)

        self.logger.info(f'programming {len(image)} bytes')
        for offset in range(0, len(image), PROG_MULTI_MAX):
            chunk = image[offset : offset + PROG_MULTI_MAX]
            self._command(ser, bytes([PROG_MULTI, len(chunk)]) + chunk + bytes([EOC]))

        expected = bootloader_crc(image)
        expected = bootloader_crc(b'\xff' * (fw_size - len(image)), expected)
        crc = self._get_word(ser, [GET_CRC, EOC], timeout=30.0)
        if crc != expected:
            raise RuntimeError(f'CRC mismatch: 0x{crc:08x} instead of 0x{expected:08x}')

        self.logger.info('booting the application')
        ser.write(bytes([BOOT, EOC]))
        # The bootloader may drop off USB before acknowledging.
        with contextlib.suppress(RuntimeError, serial.SerialException, OSError):
            self._get_sync(ser, 1.0)
