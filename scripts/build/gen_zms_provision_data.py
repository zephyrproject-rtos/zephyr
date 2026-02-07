#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

import argparse
import os
import sys

import fs_raw_instance as storage_raw
from intelhex import IntelHex

NVM_ERASE_VALUE = b'\xff'


def provision_error_handle(msg, param_prefix=None):
    parser.print_usage()

    if param_prefix is None:
        param_prefix = ''
    else:
        param_prefix = f'argument {param_prefix}: '
    print('provision-raw: error: ' + param_prefix + msg)

    sys.exit(1)


def hex_arg_to_int(hex_str):
    try:
        return int(hex_str, 16)
    except ValueError:
        return None


def parse_id_data_pair(pair_str):
    """Parse ID:data pair from command line.
    Format: ID:data where data is hex string (without 0x prefix)
    Example: 1:48656c6c6f for ID=1, data=b'Hello'
    """
    try:
        parts = pair_str.split(':', 1)
        if len(parts) != 2:
            return None, None, "Invalid format. Use ID:hexdata"

        # Parse ID - can be decimal or hex (with 0x prefix)
        id_str = parts[0].strip()
        if id_str.startswith('0x') or id_str.startswith('0X'):
            record_id = int(id_str, 16)
        else:
            record_id = int(id_str, 10)

        # Parse data as hex string
        data_str = parts[1].strip()
        if data_str.startswith('0x') or data_str.startswith('0X'):
            data_str = data_str[2:]

        # Convert hex string to bytes
        if len(data_str) % 2 != 0:
            return None, None, "Data hex string must have even number of characters"

        data = bytes.fromhex(data_str)

        return record_id, data, None
    except ValueError as e:
        return None, None, str(e)


def parse_args():
    global parser

    parser = argparse.ArgumentParser(
        description='Raw ZMS Provisioning Tool', add_help=False, allow_abbrev=False
    )

    parser.add_argument(
        '-d',
        '--data',
        action='append',
        metavar='ID:HEXDATA',
        help='ID-data pair in format ID:hexdata. Can be specified multiple times. '
        'Example: -d 1:48656c6c6f -d 2:576f726c64',
    )
    parser.add_argument(
        '-i',
        '--input-file',
        metavar='FILE',
        help='Input file with ID-data pairs (one per line, format: ID:hexdata)',
    )
    parser.add_argument(
        '-o',
        '--output-path',
        default='provisioned_raw.hex',
        metavar='PATH',
        help='Path to store the result of the provisioning.',
    )
    parser.add_argument(
        '-f',
        '--storage-base',
        metavar='ADDRESS',
        required=True,
        help='Storage base address given in hex format.',
    )
    parser.add_argument(
        '-s',
        '--storage-size',
        metavar='SIZE',
        required=True,
        help='Storage size in bytes given in hex format.',
    )
    parser.add_argument(
        '-n',
        '--nvm-address',
        default=0x0,
        metavar='NVM_OFFSET',
        help='NVM base address offset in hex format. Default is 0x0.',
    )
    parser.add_argument(
        '-z',
        '--nvm-size',
        default=0x0,
        metavar='NVM_SIZE',
        help='NVM size in bytes given in hex format. Default is 0x0.',
    )
    parser.add_argument(
        '-w',
        '--write-block-size',
        default=16,
        metavar='WRITE_BLOCK_SIZE',
        help='Write block size in bytes given in int format. Default is 16.',
    )
    parser.add_argument(
        '-c',
        '--sector-size',
        default=0x1000,
        metavar='SECTOR_SIZE',
        help='Sector size in bytes given in hex format. Default is 0x1000.',
    )
    parser.add_argument(
        '-x',
        '--input-hex-file',
        help='Hex file to be merged with provisioned data. If this option is set, '
        'the output hex file will be [input-hex-file + provisioned data].',
    )

    parser.add_argument('--help', action='help', help='Show this help message and exit')
    args = parser.parse_args()

    # Validate that at least one data source is provided
    if not args.data and not args.input_file:
        provision_error_handle('At least one of -d/--data or -i/--input-file must be specified')
    return args


def storage_base_input_handle(storage_base, storage_size, nvm_address, nvm_size, write_block_size):
    param_prefix = '-f/--storage-base'

    nvm_size = hex_arg_to_int(nvm_size)
    storage_base = hex_arg_to_int(storage_base)
    storage_size = hex_arg_to_int(storage_size)

    if nvm_address != 0x0:
        nvm_address = hex_arg_to_int(nvm_address)
        storage_base = nvm_address + storage_base

    if storage_base < nvm_address:
        provision_error_handle(
            f'storage address is smaller than the target device memory: '
            f'{hex(storage_base)} < {hex(nvm_address)}',
            param_prefix,
        )
    if (nvm_address + nvm_size - storage_base) <= 0:
        provision_error_handle(
            f'storage address is bigger than the target device memory: '
            f'{hex(storage_base)} >= {hex(nvm_size)}',
            param_prefix,
        )

    if (nvm_address + nvm_size - storage_base) < storage_size:
        provision_error_handle(
            f'not enough space from address to the end of target device memory:'
            f' {hex(storage_base)} + {hex(storage_size)} >'
            f' {hex(nvm_address + nvm_size)}',
            param_prefix,
        )

    if storage_base % write_block_size != 0:
        aligned_page = hex(storage_base & ~(write_block_size - 1))
        provision_error_handle(
            f'address should be page aligned: {hex(storage_base)} -> {aligned_page}', param_prefix
        )

    return storage_base


