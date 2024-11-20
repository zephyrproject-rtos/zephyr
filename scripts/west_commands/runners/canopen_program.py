# Copyright (c) 2020 Vestas Wind Systems A/S
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for performing program download over CANopen (DSP 302-3).'''

import argparse
import os
import time

from runners.core import ZephyrBinaryRunner, RunnerCaps

try:
    import canopen
    from progress.bar import Bar
    MISSING_REQUIREMENTS = False
except ImportError:
    MISSING_REQUIREMENTS = True

# Default Python-CAN context to use, see python-can documentation for details
DEFAULT_CAN_CONTEXT = 'default'

# Default program number
DEFAULT_PROGRAM_NUMBER = 1

# Program download buffer size in bytes
DEFAULT_PROGRAM_DOWNLOAD_BUFFER_SIZE = 1024

# Default timeouts and retries
DEFAULT_TIMEOUT = 10.0 # seconds
DEFAULT_SDO_TIMEOUT = 1 # seconds
DEFAULT_SDO_RETRIES = 1

# Object dictionary indexes
H1F50_PROGRAM_DATA = 0x1F50
H1F51_PROGRAM_CTRL = 0x1F51
H1F56_PROGRAM_SWID = 0x1F56
H1F57_FLASH_STATUS = 0x1F57

# Program control commands
PROGRAM_CTRL_STOP = 0x00
PROGRAM_CTRL_START = 0x01
PROGRAM_CTRL_RESET = 0x02
PROGRAM_CTRL_CLEAR = 0x03
PROGRAM_CTRL_ZEPHYR_CONFIRM = 0x80

class ToggleAction(argparse.Action):
    '''Toggle argument parser'''
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, not option_string.startswith('--no-'))

class CANopenBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for CANopen.'''
    def __init__(self, cfg, dev_id, can_context=DEFAULT_CAN_CONTEXT,
                 program_number=DEFAULT_PROGRAM_NUMBER, confirm=True,
                 confirm_only=True, timeout=DEFAULT_TIMEOUT,
                 sdo_retries=DEFAULT_SDO_RETRIES, sdo_timeout=DEFAULT_SDO_TIMEOUT,
                 download_buffer_size=DEFAULT_PROGRAM_DOWNLOAD_BUFFER_SIZE, block_transfer=False):
        if MISSING_REQUIREMENTS:
            raise RuntimeError('one or more Python dependencies were missing; '
                               "see the getting started guide for details on "
                               "how to fix")

        super().__init__(cfg)
        self.dev_id = dev_id # Only use for error checking in do_run()
        self.bin_file = cfg.bin_file
        self.confirm = confirm
        self.confirm_only = confirm_only
        self.timeout = timeout
        self.downloader = CANopenProgramDownloader(logger=self.logger,
                                                   node_id=dev_id,
                                                   can_context=can_context,
                                                   program_number=program_number,
                                                   sdo_retries=sdo_retries,
                                                   sdo_timeout=sdo_timeout,
                                                   download_buffer_size=download_buffer_size,
                                                   block_transfer=block_transfer,
                                                   )

    @classmethod
    def name(cls):
        return 'canopen'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dev_id=True, flash_addr=False)

    @classmethod
    def dev_id_help(cls) -> str:
        return 'CANopen Node ID.'

    @classmethod
    def do_add_parser(cls, parser):
        # Optional:
        parser.add_argument('--node-id', dest='dev_id',
                            help=cls.dev_id_help())

        parser.add_argument('--can-context', default=DEFAULT_CAN_CONTEXT,
                            help=f'Python-CAN context to use (default: {DEFAULT_CAN_CONTEXT})')
        parser.add_argument('--program-number', type=int, default=DEFAULT_PROGRAM_NUMBER,
                            help=f'program number (default: {DEFAULT_PROGRAM_NUMBER})')
        parser.add_argument('--confirm', '--no-confirm',
                            dest='confirm', nargs=0,
                            action=ToggleAction,
                            help='confirm after starting? (default: yes)')
        parser.add_argument('--confirm-only', default=False, action='store_true',
                            help='confirm only, no program download (default: no)')
        parser.add_argument('--timeout', type=float, default=DEFAULT_TIMEOUT,
                            help=f'Timeout in seconds (default: {DEFAULT_TIMEOUT})')
        parser.add_argument('--sdo-retries', type=int, default=DEFAULT_SDO_RETRIES,
                            help=f'CANopen SDO request retries (default: {DEFAULT_SDO_RETRIES})')
        parser.add_argument('--sdo-timeout', type=float, default=DEFAULT_SDO_TIMEOUT,
                            help=f'''CANopen SDO response timeout in seconds
                            (default: {DEFAULT_SDO_TIMEOUT})''')
        parser.add_argument('--download-buffer-size', type=int, default=DEFAULT_PROGRAM_DOWNLOAD_BUFFER_SIZE,
                            help=f'Program download buffer size in bytes (default: {DEFAULT_PROGRAM_DOWNLOAD_BUFFER_SIZE})')
        parser.add_argument('--block-transfer', default=False, action='store_true',
                            help='Use SDO block transfers (experimental, default: no)')

        parser.set_defaults(confirm=True)

    @classmethod
    def do_create(cls, cfg, args):
        return CANopenBinaryRunner(cfg, int(args.dev_id),
                                   can_context=args.can_context,
                                   program_number=args.program_number,
                                   confirm=args.confirm,
                                   confirm_only=args.confirm_only,
                                   timeout=args.timeout,
                                   sdo_retries=args.sdo_retries,
                                   sdo_timeout=args.sdo_timeout,
                                   download_buffer_size=args.download_buffer_size,
                                   block_transfer=args.block_transfer)

    def do_run(self, command, **kwargs):
        if not self.dev_id:
            raise RuntimeError('Please specify a CANopen node ID with the '
                               '-i/--dev-id or --node-id command-line switch.')
        if command == 'flash':
            self.flash(**kwargs)

    def flash(self, **kwargs):
        '''Download program to flash over CANopen'''
        self.ensure_output('bin')
        self.logger.info('Using Node ID %d, program number %d',
                         self.downloader.node_id,
                         self.downloader.program_number)

        self.downloader.connect()
        status = self.downloader.wait_for_flash_status_ok(self.timeout)
        if status == 0:
            self.downloader.swid()
        else:
            self.logger.warning('Flash status 0x{:02x}, '
                                'skipping software identification'.format(status))

        self.downloader.enter_pre_operational()

        if self.confirm_only:
            self.downloader.zephyr_confirm_program()
            self.downloader.disconnect()
            return

        if self.bin_file is None:
            raise ValueError('Cannot download program; bin_file is missing')

        self.downloader.stop_program()
        self.downloader.clear_program()
        self.downloader.wait_for_flash_status_ok(self.timeout)
        self.downloader.download(self.bin_file)

        status = self.downloader.wait_for_flash_status_ok(self.timeout)
        if status != 0:
            raise ValueError('Program download failed: '
                             'flash status 0x{:02x}'.format(status))

        self.downloader.swid()
        self.downloader.start_program()
        self.downloader.wait_for_bootup(self.timeout)
        self.downloader.swid()

        if self.confirm:
            self.downloader.enter_pre_operational()
            self.downloader.zephyr_confirm_program()

        self.downloader.disconnect()

