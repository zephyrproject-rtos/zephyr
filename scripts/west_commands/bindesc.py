# Copyright (c) 2023 Yonatan Schachter
#
# SPDX-License-Identifier: Apache-2.0

from textwrap import dedent
import struct

from west.commands import WestCommand
from west import log


try:
    from elftools.elf.elffile import ELFFile
    from intelhex import IntelHex
    MISSING_REQUIREMENTS = False
except ImportError:
    MISSING_REQUIREMENTS = True


# Based on scripts/build/uf2conv.py
def convert_from_uf2(buf):
    UF2_MAGIC_START0 = 0x0A324655 # First magic number ('UF2\n')
    UF2_MAGIC_START1 = 0x9E5D5157 # Second magic number
    numblocks = len(buf) // 512
    curraddr = None
    outp = []
    for blockno in range(numblocks):
        ptr = blockno * 512
        block = buf[ptr:ptr + 512]
        hd = struct.unpack(b'<IIIIIIII', block[0:32])
        if hd[0] != UF2_MAGIC_START0 or hd[1] != UF2_MAGIC_START1:
            log.inf('Skipping block at ' + ptr + '; bad magic')
            continue
        if hd[2] & 1:
            # NO-flash flag set; skip block
            continue
        datalen = hd[4]
        if datalen > 476:
            log.die(f'Invalid UF2 data size at {ptr}')
        newaddr = hd[3]
        if curraddr is None:
            curraddr = newaddr
        padding = newaddr - curraddr
        if padding < 0:
            log.die(f'Block out of order at {ptr}')
        if padding > 10*1024*1024:
            log.die(f'More than 10M of padding needed at {ptr}')
        if padding % 4 != 0:
            log.die(f'Non-word padding size at {ptr}')
        while padding > 0:
            padding -= 4
            outp += b'\x00\x00\x00\x00'
        outp.append(block[32 : 32 + datalen])
        curraddr = newaddr + datalen
    return b''.join(outp)