def input_hex_file_input_handle(input_hex_file):
    param_prefix = '-x/--input-hex-file'

    if not input_hex_file:
        return input_hex_file

    try:
        input_hex_file = os.path.realpath(input_hex_file)
    except Exception:
        provision_error_handle('malformed path format', param_prefix)

    if not os.path.exists(input_hex_file):
        provision_error_handle('target file does not exist', param_prefix)

    if not os.path.isfile(input_hex_file):
        provision_error_handle('target path is not a file', param_prefix)

    try:
        IntelHex(input_hex_file)
    except Exception as e:
        msg = 'target file with malformed content format\n' + str(e)
        provision_error_handle(msg, param_prefix)

    return input_hex_file


def output_path_input_handle(output_path):
    param_prefix = '-o/--output-path'

    if not output_path:
        return output_path

    try:
        output_path = os.path.realpath(output_path)
    except Exception:
        provision_error_handle('malformed path format', param_prefix)

    if os.path.isdir(output_path):
        provision_error_handle('target is an existing directory', param_prefix)

    if not os.path.exists(os.path.split(output_path)[0]):
        provision_error_handle('target directory does not exist', param_prefix)

    if os.path.exists(output_path):
        provision_error_handle('target file already exists', param_prefix)

    return output_path


def parse_data_from_args(data_args):
    """Parse ID-data pairs from command line arguments"""
    param_prefix = '-d/--data'
    id_data_records = {}

    if not data_args:
        return id_data_records

    for pair_str in data_args:
        record_id, data, error = parse_id_data_pair(pair_str)
        if error:
            provision_error_handle(f'Invalid data pair "{pair_str}": {error}', param_prefix)

        if record_id in id_data_records:
            provision_error_handle(f'Duplicate ID {record_id}', param_prefix)

        id_data_records[record_id] = data

    return id_data_records


def parse_data_from_file(input_file):
    """Parse ID-data pairs from input file"""
    param_prefix = '-i/--input-file'
    id_data_records = {}

    if not input_file:
        return id_data_records

    try:
        with open(input_file) as f:
            line_num = 0
            for line in f:
                line_num += 1
                line = line.strip()

                # Skip empty lines and comments
                if not line or line.startswith('#'):
                    continue

                record_id, data, error = parse_id_data_pair(line)
                if error:
                    provision_error_handle(
                        f'Line {line_num}: Invalid data pair "{line}": {error}', param_prefix
                    )

                if record_id in id_data_records:
                    provision_error_handle(
                        f'Line {line_num}: Duplicate ID {record_id}', param_prefix
                    )

                id_data_records[record_id] = data
    except FileNotFoundError:
        return id_data_records
    except Exception as e:
        provision_error_handle(f'Error reading file: {str(e)}', param_prefix)

    return id_data_records


def merge_hex_files(input_hex_file, provisioned_data_hex_file, output_file):
    try:
        h = IntelHex(input_hex_file)
        h.merge(IntelHex(provisioned_data_hex_file))
        h.write_hex_file(output_file)
    except Exception as e:
        os.remove(provisioned_data_hex_file)
        msg = '--input-hex-file target cannot be merged with provisioning data\n' + str(e)
        provision_error_handle(msg)


def provision(
    data_args,
    input_file,
    output_path,
    storage_base,
    storage_size,
    nvm_address,
    nvm_size,
    write_block_size,
    sector_size,
    input_hex_file,
):
    storage_base = storage_base_input_handle(
        storage_base, storage_size, nvm_address, nvm_size, write_block_size
    )
    input_hex_file = input_hex_file_input_handle(input_hex_file)
    output_path = output_path_input_handle(output_path)

    # Parse ID-data pairs from both sources
    id_data_records = parse_data_from_args(data_args)
    file_records = parse_data_from_file(input_file)

    # Merge records, checking for duplicates
    for record_id, data in file_records.items():
        if record_id in id_data_records:
            provision_error_handle(f'Duplicate ID {record_id} in command line and input file')
        id_data_records[record_id] = data

    if not id_data_records:
        provision_error_handle('No ID-data pairs provided')

    print(f'''Using:
    * Storage base: {hex(storage_base)}
    * Device write block size: {write_block_size} bytes
    * Number of records: {len(id_data_records)}''')

    # Create raw storage instance
    raw_storage = storage_raw.storage_raw_instance_create(
        storage_base, sector_size, write_block_size, NVM_ERASE_VALUE
    )

    if raw_storage.write(output_path, id_data_records):
        print('Provisioning successful')
        print(f'Generated: {output_path}')
    else:
        provision_error_handle('Provisioning failed')

    if input_hex_file:
        provisioned_data_hex_file = output_path
        merge_hex_files(input_hex_file, provisioned_data_hex_file, output_path)
        print(f'Merged with: {input_hex_file}')


if __name__ == "__main__":
    args = parse_args()
    provision(
        args.data,
        args.input_file,
        args.output_path,
        args.storage_base,
        args.storage_size,
        args.nvm_address,
        args.nvm_size,
        args.write_block_size,
        args.sector_size,
        args.input_hex_file,
    )
