#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

import fs_class as fs_core
import intelhex
import zms as fs_zms


class RawStorage:
    """Direct raw storage"""

    def __init__(self, fs: fs_core.Fs):
        self.fs = fs

    @staticmethod
    def validate_id_data_records(id_data_records):
        """Validate ID-data pairs"""
        for record_id, data in id_data_records.items():
            if not isinstance(record_id, int):
                print(f"Record ID {record_id} is not an integer")
                return False
            if record_id < 0:
                print(f"Record ID {record_id} is negative")
                return False
            if not isinstance(data, bytes):
                print(f"Data for ID {record_id} is not a bytes object")
                return False
        return True

    def read(self, bin_str):
        """
        Read ID-data records from the binary string containing the storage partition.

        Args:
            bin_str (str): Binary string containing the storage partition.

        Returns:
            dict: Dictionary with ID-data records.
        """
        records = self.fs.parse(bin_str)

        if records is None:
            return None

        if not self.validate_id_data_records(records):
            return None

        return records

    def write(self, dest_hex_file, id_data_records):
        """
        Write ID-data records to the Intel Hex file.

        Args:
            dest_hex_file (str): Destination hex file path.
            id_data_records (dict): Dictionary with ID-data records (int -> bytes).

        Returns:
            bool: True if the operation was successful, False otherwise.
        """
        # Validate ID-data records
        if not self.validate_id_data_records(id_data_records):
            return False

        # Create an IntelHex object
        ih = intelhex.IntelHex()

        # Write initialization records
        self.fs.initialize(ih)

        # Write ID-data records
        for record_id, data in id_data_records.items():
            print(f"  Writing ID {record_id} ({len(data)} bytes)")
            self.fs.write_record(ih, record_id, data)

        # Write to hex file
        try:
            ih.write_hex_file(dest_hex_file)
        except Exception as e:
            print(f"Error writing hex file: {str(e)}")
            return False

        return True


def storage_raw_instance_create(
    storage_base, storage_sector_size, device_write_block_size, device_flash_erase_value
):
    """
    Create a raw storage instance

    Args:
        storage_base: Base address of storage partition
        storage_sector_size: Size of each sector
        device_write_block_size: Write block size of the device
        device_flash_erase_value: Erase value for flash (typically 0xFF)

    Returns:
        RawStorage: Raw storage instance
    """

    fs = fs_zms.FsZms(
        base_addr=storage_base,
        sector_size=storage_sector_size,
        write_block_size=device_write_block_size,
        flash_erase_value=device_flash_erase_value,
    )

    return RawStorage(fs=fs)