class CANopenProgramDownloader(object):
    '''CANopen program downloader'''
    def __init__(self, logger, node_id, can_context=DEFAULT_CAN_CONTEXT,
                 program_number=DEFAULT_PROGRAM_NUMBER,
                 sdo_retries=DEFAULT_SDO_RETRIES, sdo_timeout=DEFAULT_SDO_TIMEOUT,
                 download_buffer_size=DEFAULT_PROGRAM_DOWNLOAD_BUFFER_SIZE, block_transfer=False):
        super(CANopenProgramDownloader, self).__init__()
        self.logger = logger
        self.node_id = node_id
        self.can_context = can_context
        self.program_number = program_number
        self.network = canopen.Network()
        self.node = self.network.add_node(self.node_id,
                                          self.create_object_dictionary())
        self.data_sdo = self.node.sdo[H1F50_PROGRAM_DATA][self.program_number]
        self.ctrl_sdo = self.node.sdo[H1F51_PROGRAM_CTRL][self.program_number]
        self.swid_sdo = self.node.sdo[H1F56_PROGRAM_SWID][self.program_number]
        self.flash_sdo = self.node.sdo[H1F57_FLASH_STATUS][self.program_number]
        self.download_buffer_size = download_buffer_size

        self.node.sdo.MAX_RETRIES = sdo_retries
        self.node.sdo.RESPONSE_TIMEOUT = sdo_timeout

        self.block_transfer = block_transfer

    def connect(self):
        '''Connect to CAN network'''
        try:
            self.network.connect(context=self.can_context)
        except:
            raise ValueError('Unable to connect to CAN network')

    def disconnect(self):
        '''Disconnect from CAN network'''
        self.network.disconnect()

    def enter_pre_operational(self):
        '''Enter pre-operational NMT state'''
        self.logger.info("Entering pre-operational mode")
        try:
            self.node.nmt.state = 'PRE-OPERATIONAL'
        except:
            raise ValueError('Failed to enter pre-operational mode')

    def _ctrl_program(self, cmd):
        '''Write program control command to CANopen object dictionary (0x1f51)'''
        try:
            self.ctrl_sdo.raw = cmd
        except:
            raise ValueError('Unable to write control command 0x{:02x}'.format(cmd))

    def stop_program(self):
        '''Write stop control command to CANopen object dictionary (0x1f51)'''
        self.logger.info('Stopping program')
        self._ctrl_program(PROGRAM_CTRL_STOP)

    def start_program(self):
        '''Write start control command to CANopen object dictionary (0x1f51)'''
        self.logger.info('Starting program')
        self._ctrl_program(PROGRAM_CTRL_START)

    def clear_program(self):
        '''Write clear control command to CANopen object dictionary (0x1f51)'''
        self.logger.info('Clearing program')
        self._ctrl_program(PROGRAM_CTRL_CLEAR)

    def zephyr_confirm_program(self):
        '''Write confirm control command to CANopen object dictionary (0x1f51)'''
        self.logger.info('Confirming program')
        self._ctrl_program(PROGRAM_CTRL_ZEPHYR_CONFIRM)

    def swid(self):
        '''Read software identification from CANopen object dictionary (0x1f56)'''
        try:
            swid = self.swid_sdo.raw
        except:
            raise ValueError('Failed to read software identification')
        self.logger.info('Program software identification: 0x{:08x}'.format(swid))
        return swid

    def flash_status(self):
        '''Read flash status identification'''
        try:
            status = self.flash_sdo.raw
        except:
            raise ValueError('Failed to read flash status identification')
        return status

    def download(self, bin_file):
        '''Download program to CANopen object dictionary (0x1f50)'''
        self.logger.info('Downloading program: %s', bin_file)
        try:
            size = os.path.getsize(bin_file)
            infile = open(bin_file, 'rb')  # noqa: SIM115
            outfile = self.data_sdo.open('wb', buffering=self.download_buffer_size,
                                         size=size, block_transfer=self.block_transfer)

            progress = Bar('%(percent)d%%', max=size, suffix='%(index)d/%(max)dB')
            while True:
                chunk = infile.read(self.download_buffer_size // 2)
                if not chunk:
                    break
                outfile.write(chunk)
                progress.next(n=len(chunk))
        except:
            raise ValueError('Failed to download program')
        finally:
            progress.finish()
            infile.close()
            outfile.close()

    def wait_for_bootup(self, timeout=DEFAULT_TIMEOUT):
        '''Wait for boot-up message reception'''
        self.logger.info('Waiting for boot-up message...')
        try:
            self.node.nmt.wait_for_bootup(timeout=timeout)
        except:
            raise ValueError('Timeout waiting for boot-up message')

    def wait_for_flash_status_ok(self, timeout=DEFAULT_TIMEOUT):
        '''Wait for flash status ok'''
        self.logger.info('Waiting for flash status ok')
        end_time = time.time() + timeout
        while True:
            now = time.time()
            status = self.flash_status()
            if status == 0:
                break

            if now > end_time:
                return status

        return status

    @staticmethod
    def create_object_dictionary():
        '''Create a synthetic CANopen object dictionary for program download'''
        objdict = canopen.objectdictionary.ObjectDictionary()

        array = canopen.objectdictionary.Array('Program data', 0x1f50)
        member = canopen.objectdictionary.Variable('', 0x1f50, subindex=1)
        member.data_type = canopen.objectdictionary.DOMAIN
        array.add_member(member)
        objdict.add_object(array)

        array = canopen.objectdictionary.Array('Program control', 0x1f51)
        member = canopen.objectdictionary.Variable('', 0x1f51, subindex=1)
        member.data_type = canopen.objectdictionary.UNSIGNED8
        array.add_member(member)
        objdict.add_object(array)

        array = canopen.objectdictionary.Array('Program software ID', 0x1f56)
        member = canopen.objectdictionary.Variable('', 0x1f56, subindex=1)
        member.data_type = canopen.objectdictionary.UNSIGNED32
        array.add_member(member)
        objdict.add_object(array)

        array = canopen.objectdictionary.Array('Flash error ID', 0x1f57)
        member = canopen.objectdictionary.Variable('', 0x1f57, subindex=1)
        member.data_type = canopen.objectdictionary.UNSIGNED32
        array.add_member(member)
        objdict.add_object(array)

        return objdict
