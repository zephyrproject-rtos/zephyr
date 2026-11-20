# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Runner that stores Zephyr as boot payload through the Microchip HSS CLI.

The Hart Software Services (HSS) is the zeroth stage boot loader of Microchip
PolarFire SoC and PIC64GX devices. It loads a payload from the SD card/eMMC
into DDR and starts it on the U54 harts. This runner turns zephyr.elf into an
HSS payload with hss-payload-generator, sends it over the HSS console UART
using YMODEM, and lets HSS write it to the boot medium.
'''

import binascii
import os
import re
import time
import zlib
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_BAUD = 115200
DEFAULT_GENERATOR = 'hss-payload-generator'
DEFAULT_TIMEOUT = 30.0

# On-board Microchip debugger (embedded FlashPro5): FTDI channel A is JTAG,
# channel B (USB interface 1) is MMUART0, which is the HSS console.
MCHP_DEBUG_VID = 0x1514
MCHP_DEBUG_PID = 0x200A
MCHP_DEBUG_HSS_INTERFACE = '1'

HSS_PROMPT = b'>> '
HSS_PRESS_KEY = b'Press a key to enter CLI'
HSS_MENU_SELECT = b'Select a number:'
HSS_YMODEM_START = b'using YMODEM (CTRL-C to cancel)'
HSS_RECEIVED_RE = re.compile(rb'Received (\d+) bytes from .*? \(CRC32 is 0x([0-9A-Fa-f]+)\)')
HSS_PROGRAM_RE = re.compile(
    rb'Programming MMC \.\.\.\s*(Success|FAILED)|Initializing MMC \.\.\.\s*FAILED'
)
HSS_BOOT_RE = re.compile(rb'Boot image set name: "([^"]*)"|HSS_Boot_VerifyMagic\(\) failed')

CTRL_A = b'\x01'  # resets the HSS CLI line buffer and reprints the prompt

SOH = 0x01
STX = 0x02
EOT = 0x04
ACK = 0x06
NAK = 0x15
CAN = 0x18
CRC_REQUEST = ord('C')


class HssError(RuntimeError):
    '''Communication with the HSS failed.'''


def ymodem_block(number, data, size):
    '''Build one YMODEM (CRC-16) block, padding data to size bytes.'''
    start = SOH if size == 128 else STX
    pad = b'\x00' if number == 0 else b'\x1a'
    payload = data.ljust(size, pad)
    crc = binascii.crc_hqx(payload, 0)
    return bytes([start, number & 0xFF, 0xFF - (number & 0xFF)]) + payload + crc.to_bytes(2, 'big')


def ymodem_header(name, size):
    '''Build YMODEM block 0, carrying the file name and size.'''
    info = name.encode() + b'\x00' + str(size).encode() + b'\x00'
    return ymodem_block(0, info, 128)


class HssConsole:
    '''Line-agnostic reader/writer for the HSS console.'''

    def __init__(self, ser, logger):
        self.ser = ser
        self.logger = logger
        self.buf = b''

    def write(self, data):
        self.ser.write(data)
        self.ser.flush()

    def flush_input(self):
        self.ser.reset_input_buffer()
        self.buf = b''

    def _fill(self):
        chunk = self.ser.read(max(1, self.ser.in_waiting))
        if chunk:
            self.logger.debug(f'hss: {chunk!r}')
            self.buf += chunk
        return chunk

    def expect(self, pattern, timeout):
        '''Wait for a bytes literal or compiled regex in the console output.

        Consumes the output up to and including the match and returns the
        match (a re.Match for regexes, the literal otherwise).'''
        deadline = time.monotonic() + timeout
        while True:
            if isinstance(pattern, re.Pattern):
                match = pattern.search(self.buf)
                if match:
                    self.buf = self.buf[match.end() :]
                    return match
            else:
                idx = self.buf.find(pattern)
                if idx >= 0:
                    self.buf = self.buf[idx + len(pattern) :]
                    return pattern
            if time.monotonic() > deadline:
                return None
            self._fill()

    def read_byte(self, timeout):
        '''Read a single raw byte (used for YMODEM handshakes).'''
        deadline = time.monotonic() + timeout
        while not self.buf:
            if time.monotonic() > deadline:
                return None
            self._fill()
        byte, self.buf = self.buf[0], self.buf[1:]
        return byte


class MchpHssBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the Microchip Hart Software Services (HSS).'''

    def __init__(
        self,
        cfg,
        port=None,
        baud=DEFAULT_BAUD,
        generator=DEFAULT_GENERATOR,
        receive_only=False,
        boot=True,
        timeout=DEFAULT_TIMEOUT,
    ):
        super().__init__(cfg)
        self.port = port
        self.baud = baud
        self.generator = generator
        self.receive_only = receive_only
        self.boot = boot
        self.timeout = timeout

    @classmethod
    def name(cls):
        return 'mchp_hss'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--port',
            help='''HSS console serial port (MMUART0); default:
                            auto-detect the on-board Microchip debugger''',
        )
        parser.add_argument(
            '--baud',
            type=int,
            default=DEFAULT_BAUD,
            help=f'HSS console baud rate, default {DEFAULT_BAUD}',
        )
        parser.add_argument(
            '--generator',
            default=os.environ.get('HSS_PAYLOAD_GENERATOR', DEFAULT_GENERATOR),
            help='''hss-payload-generator executable; default:
                            $HSS_PAYLOAD_GENERATOR or hss-payload-generator
                            from PATH''',
        )
        parser.add_argument(
            '--receive-only',
            action='store_true',
            help='''transfer the payload to HSS and check it,
                            but do not write it to the boot medium''',
        )
        parser.add_argument(
            '--no-boot',
            dest='boot',
            action='store_false',
            help='leave HSS in its CLI instead of booting',
        )
        parser.add_argument(
            '--timeout',
            type=float,
            default=DEFAULT_TIMEOUT,
            help=f'''seconds to wait for HSS to come out of
                            reset, default {DEFAULT_TIMEOUT}''',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return MchpHssBinaryRunner(
            cfg,
            port=args.port,
            baud=args.baud,
            generator=args.generator,
            receive_only=args.receive_only,
            boot=args.boot,
            timeout=args.timeout,
        )

    def do_run(self, command, **kwargs):
        if self.cfg.elf_file is None or not os.path.isfile(self.cfg.elf_file):
            raise ValueError('Cannot flash; zephyr.elf not found')

        payload = self.make_payload()
        port = self.port or self.find_port()

        import serial

        try:
            ser = serial.Serial(port, self.baud, timeout=0.05, exclusive=True)
        except serial.SerialException as e:
            raise HssError(
                f'cannot open {port} ({e}); close any terminal using the HSS console first'
            ) from e
        with ser:
            self.flash_over_console(HssConsole(ser, self.logger), payload)

    def hss_harts(self):
        '''Return the HSS names of the harts this image runs on.'''
        boot_hart = int(self.build_conf.get('CONFIG_RV_BOOT_HART', 0))
        if boot_hart == 0:
            raise ValueError(
                'HSS can only boot payloads on the U54 harts; '
                'this image targets the E51 monitor core'
            )
        count = 1
        if self.build_conf.getboolean('CONFIG_SMP'):
            count = int(self.build_conf.get('CONFIG_MP_MAX_NUM_CPUS', 1))
        return [f'u54_{boot_hart + i}' for i in range(count)]

    def payload_config(self, elf_file, entry):
        '''Return the hss-payload-generator YAML configuration.'''
        harts = self.hss_harts()
        entry_points = ', '.join(f"{h}: '0x{entry:x}'" for h in harts)
        options = [f"exec-addr: '0x{entry:x}'", f'owner-hart: {harts[0]}']
        options += [f'secondary-hart: {h}' for h in harts[1:]]
        options += ['priv-mode: prv_m', 'skip-opensbi: true', "payload-name: 'zephyr'"]
        return (
            f"set-name: 'Zephyr'\n"
            f'hart-entry-points: {{{entry_points}}}\n'
            f'payloads:\n'
            f'  "{elf_file}": {{{", ".join(options)}}}\n'
        )

    def make_payload(self):
        '''Generate build/zephyr/hss-payload.bin and return its content.'''
        from elftools.elf.elffile import ELFFile

        generator = self.require(self.generator)
        elf_file = Path(self.cfg.elf_file).resolve()
        with open(elf_file, 'rb') as f:
            entry = ELFFile(f).header['e_entry']

        out_dir = elf_file.parent
        yaml_file = out_dir / 'hss-payload.yaml'
        bin_file = out_dir / 'hss-payload.bin'
        yaml_file.write_text(self.payload_config(elf_file, entry))
        bin_file.unlink(missing_ok=True)
        self.check_call([generator, '-c', str(yaml_file), str(bin_file)])
        if not bin_file.is_file():
            raise HssError(f'{generator} did not create {bin_file}')
        self.logger.info(f'HSS payload: {bin_file}')
        return bin_file.read_bytes()

    def find_port(self):
        '''Find the HSS console of the on-board Microchip debugger.'''
        import serial.tools.list_ports

        for info in sorted(serial.tools.list_ports.comports(), key=lambda p: p.device):
            if (info.vid, info.pid) != (MCHP_DEBUG_VID, MCHP_DEBUG_PID):
                continue
            interface = (info.location or '').rpartition('.')[2]
            if interface == MCHP_DEBUG_HSS_INTERFACE:
                self.logger.info(f'Using HSS console {info.device}')
                return info.device
        raise HssError(
            'Microchip on-board debugger not found; pass --port with the HSS console (MMUART0)'
        )

    def flash_over_console(self, con, payload):
        self.enter_cli(con)

        con.write(b'YMODEM\r')
        self.expect(con, HSS_MENU_SELECT, 5, 'the YMODEM menu')
        con.write(b'3')
        self.expect(con, HSS_YMODEM_START, 5, 'YMODEM receive')
        self.ymodem_send(con, 'hss-payload.bin', payload)

        received = self.expect(con, HSS_RECEIVED_RE, 15, 'the transfer result')
        size, crc = int(received.group(1)), int(received.group(2), 16)
        if size != len(payload) or crc != zlib.crc32(payload):
            raise HssError(
                f'HSS received {size} bytes with CRC32 0x{crc:08x}, '
                f'expected {len(payload)} bytes with CRC32 '
                f'0x{zlib.crc32(payload):08x}'
            )
        self.logger.info(f'Transfer verified ({size} bytes, CRC32 0x{crc:08x})')
        self.expect(con, HSS_MENU_SELECT, 5, 'the YMODEM menu')

        if not self.receive_only:
            self.logger.warning(
                'Writing payload to offset 0 of the boot medium; this '
                'overwrites its partition table'
            )
            con.write(b'5')
            result = self.expect(con, HSS_PROGRAM_RE, 120, 'the MMC write')
            if result.group(1) != b'Success':
                raise HssError('HSS failed to write the payload')
            self.expect(con, HSS_MENU_SELECT, 5, 'the YMODEM menu')

        con.write(b'6')
        self.expect(con, HSS_PROMPT, 5, 'the HSS prompt')

        if self.boot:
            con.write(b'BOOT\r')
            booted = self.expect(con, HSS_BOOT_RE, 15, 'the boot image')
            if booted.group(1) is None:
                raise HssError('HSS found no valid boot image')
            self.logger.info(f'HSS is booting "{booted.group(1).decode()}"')
        else:
            self.logger.info('HSS is waiting in its CLI')

    def enter_cli(self, con):
        '''Reset the board and stop HSS in its CLI before it boots.'''
        con.flush_input()
        con.write(CTRL_A)
        if con.expect(HSS_PROMPT, 1) is not None:
            self.logger.info('Resetting the board through the HSS CLI')
            con.write(b'RESET\r')
        else:
            self.logger.warning('HSS CLI not responding; press the board reset button')

        self.expect(con, HSS_PRESS_KEY, self.timeout, 'HSS to restart')
        con.write(b' ')
        self.expect(con, HSS_PROMPT, 5, 'the HSS prompt')

    @staticmethod
    def expect(con, pattern, timeout, what):
        match = con.expect(pattern, timeout)
        if match is None:
            raise HssError(f'timed out waiting for {what}')
        return match

    def ymodem_send(self, con, name, data):
        '''Send data with YMODEM-1K (CRC-16), as received by HSS.'''
        deadline = time.monotonic() + 15
        while True:
            byte = con.read_byte(max(0, deadline - time.monotonic()))
            if byte is None:
                raise HssError('YMODEM receiver did not start')
            if byte == CRC_REQUEST:
                break

        self.ymodem_packet(con, ymodem_header(name, len(data)), 'header')

        blocks = (len(data) + 1023) // 1024
        step = max(1, -(-blocks // 10))
        for i in range(blocks):
            block = ymodem_block(i + 1, data[i * 1024 : (i + 1) * 1024], 1024)
            self.ymodem_packet(con, block, f'block {i + 1}')
            if (i + 1) % step == 0 or i + 1 == blocks:
                self.logger.info(f'Sent {min((i + 1) * 1024, len(data))}/{len(data)} bytes')

        # HSS reads a full packet header after EOT before it ACKs.
        self.ymodem_packet(con, bytes([EOT]), 'EOT', timeout=15)

    @staticmethod
    def ymodem_packet(con, packet, what, timeout=10, retries=10):
        for _ in range(retries):
            con.write(packet)
            deadline = time.monotonic() + timeout
            while True:
                byte = con.read_byte(max(0, deadline - time.monotonic()))
                if byte in (ACK, NAK, None):
                    break
                if byte == CAN and con.read_byte(1) == CAN:
                    raise HssError(f'YMODEM transfer cancelled by HSS at {what}')
                # ignore stray 'C' and console noise
            if byte == ACK:
                return
        raise HssError(f'YMODEM: no ACK for {what}')