class Bindesc(WestCommand):
    EXTENSIONS = ['bin', 'hex', 'elf', 'uf2']

    # Corresponds to the definitions in include/zephyr/bindesc.h.
    # Do not change without syncing the definitions in both files!
    TYPE_UINT = 0
    TYPE_STR = 1
    TYPE_BYTES = 2
    MAGIC = 0xb9863e5a7ea46046
    DESCRIPTORS_END = 0xffff

    def __init__(self):
        self.TAG_TO_NAME = {
            # Corresponds to the definitions in include/zephyr/bindesc.h.
            # Do not change without syncing the definitions in both files!
            self.bindesc_gen_tag(self.TYPE_STR, 0x800): 'APP_VERSION_STRING',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x801): 'APP_VERSION_MAJOR',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x802): 'APP_VERSION_MINOR',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x803): 'APP_VERSION_PATCHLEVEL',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x804): 'APP_VERSION_NUMBER',
            self.bindesc_gen_tag(self.TYPE_STR, 0x805): 'APP_BUILD_VERSION',
            self.bindesc_gen_tag(self.TYPE_STR, 0x900): 'KERNEL_VERSION_STRING',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x901): 'KERNEL_VERSION_MAJOR',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x902): 'KERNEL_VERSION_MINOR',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x903): 'KERNEL_VERSION_PATCHLEVEL',
            self.bindesc_gen_tag(self.TYPE_UINT, 0x904): 'KERNEL_VERSION_NUMBER',
            self.bindesc_gen_tag(self.TYPE_STR, 0x905): 'KERNEL_BUILD_VERSION',
            self.bindesc_gen_tag(self.TYPE_UINT, 0xa00): 'BUILD_TIME_YEAR',
            self.bindesc_gen_tag(self.TYPE_UINT, 0xa01): 'BUILD_TIME_MONTH',
            self.bindesc_gen_tag(self.TYPE_UINT, 0xa02): 'BUILD_TIME_DAY',
            self.bindesc_gen_tag(self.TYPE_UINT, 0xa03): 'BUILD_TIME_HOUR',
            self.bindesc_gen_tag(self.TYPE_UINT, 0xa04): 'BUILD_TIME_MINUTE',
            self.bindesc_gen_tag(self.TYPE_UINT, 0xa05): 'BUILD_TIME_SECOND',
            self.bindesc_gen_tag(self.TYPE_UINT, 0xa06): 'BUILD_TIME_UNIX',
            self.bindesc_gen_tag(self.TYPE_STR, 0xa07): 'BUILD_DATE_TIME_STRING',
            self.bindesc_gen_tag(self.TYPE_STR, 0xa08): 'BUILD_DATE_STRING',
            self.bindesc_gen_tag(self.TYPE_STR, 0xa09): 'BUILD_TIME_STRING',
            self.bindesc_gen_tag(self.TYPE_STR, 0xb00): 'HOST_NAME',
            self.bindesc_gen_tag(self.TYPE_STR, 0xb01): 'C_COMPILER_NAME',
            self.bindesc_gen_tag(self.TYPE_STR, 0xb02): 'C_COMPILER_VERSION',
            self.bindesc_gen_tag(self.TYPE_STR, 0xb03): 'CXX_COMPILER_NAME',
            self.bindesc_gen_tag(self.TYPE_STR, 0xb04): 'CXX_COMPILER_VERSION',
        }
        self.NAME_TO_TAG = {v: k for k, v in self.TAG_TO_NAME.items()}

        super().__init__(
            'bindesc',
            'work with Binary Descriptors',
            dedent('''
            Work with Binary Descriptors - constant data objects
            describing a binary image
            '''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name,
                                         help=self.help,
                                         description=self.description)

        subparsers = parser.add_subparsers(help='sub-command to run', required=True)

        dump_parser = subparsers.add_parser('dump', help='Dump all binary descriptors in the image')
        dump_parser.add_argument('file', type=str, help='Executable file')
        dump_parser.add_argument('--file-type', type=str, choices=self.EXTENSIONS, help='File type')
        dump_parser.add_argument('-b', '--big-endian', action='store_true',
                                 help='Target CPU is big endian')
        dump_parser.set_defaults(subcmd='dump', big_endian=False)

        search_parser = subparsers.add_parser('search', help='Search for a specific descriptor')
        search_parser.add_argument('descriptor', type=str, help='Descriptor name')
        search_parser.add_argument('file', type=str, help='Executable file')
        search_parser.add_argument('--file-type', type=str, choices=self.EXTENSIONS, help='File type')
        search_parser.add_argument('-b', '--big-endian', action='store_true',
                                   help='Target CPU is big endian')
        search_parser.set_defaults(subcmd='search', big_endian=False)

        custom_search_parser = subparsers.add_parser('custom_search',
                                                     help='Search for a custom descriptor')
        custom_search_parser.add_argument('type', type=str, choices=['UINT', 'STR', 'BYTES'],
                                          help='Descriptor type')
        custom_search_parser.add_argument('id', type=str, help='Descriptor ID in hex')
        custom_search_parser.add_argument('file', type=str, help='Executable file')
        custom_search_parser.add_argument('--file-type', type=str, choices=self.EXTENSIONS,
                                          help='File type')
        custom_search_parser.add_argument('-b', '--big-endian', action='store_true',
                                   help='Target CPU is big endian')
        custom_search_parser.set_defaults(subcmd='custom_search', big_endian=False)

        list_parser = subparsers.add_parser('list', help='List all known descriptors')
        list_parser.set_defaults(subcmd='list', big_endian=False)

        get_offset_parser = subparsers.add_parser('get_offset', help='Get the offset of the descriptors')
        get_offset_parser.add_argument('file', type=str, help='Executable file')
        get_offset_parser.add_argument('--file-type', type=str, choices=self.EXTENSIONS,
                                          help='File type')
        get_offset_parser.add_argument('-b', '--big-endian', action='store_true',
                                       help='Target CPU is big endian')
        get_offset_parser.set_defaults(subcmd='get_offset', big_endian=False)
        return parser

    def dump(self, args):
        image = self.get_image_data(args.file)

        descriptors = self.parse_descriptors(image)
        for tag, value in descriptors.items():
            if tag in self.TAG_TO_NAME:
                tag = self.TAG_TO_NAME[tag]
            log.inf(f'{tag}', self.bindesc_repr(value))

    def list(self, args):
        for tag in self.TAG_TO_NAME.values():
            log.inf(f'{tag}')

    def common_search(self, args, search_term):
        image = self.get_image_data(args.file)

        descriptors = self.parse_descriptors(image)

        if search_term in descriptors:
            value = descriptors[search_term]
            log.inf(self.bindesc_repr(value))
        else:
            log.die('Descriptor not found')

    def search(self, args):
        try:
            search_term = self.NAME_TO_TAG[args.descriptor]
        except KeyError:
            log.die(f'Descriptor {args.descriptor} is invalid')

        self.common_search(args, search_term)

    def custom_search(self, args):
        custom_type = {
            'STR': self.TYPE_STR,
            'UINT': self.TYPE_UINT,
            'BYTES': self.TYPE_BYTES
        }[args.type]
        custom_tag = self.bindesc_gen_tag(custom_type, int(args.id, 16))
        self.common_search(args, custom_tag)

    def get_offset(self, args):
        image = self.get_image_data(args.file)

        magic = struct.pack('>Q' if self.is_big_endian else 'Q', self.MAGIC)
        index = image.find(magic)
        if index == -1:
            log.die('Could not find binary descriptor magic')
        log.inf(f'{index} {hex(index)}')

    def do_run(self, args, _):
        if MISSING_REQUIREMENTS:
            raise RuntimeError('one or more Python dependencies were missing; '
                               'see the getting started guide for details on '
                               'how to fix')
        self.is_big_endian = args.big_endian
        self.file_type = self.guess_file_type(args)
        subcmd = getattr(self, args.subcmd)
        subcmd(args)

    def get_image_data(self, file_name):
        if self.file_type == 'bin':
            with open(file_name, 'rb') as bin_file:
                return bin_file.read()

        if self.file_type == 'hex':
            return IntelHex(file_name).tobinstr()

        if self.file_type == 'uf2':
            with open(file_name, 'rb') as uf2_file:
                return convert_from_uf2(uf2_file.read())

        if self.file_type == 'elf':
            with open(file_name, 'rb') as f:
                elffile = ELFFile(f)

                section = elffile.get_section_by_name('rom_start')
                if section:
                    return section.data()

                section = elffile.get_section_by_name('text')
                if section:
                    return section.data()

            log.die('No "rom_start" or "text" section found')

        log.die('Unknown file type')

    def parse_descriptors(self, image):
        magic = struct.pack('>Q' if self.is_big_endian else 'Q', self.MAGIC)
        index = image.find(magic)
        if index == -1:
            log.die('Could not find binary descriptor magic')

        descriptors = {}

        index += len(magic) # index points to first descriptor
        current_tag = self.bytes_to_short(image[index:index+2])
        while current_tag != self.DESCRIPTORS_END:
            index += 2 # index points to length
            length = self.bytes_to_short(image[index:index+2])
            index += 2 # index points to data
            data = image[index:index+length]

            tag_type = self.bindesc_get_type(current_tag)
            if tag_type == self.TYPE_STR:
                decoded_data = data[:-1].decode('ascii')
            elif tag_type == self.TYPE_UINT:
                decoded_data = self.bytes_to_uint(data)
            elif tag_type == self.TYPE_BYTES:
                decoded_data = data
            else:
                log.die(f'Unknown type for tag 0x{current_tag:04x}')

            key = f'0x{current_tag:04x}'
            descriptors[key] = decoded_data
            index += length
            index = self.align(index, 4)
            current_tag = self.bytes_to_short(image[index:index+2])

        return descriptors

    def guess_file_type(self, args):
        if "file" not in args:
            return None

        # If file type is explicitly given, use it
        if args.file_type is not None:
            return args.file_type

        # If the file has a known extension, use it
        for extension in self.EXTENSIONS:
            if args.file.endswith(f'.{extension}'):
                return extension

        with open(args.file, 'rb') as f:
            header = f.read(1024)

        # Try the elf magic
        if header.startswith(b'\x7fELF'):
            return 'elf'

        # Try the uf2 magic
        if header.startswith(b'UF2\n'):
            return 'uf2'

        try:
            # if the file is textual it's probably hex
            header.decode('ascii')
            return 'hex'
        except UnicodeDecodeError:
            # Default to bin
            return 'bin'

    def bytes_to_uint(self, b):
        return struct.unpack('>I' if self.is_big_endian else 'I', b)[0]

    def bytes_to_short(self, b):
        return struct.unpack('>H' if self.is_big_endian else 'H', b)[0]

    @staticmethod
    def bindesc_gen_tag(_type, _id):
        return f'0x{(_type << 12 | _id):04x}'

    @staticmethod
    def bindesc_get_type(tag):
        return tag >> 12

    @staticmethod
    def align(x, alignment):
        return (x + alignment - 1) & (~(alignment - 1))

    @staticmethod
    def bindesc_repr(value):
        if isinstance(value, str):
            return f'"{value}"'
        if isinstance(value, (int, bytes)):
            return f'{value}'
