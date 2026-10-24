# Copyright (c) 2026 Beken Corporation
# SPDX-License-Identifier: Apache-2.0

'''Beken BKFIL runner (flash only) for BK7258 and other Beken SoCs.

This runner wraps the vendor-provided BKFIL command line tool
``bk_loader`` (bundled with the BKFIL distribution, available for
Windows, macOS and Linux from Beken):

    https://dl.bekencorp.com/tools/bkfil/v4

The BK7258 boots through a first-stage bootloader and a system
controller firmware that ship with the SoC.  Zephyr targets the
application core whose image lives in the ``app1`` flash partition at
offset 0x165000.  The raw ``zephyr.bin`` must be converted into the
vendor CRC-expanded package format before it can be programmed; use
``--package-script``/``--pad-script`` to let the runner do it, or pass
a ready-to-flash file with ``--image``.

Example (WSL host driving the Windows CLI binary through WSL
interop):

    west flash -p bk7258_evb --runner beken \
        --bkfil-tool /mnt/c/BKFIL_windows_4.1.6/windows/bk_loader/bk_loader.dist/bk_loader.exe \
        --port COM17 \
        --image /mnt/c/temp/all-app-pad.bin
'''

import os
import pathlib
import re

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_BAUDRATE = '2000000'
DEFAULT_RESET_TYPE = '3'
DEFAULT_RESET_BAUDRATE = '115200'
DEFAULT_ERASE_RANGE = '0-286000'

# Offset of the application partition (app1) in flash.  The bootloader
# and system controller firmware live below it; --app1-only only
# touches [app1_offset, 0x286000).
APP1_OFFSET = 0x165000
APP1_ERASE_END = 0x286000

# The vendor CLI accepts Windows-style paths.  When the runner executes
# under WSL (interop), translate /mnt/<drive>/... paths to <drive>:/...
# so the Windows binary can open the staged image.
_MNT_RE = re.compile(r'^/mnt/([a-zA-Z])/(.*)$')


def to_host_path(path):
    match = _MNT_RE.match(str(path))
    if match:
        return f'{match.group(1).upper()}:/{match.group(2)}'
    return str(path)


