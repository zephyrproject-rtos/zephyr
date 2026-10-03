# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing Beken BK7258 SoCs through the BootROM over UART.

The BootROM listens on UART0 for a short time after reset and speaks a
download protocol framed like HCI vendor commands, though nothing Bluetooth is
involved:

    short command   01 E0 FC <len8>            <cmd> <payload>
    long command    01 E0 FC FF F4 <len16 LE>  <cmd> <payload>
    short response  04 0E <len8>        01 E0 FC    <cmd> <data>
    long response   04 0E FF 01 E0 FC F4 <len16 LE> <cmd> <data>

Which framing a command uses is fixed per command. Only one command may be
outstanding: the ROM drops one that arrives while it is busy. Once it has
answered, the ROM stays in download mode until it is told to reboot. Flash is
erased and written in 4 KB sectors at physical offsets, so the image is written
as is, CRC words included, and each sector is checked against the ROM's CRC-32.

Reset uses the board's two-transistor auto-reset: the chip is held in reset
while RTS is asserted and DTR is not, and runs in every other combination.
'''

import contextlib
import logging
import struct
import sys
import time
import zlib

from runners.core import FileType, RunnerCaps, ZephyrBinaryRunner

SECTOR = 4096
ROM_BAUD = 115200
DEFAULT_BAUD = 1000000
DEFAULT_MONITOR_BAUD = 115200

CMD_LINK_CHECK = 0x00
CMD_READ_REG = 0x03
CMD_WRITE_SECTOR = 0x07
CMD_READ_SECTOR = 0x09
CMD_ERASE_SECTOR = 0x0B
CMD_READ_SR = 0x0C
CMD_WRITE_SR = 0x0D
CMD_JEDEC_ID = 0x0E
CMD_REBOOT = 0x0E
CMD_SET_BAUD = 0x0F
CMD_CRC = 0x10

REG_CHIP_ID = 0x44010004
# The BootROM reports the BK7236 die the BK7258 is built on.
CHIP_FAMILY = 0x7236

# For the trace only. 0x0E is reboot as a short command, JEDEC ID as a long one.
CMD_NAMES = {
    0x00: 'link-check',
    0x01: 'link-check',
    0x03: 'read-reg',
    0x07: 'write-sector',
    0x09: 'read-sector',
    0x0B: 'erase-sector',
    0x0C: 'read-sr',
    0x0D: 'write-sr',
    0x0E: 'jedec-id/reboot',
    0x0F: 'set-baud',
    0x10: 'crc',
}


class FlashError(RuntimeError):
    pass


def rom_crc(data):
    '''CRC-32 as the ROM computes it: no final inversion.'''
    return zlib.crc32(data) ^ 0xFFFFFFFF


class BootRom:
    '''The BootROM download protocol over an open serial port.'''

    def __init__(self, ser, logger):
        self.ser = ser
        self.logger = logger
        self.buf = bytearray()
        # Set while hammering the link check, so the trace is not hundreds of lines.
        self.quiet = False
        self.junk = bytearray()
        self.t0 = time.monotonic()

    @property
    def tracing(self):
        return self.logger.isEnabledFor(logging.DEBUG)

    def log(self, msg):
        if self.tracing and not self.quiet:
            self.logger.debug(f'[{time.monotonic() - self.t0:7.3f}] {msg}')

    @staticmethod
    def hexdump(data):
        if len(data) <= 32:
            return data.hex(' ')
        return f'{data[:32].hex(" ")} ... ({len(data)} B)'

    def set_lines(self, dtr, rts):
        self.ser.dtr = dtr
        self.ser.rts = rts
        self.log(f'lines DTR {"on" if dtr else "off"}, RTS {"on" if rts else "off"}')

    def set_host_baud(self, baud):
        self.ser.baudrate = baud
        self.log(f'host baud {baud}')

    def _send(self, frame, cmd):
        self.log(f'tx {CMD_NAMES.get(cmd, "?")}: {self.hexdump(frame)}')
        self.ser.write(frame)

    def send_short(self, cmd, payload=b''):
        self._send(bytes([0x01, 0xE0, 0xFC, 1 + len(payload), cmd]) + payload, cmd)

    def send_long(self, cmd, payload=b''):
        hdr = bytes([0x01, 0xE0, 0xFC, 0xFF, 0xF4]) + struct.pack('<H', 1 + len(payload))
        self._send(hdr + bytes([cmd]) + payload, cmd)

    def _drop(self, n, why):
        dropped = bytes(self.buf[:n])
        del self.buf[:n]
        if dropped:
            self.junk += dropped
            self.log(f'rx skip ({why}): {self.hexdump(dropped)} {dropped[:32]!r}')

    def drain(self, quiet_s=0.1, limit_s=2.0):
        '''Let an interrupted command finish: the ROM drops a command sent while busy.'''
        self.ser.flush()
        end = time.monotonic() + limit_s
        self.ser.timeout = quiet_s
        while time.monotonic() < end:
            data = self.ser.read(4096)
            if not data:
                break
            self.buf += data
        self._drop(len(self.buf), 'drained')

    def _need(self, n, deadline, cmd):
        while len(self.buf) < n:
            left = deadline - time.monotonic()
            if left <= 0:
                what = 'incomplete' if self.buf else 'no'
                if self.buf:
                    self.log(f'rx timeout, {len(self.buf)} B buffered: {self.hexdump(self.buf)}')
                raise FlashError(f'{what} response to command 0x{cmd:02x}')
            self.ser.timeout = min(left, 0.05)
            self.buf += self.ser.read(max(1, self.ser.in_waiting))

    def recv(self, cmd, timeout):
        '''Return the data of the next response to cmd; skip anything else.'''
        deadline = time.monotonic() + timeout
        while True:
            self._need(2, deadline, cmd)
            i = self.buf.find(b'\x04\x0e')
            if i < 0:
                self._drop(len(self.buf) - 1, 'no 04 0e')
                continue
            self._drop(i, 'before 04 0e')
            self._need(3, deadline, cmd)
            if self.buf[2] == 0xFF:
                self._need(9, deadline, cmd)
                if self.buf[3:7] != b'\x01\xe0\xfc\xf4':
                    self._drop(2, 'bad long header')
                    continue
                n = struct.unpack_from('<H', self.buf, 7)[0]
                start = 9
            else:
                self._need(6, deadline, cmd)
                if self.buf[2] < 4 or self.buf[3:6] != b'\x01\xe0\xfc':
                    self._drop(2, 'bad short header')
                    continue
                n = self.buf[2] - 3
                start = 6
            self._need(start + n, deadline, cmd)
            raw = bytes(self.buf[: start + n])
            frame = raw[start:]
            del self.buf[: start + n]
            if frame and frame[0] == cmd:
                self.log(f'rx {CMD_NAMES.get(cmd, "?")}: {self.hexdump(raw)}')
                return frame[1:]
            self.log(f'rx unexpected (wanted 0x{cmd:02x}): {self.hexdump(raw)}')

    def recv_long_ok(self, cmd, timeout):
        data = self.recv(cmd, timeout)
        if not data or data[0] != 0:
            raise FlashError(f'command 0x{cmd:02x} failed, status {data[:1].hex()}')
        return data[1:]

    def reset_into_rom(self, attempts=3):
        for attempt in range(1, attempts + 1):
            self.log(f'reset attempt {attempt}/{attempts}')
            self.set_host_baud(ROM_BAUD)
            self.set_lines(dtr=False, rts=True)
            time.sleep(0.2)
            self.ser.reset_input_buffer()
            self.buf.clear()
            self.junk.clear()
            self.set_lines(dtr=False, rts=False)
            # The ROM listens only briefly after reset, so hammer the link check.
            t = time.monotonic()
            deadline = t + 1.0
            tries = 0
            answered = False
            self.quiet = True
            try:
                while time.monotonic() < deadline:
                    self.send_short(CMD_LINK_CHECK)
                    tries += 1
                    try:
                        self.recv(0x01, 0.005)
                    except FlashError:
                        continue
                    answered = True
                    break
            finally:
                self.quiet = False
            ms = (time.monotonic() - t) * 1000
            other = f'{len(self.junk)} B of other traffic'
            if self.junk:
                other += f': {bytes(self.junk[:64])!r}'
            if not answered:
                self.log(f'no answer after {tries} link checks in {ms:.0f} ms, {other}')
                continue
            self.log(f'link check answered after {tries} tries, {ms:.0f} ms, {other}')
            # Drain replies to the queued link checks, then confirm once.
            time.sleep(0.02)
            self.ser.reset_input_buffer()
            self.buf.clear()
            self.send_short(CMD_LINK_CHECK)
            self.recv(0x01, 0.1)
            return
        raise FlashError('BootROM did not answer the link check after reset')

    def read_reg(self, addr):
        self.send_short(CMD_READ_REG, struct.pack('<I', addr))
        data = self.recv(CMD_READ_REG, 0.5)
        return struct.unpack('<II', data[:8])[1]

    def jedec_id(self):
        self.send_long(CMD_JEDEC_ID, struct.pack('<I', 0x9F))
        data = self.recv_long_ok(CMD_JEDEC_ID, 0.5)
        return struct.unpack('<I', data[:4])[0] >> 8

    def read_sr(self, reg):
        self.send_long(CMD_READ_SR, bytes([reg]))
        return self.recv_long_ok(CMD_READ_SR, 0.5)[1]

    def write_sr(self, value):
        # Register 0x01 is the flash's write-status opcode, one byte (SR1).
        self.send_long(CMD_WRITE_SR, bytes([0x01, value]))
        self.recv_long_ok(CMD_WRITE_SR, 0.5)

    def set_baud(self, baud, delay_ms=90):
        self.send_short(CMD_SET_BAUD, struct.pack('<IB', baud, delay_ms))
        self.ser.flush()
        # The chip switches before it answers; follow it half-way into the delay.
        time.sleep(delay_ms / 2000)
        self.set_host_baud(baud)
        self._drop(len(self.buf), 'stale at old baud')
        data = self.recv(CMD_SET_BAUD, 1.0)
        if struct.unpack('<I', data[:4])[0] != baud:
            raise FlashError(f'chip did not confirm {baud} baud')

    def read_sector(self, addr):
        self.send_long(CMD_READ_SECTOR, struct.pack('<I', addr))
        data = self.recv_long_ok(CMD_READ_SECTOR, 2.0)
        if struct.unpack('<I', data[:4])[0] != addr or len(data) != 4 + SECTOR:
            raise FlashError(f'bad read of sector 0x{addr:x}')
        return data[4:]

    def erase_sector(self, addr):
        self.send_long(CMD_ERASE_SECTOR, struct.pack('<I', addr))
        self.recv_long_ok(CMD_ERASE_SECTOR, 2.0)

    def write_sector(self, addr, data):
        self.send_long(CMD_WRITE_SECTOR, struct.pack('<I', addr) + data)
        self.recv_long_ok(CMD_WRITE_SECTOR, 2.0)

    def crc(self, addr, size):
        self.send_short(CMD_CRC, struct.pack('<II', addr, addr + size - 1))
        return struct.unpack('<I', self.recv(CMD_CRC, 2.0)[:4])[0]

    def reboot(self):
        self.send_short(CMD_REBOOT, b'\xa5')
        self.ser.flush()

    @contextlib.contextmanager
    def session(self, attempts=3):
        '''Reset into the BootROM, and restart the chip on leaving, error or not.'''
        self.reset_into_rom(attempts)
        try:
            yield
        except BaseException:
            # Ctrl-C included. Let a command in flight finish, as the ROM drops one
            # sent while it is busy, then restart; report the error that got us here.
            with contextlib.suppress(Exception):
                self.drain()
                self.reboot()
            raise
        self.reboot()


def pad_to_sectors(rom, addr, image):
    '''Widen the image to whole sectors with what flash holds around it now.

    Returns the sector-aligned start and the widened data. Call this before
    raising the baud: a 4 KB read back at 2 Mbaud through a CH340 adapter
    intermittently loses bytes, while 115200 is reliable.
    '''
    start = addr - addr % SECTOR
    head = addr - start
    if head:
        image = rom.read_sector(start)[:head] + image
    tail = len(image) % SECTOR
    if tail:
        image += rom.read_sector(start + len(image) - tail)[tail:]
    return start, image


def program(rom, addr, image):
    sectors = range(0, len(image), SECTOR)
    for n, off in enumerate(sectors, 1):
        a = addr + off
        chunk = image[off : off + SECTOR]
        want = rom_crc(chunk)
        for _ in range(3):
            rom.erase_sector(a)
            rom.write_sector(a, chunk)
            got = rom.crc(a, SECTOR)
            rom.log(f'sector 0x{a:06x} crc chip 0x{got:08x} host 0x{want:08x}')
            if got == want:
                break
            rom.logger.warning(f'CRC mismatch at 0x{a:06x}, retrying')
        else:
            raise FlashError(f'sector 0x{a:06x} failed verification')
        if not rom.tracing:
            print(f'\r  0x{a:06x}  {n}/{len(sectors)} sectors', end='', flush=True)
    if not rom.tracing:
        print()


def write_image(rom, addr, baud, image):
    chip = rom.read_reg(REG_CHIP_ID)
    mid = rom.jedec_id()
    rom.logger.info(f'chip id 0x{chip:08x}, flash JEDEC id 0x{mid:06x} ({1 << (mid >> 16)} bytes)')
    if (chip >> 16) != CHIP_FAMILY:
        raise FlashError(f'unexpected chip id 0x{chip:08x}')
    if addr + len(image) > 1 << (mid >> 16):
        raise FlashError('image does not fit in flash')

    start, padded = pad_to_sectors(rom, addr, image)
    rom.log(f'writing sectors 0x{start:x}..0x{start + len(padded) - 1:x}')
    if baud != ROM_BAUD:
        rom.set_baud(baud)

    sr1 = rom.read_sr(0x05)
    sr2 = rom.read_sr(0x35)
    rom.log(f'flash SR1 0x{sr1:02x}, SR2 0x{sr2:02x}; clearing SR1 to unprotect')
    rom.write_sr(0x00)
    try:
        rom.logger.info(f'writing {len(image)} bytes at 0x{addr:x} ({baud} baud)')
        t = time.monotonic()
        program(rom, start, padded)
        rom.logger.info(f'done in {time.monotonic() - t:.1f} s, verified by CRC')
        rom.log(f'restoring SR1 0x{sr1:02x}')
    except BaseException:
        # Ctrl-C included, or the next run reads 0x00 and "restores" that.
        # Best effort, and never mask the error that got us here.
        try:
            rom.drain()
            rom.write_sr(sr1)
        except FlashError:
            rom.logger.warning(f'could not restore status register 0x{sr1:02x}')
        raise
    rom.write_sr(sr1)
    if rom.read_sr(0x35) != sr2:
        rom.logger.warning('flash status register 2 changed')


def monitor(ser, baud):
    ser.baudrate = baud
    ser.timeout = 0.1
    print(f'--- console at {baud}, Ctrl-C to exit ---')
    try:
        while True:
            data = ser.read(256)
            if data:
                sys.stdout.write(data.decode(errors='replace'))
                sys.stdout.flush()
    except KeyboardInterrupt:
        print()


@contextlib.contextmanager
def open_port(port):
    '''An open serial port with both lines released, so the chip runs.'''
    # Imported here: west imports every runner module, whichever one runs.
    import serial

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = ROM_BAUD
    # Set before open so that opening cannot reset the chip.
    ser.dtr = False
    ser.rts = False
    try:
        ser.open()
        yield ser
    except serial.SerialException as e:
        raise FlashError(str(e)) from None
    finally:
        ser.close()


def flash(port, path, logger, offset=0, baud=DEFAULT_BAUD, monitor_baud=None):
    '''Write the image in path at a flash offset, verify it, restart the chip.

    With monitor_baud set, show the chip's console at that baud afterwards until Ctrl-C.
    '''
    with open(path, 'rb') as f:
        image = f.read()
    if not image:
        raise FlashError(f'{path} is empty')
    with open_port(port) as ser:
        rom = BootRom(ser, logger)
        with rom.session():
            write_image(rom, offset, baud, image)
        if monitor_baud:
            monitor(ser, monitor_baud)


def probe(port, logger):
    '''Raise FlashError unless a BK7258 BootROM answers on port; restart it either way.'''
    with open_port(port) as ser:
        rom = BootRom(ser, logger)
        with rom.session(attempts=1):
            chip = rom.read_reg(REG_CHIP_ID)
    if (chip >> 16) != CHIP_FAMILY:
        raise FlashError(f'unexpected chip id 0x{chip:08x}')


class BkflashBinaryRunner(ZephyrBinaryRunner):
    '''Runner for the BK7258 BootROM download protocol.'''

    def __init__(
        self,
        cfg,
        port=None,
        baud=DEFAULT_BAUD,
        offset=0,
        monitor=False,
        monitor_baud=DEFAULT_MONITOR_BAUD,
        dry_run=False,
    ):
        super().__init__(cfg)
        self.port = port
        self.baud = baud
        self.offset = offset
        self.monitor = monitor
        self.monitor_baud = monitor_baud
        self.dry_run = dry_run

    @classmethod
    def name(cls):
        return 'bkflash'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dry_run=True, file=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--port',
            help='serial port; by default USB serial ports are probed for a BK7258, '
            'which resets each board probed',
        )
        parser.add_argument(
            '--baud-rate',
            type=int,
            default=DEFAULT_BAUD,
            help=f'baud rate for the transfer, default {DEFAULT_BAUD}; up to 2000000 '
            'works but reads back unreliably, and erase time dominates anyway',
        )
        parser.add_argument(
            '--flash-offset',
            type=lambda s: int(s, 0),
            default=0,
            help='flash offset to write at, default 0; any byte offset, the flash '
            'around the image is preserved. The BootROM only boots an image at 0',
        )
        parser.add_argument(
            '--monitor', action='store_true', help='show the console after flashing'
        )
        parser.add_argument(
            '--monitor-baud',
            type=int,
            default=DEFAULT_MONITOR_BAUD,
            help='console baud rate for --monitor; the build sets it from the '
            f'zephyr,console current-speed, default {DEFAULT_MONITOR_BAUD}',
        )

    @classmethod
    def do_create(cls, cfg, args):
        if args.flash_offset < 0:
            raise ValueError('--flash-offset must not be negative')
        return BkflashBinaryRunner(
            cfg,
            port=args.port,
            baud=args.baud_rate,
            offset=args.flash_offset,
            monitor=args.monitor,
            monitor_baud=args.monitor_baud,
            dry_run=args.dry_run,
        )

    def _detect_chip(self, port, failures):
        '''Return "bk7258" if one answers on port, after restarting it.

        Otherwise record why in failures, keyed by port, and return None.'''
        try:
            probe(port, self.logger)
        except FlashError as err:
            failures[port] = str(err)
            return None
        return 'bk7258'

    def _resolve_port(self):
        try:
            from serial.tools.list_ports import comports
        except ImportError as err:
            raise RuntimeError(f'cannot list serial ports ({err}); pass --port') from None

        # Probing a port that answers nothing takes a second, and pyserial lists
        # every legacy ttyS* on Linux, so only USB adapters are tried.
        usb = {p.device for p in comports() if p.vid is not None}
        if not usb:
            raise RuntimeError('no USB serial port found; connect the board, or pass --port')

        failures = {}
        port = self.resolve_port_by_chip(
            lambda p: self._detect_chip(p, failures) if p in usb else None,
            'bk7258',
            self.logger,
        )
        if port is None:
            reasons = ''.join(f'\n  {p}: {failures.get(p, "no BK7258")}' for p in sorted(usb))
            raise RuntimeError(
                f'no BK7258 answered on any USB serial port:{reasons}\n'
                'A board held by a terminal program does not answer; close it, or pass --port'
            )
        return port

    def do_run(self, command, **kwargs):
        if self.cfg.file:
            if self.cfg.file_type != FileType.BIN:
                raise ValueError('bkflash only writes binary files, already in on-flash format')
            image = self.cfg.file
        else:
            # The build points bin_file at the image with the flash CRC words
            # (zephyr.crc.bin), not at zephyr.bin.
            self.ensure_output('bin')
            image = self.cfg.bin_file

        if self.dry_run:
            # Probing resets boards, which a dry run must not do.
            port = self.port or 'a probed USB serial port'
            self.logger.info(
                f'would flash {image} at 0x{self.offset:x} through {port}, {self.baud} baud'
            )
            return

        port = self.port or self._resolve_port()
        self.logger.info(f'Flashing {image} through {port}')
        flash(
            port,
            image,
            self.logger,
            offset=self.offset,
            baud=self.baud,
            monitor_baud=self.monitor_baud if self.monitor else None,
        )
