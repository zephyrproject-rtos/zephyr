#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

import argparse
import os
import sys

import fs_raw_instance as storage_raw
from zms import ZmsIdSize

NVM_ERASE_VALUE = b'\xff'


def provision_error_handle(msg, param_prefix=None):
    parser.print_usage()

    if param_prefix is None:
        param_prefix = ''
    else:
        param_prefix = f'argument {param_prefix}: '
    print('gen_zms_provision_data.py: error: ' + param_prefix + msg)

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

        data_str = parts[1].strip()
        if data_str.startswith('0x') or data_str.startswith('0X'):
            data_str = data_str[2:]

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
        '-w',
        '--write-block-size',
        default=16,
        metavar='WRITE_BLOCK_SIZE',
        help='Write block size in bytes given in int format. Default is 16.',
    )
    parser.add_argument(
        '-c',
        '--sector-size',
        default='0x1000',
        metavar='SECTOR_SIZE',
        help='Sector size in bytes given in hex format. Default is 0x1000.',
    )
    parser.add_argument(
        '--id-size',
        type=int,
        choices=[32, 64],
        default=32,
        metavar='{32,64}',
        help='Size of the ZMS IDs in bits. Must match CONFIG_ZMS_ID_64BIT of the '
        'application. Default is 32.',
    )

    parser.add_argument('--help', action='help', help='Show this help message and exit')
    args = parser.parse_args()

    return args


def storage_base_input_handle(storage_base, storage_size, write_block_size):
    param_prefix = '-f/--storage-base'

    storage_base = hex_arg_to_int(storage_base)
    storage_size = hex_arg_to_int(storage_size)
    write_block_size = int(write_block_size)

    if (storage_base % write_block_size) != 0:
        aligned_page = hex(storage_base & ~(write_block_size - 1))
        provision_error_handle(
            f'address should be page aligned: {hex(storage_base)} -> {aligned_page}', param_prefix
        )

    return storage_base


def storage_size_input_handle(storage_size, sector_size):
    param_prefix = '-s/--storage-size'

    storage_size = hex_arg_to_int(storage_size)
    sector_size = hex_arg_to_int(sector_size)

    if storage_size is None:
        provision_error_handle('invalid hex value', param_prefix)

    if not sector_size:
        provision_error_handle('invalid hex value', '-c/--sector-size')

    if (storage_size % sector_size) != 0:
        provision_error_handle(
            f'size {hex(storage_size)} is not a multiple of the sector size {hex(sector_size)}',
            param_prefix,
        )

    if storage_size < 2 * sector_size:
        provision_error_handle(
            f'size {hex(storage_size)} is too small, ZMS needs at least 2 sectors '
            f'of {hex(sector_size)}',
            param_prefix,
        )


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
        os.remove(output_path)

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
        provision_error_handle(f'File not found: {input_file}', param_prefix)
    except Exception as e:
        provision_error_handle(f'Error reading file: {str(e)}', param_prefix)

    return id_data_records


def provision(
    data_args,
    input_file,
    output_path,
    storage_base,
    storage_size,
    write_block_size,
    sector_size,
    id_size,
):
    id_size = ZmsIdSize(id_size)
    storage_base = storage_base_input_handle(storage_base, storage_size, write_block_size)
    storage_size_input_handle(storage_size, sector_size)
    output_path = output_path_input_handle(output_path)

    id_data_records = parse_data_from_args(data_args)
    file_records = parse_data_from_file(input_file)

    for record_id, data in file_records.items():
        if record_id in id_data_records:
            provision_error_handle(f'Duplicate ID {record_id} in command line and input file')
        id_data_records[record_id] = data

    # The all-ones ID is reserved by ZMS for its special ATEs
    max_id = (1 << id_size.value) - 2
    for record_id in id_data_records:
        if record_id > max_id:
            provision_error_handle(
                f'ID {record_id} does not fit in {id_size.value}-bit IDs (maximum is {max_id})'
            )

    print(f'''Using:
    * Storage base: {hex(storage_base)}
    * Device write block size: {write_block_size} bytes
    * ID size: {id_size.value} bits
    * Number of records: {len(id_data_records)}''')

    sector_size = hex_arg_to_int(sector_size)
    write_block_size = int(write_block_size)
    raw_storage = storage_raw.storage_raw_instance_create(
        storage_base, sector_size, write_block_size, NVM_ERASE_VALUE, id_size
    )

    if raw_storage.write(output_path, id_data_records):
        print('Provisioning successful')
        print(f'Generated: {output_path}')
    else:
        provision_error_handle('Provisioning failed')


if __name__ == "__main__":
    args = parse_args()
    provision(
        args.data,
        args.input_file,
        args.output_path,
        args.storage_base,
        args.storage_size,
        args.write_block_size,
        args.sector_size,
        args.id_size,
    )