class BekenBkfilRunner(ZephyrBinaryRunner):
    '''Runner front-end for the Beken BKFIL bk_loader CLI.'''

    def __init__(
        self,
        cfg,
        tool,
        port,
        baudrate,
        reset_type,
        reset_baudrate,
        erase_range,
        erase,
        image,
        package_script,
        package_out,
        pad_script,
        staging,
        app1_only,
    ):
        super().__init__(cfg)
        self.tool = tool
        self.port = port
        self.baudrate = baudrate
        self.reset_type = reset_type
        self.reset_baudrate = reset_baudrate
        self.erase_range = erase_range
        self.erase = erase
        self.image = image
        self.package_script = package_script
        self.package_out = package_out
        self.pad_script = pad_script
        self.staging = staging
        self.app1_only = app1_only

    @classmethod
    def name(cls):
        return 'beken'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--bkfil-tool',
            default=os.environ.get('BKFIL_TOOL', 'bk_loader'),
            help='path to the bk_loader CLI; default is '
            'the BKFIL_TOOL environment variable or "bk_loader" on PATH',
        )
        parser.add_argument(
            '--port',
            default=os.environ.get('BKFIL_PORT'),
            help='serial port of the board, e.g. COM17 or '
            '/dev/ttyUSB0; default is the BKFIL_PORT '
            'environment variable',
        )
        parser.add_argument(
            '--baudrate',
            default=DEFAULT_BAUDRATE,
            help='download UART speed, default is ' + DEFAULT_BAUDRATE,
        )
        parser.add_argument(
            '--bkfil-reset-type',
            default=DEFAULT_RESET_TYPE,
            help='chip reset method passed to bk_loader, default is ' + DEFAULT_RESET_TYPE,
        )
        parser.add_argument(
            '--bkfil-reset-baudrate',
            default=DEFAULT_RESET_BAUDRATE,
            help='console speed used by bk_loader to trigger '
            'the ROM download mode, default is ' + DEFAULT_RESET_BAUDRATE,
        )
        parser.add_argument(
            '--erase-range',
            default=DEFAULT_ERASE_RANGE,
            help='flash range erased before download '
            '(never covers the vendor radio calibration '
            'sector), default is ' + DEFAULT_ERASE_RANGE,
        )
        parser.add_argument(
            '--image',
            help='flash-ready packaged image; if omitted, '
            'the packaging scripts below produce it from '
            'the build output',
        )
        parser.add_argument(
            '--package-script',
            help='path to mk_zephyr_package.py, which packs '
            'zephyr.bin into the vendor CRC-expanded format',
        )
        parser.add_argument(
            '--package-out', help='path where --package-script writes the packaged (unpadded) image'
        )
        parser.add_argument(
            '--pad-script',
            help='path to pad_4k.py, which pads the packaged '
            'image to a 4 KiB boundary (the loader silently '
            'truncates unpadded images)',
        )
        parser.add_argument(
            '--staging',
            help='where to write the final flash-ready image; '
            'use a location the bk_loader binary can access '
            '(e.g. a /mnt/c/... path when driving the Windows '
            'CLI from WSL)',
        )
        parser.add_argument(
            '--app1-only',
            action='store_true',
            help='program only the application partition: '
            'extract the app1 section (offset ' + hex(APP1_OFFSET) + ') from the packaged image, '
            'pad it and write it with download -s '
            + hex(APP1_OFFSET)
            + '; the bootloader and system '
            'controller firmware below app1 are left '
            'untouched, so they must already be present in '
            'flash (they ship with the board)',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return BekenBkfilRunner(
            cfg,
            args.bkfil_tool,
            args.port,
            args.baudrate,
            args.bkfil_reset_type,
            args.bkfil_reset_baudrate,
            args.erase_range,
            args.erase,
            args.image,
            args.package_script,
            args.package_out,
            args.pad_script,
            args.staging,
            args.app1_only,
        )

    def _run_tool(self, sub, extra):
        cmd = [
            self.tool,
            sub,
            '-p',
            self.port,
            '-b',
            self.baudrate,
            '--reset_type',
            self.reset_type,
            '--reset_baudrate',
            self.reset_baudrate,
        ] + extra
        self.logger.info('bk_loader %s', ' '.join(cmd))
        self.check_call(cmd)

    def prepare_image(self):
        '''Produce the flash-ready image from the build output.'''
        if self.image:
            return pathlib.Path(self.image)

        if not (self.package_script and self.package_out and self.pad_script):
            self.logger.error(
                'either --image or --package-script + --package-out + '
                '--pad-script are required: zephyr.bin is not directly '
                'programmable, it must be converted to the vendor '
                'CRC-expanded package format first'
            )
            raise RuntimeError('missing packaging inputs')

        self.check_call(['python3', self.package_script, f'--app1={self.cfg.bin_file}'])
        staged = self.staging or os.path.join(self.cfg.build_dir, 'beken_flash', 'all-app-pad.bin')
        os.makedirs(os.path.dirname(staged), exist_ok=True)
        self.check_call(['python3', self.pad_script, self.package_out, staged])
        return pathlib.Path(staged)

    def extract_app1(self, image):
        '''Cut the app1 section out of a packaged image and pad it.'''
        with open(image, 'rb') as f:
            data = f.read()
        if len(data) <= APP1_OFFSET:
            raise RuntimeError('packaged image smaller than app1 offset ' + hex(APP1_OFFSET))
        section = data[APP1_OFFSET:]
        pad = (4096 - len(section) % 4096) % 4096
        out = self.staging or os.path.join(self.cfg.build_dir, 'beken_flash', 'app1-pad.bin')
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, 'wb') as f:
            f.write(section + b'\xff' * pad)
        self.logger.info(
            'app1 section: %d B -> %s (%d B, padded %d B)',
            len(section),
            out,
            len(section) + pad,
            pad,
        )
        return out

    def do_run(self, command, **kwargs):
        if not self.port:
            raise RuntimeError('--port (or BKFIL_PORT) is required')
        if command != 'flash':
            raise RuntimeError('only flash is supported')

        self.require(self.tool)
        image = self.prepare_image()
        self.logger.info('flash-ready image: %s', image)

        if self.app1_only:
            image = self.extract_app1(image)
            erase_range = f'{APP1_OFFSET:x}-{APP1_ERASE_END:x}'
        else:
            erase_range = self.erase_range

        if self.erase:
            self._run_tool('erase', ['-f', erase_range])
        start = f'{APP1_OFFSET:x}' if self.app1_only else '0x0'
        self._run_tool('download', ['-r', '-s', start, '-i', to_host_path(image)])
