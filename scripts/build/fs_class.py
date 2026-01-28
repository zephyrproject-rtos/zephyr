#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum


class FsSectorStatus(Enum):
    ERASED = 0
    OPEN = 1
    CLOSED = 2
    NA = 3


@dataclass
class FsSectorData:
    status: FsSectorStatus
    records: dict


class Fs(ABC):
    def __init__(self, base_addr, sector_size, write_block_size, flash_erase_value):
        self.base_addr = base_addr
        self.sector_size = sector_size
        self.write_block_size = write_block_size
        self.flash_erase_value = flash_erase_value

    @abstractmethod
    def initialize(self, ih):
        """
        Initialize the file system.
        Write all required records to the file system to make it ready for use.

        Args:
            ih (intelhex.IntelHex): Intel Hex object.
        """

    @abstractmethod
    def write_record(self, ih, record_id, data):
        """
        Write a single record to the file system.

        Args:
            ih (intelhex.IntelHex): Intel Hex object.
            record_id (int): Record ID.
            data (bytes): Record data.
        """

    @abstractmethod
    def parse(self, bin_str):
        """
        Parse the file system.

        Args:
            bin_str (str): Binary string containing the file system.

        Returns:
            dict: Dictionary with key-value (record_id, record_data) pairs.
        """

    # Helper methods used for writing the file system

    def _from_relative_2_absolute_addr(self, offset):
        return self.base_addr + offset

    def _align_data_to_wbs(self, data):
        padding = (self.write_block_size - len(data)) % self.write_block_size
        return data + self.flash_erase_value * padding

    # Helper methods used for parsing the file system

    def _get_sector_cnt(self, bin_str):
        assert (len(bin_str) % self.sector_size) == 0
        return int(len(bin_str) / self.sector_size)

    def _get_sector_addr_range(self, sector_idx):
        start = self.sector_size * sector_idx
        end = self.sector_size * (sector_idx + 1)
        return start, end

    def _get_sector(self, bin_str, sector_idx):
        start, end = self._get_sector_addr_range(sector_idx)
        sector = bin_str[start:end]
        return sector

    def _parse_all_sectors(self, bin_str, parse_sector_fn):
        sectors = []

        sector_cnt = self._get_sector_cnt(bin_str)
        for i in range(0, sector_cnt):
            sector = self._get_sector(bin_str, i)
            parsed_sector = parse_sector_fn(sector)
            sectors.append(parsed_sector)

        return sectors

    def _find_newest_sector_id(self, sectors):
        newest_idx = None

        for idx, elem in enumerate(sectors):
            next_elem = sectors[(idx + 1) % len(sectors)]
            if elem.status == FsSectorStatus.CLOSED and next_elem.status == FsSectorStatus.OPEN:
                if newest_idx is not None:
                    print("Memory analysis: Found another transition from closed to open sector")
                newest_idx = (idx + 1) % len(sectors)

        if newest_idx is None:
            print("Memory analysis: There was no transition, will proceed from the end of memory")
            newest_idx = len(sectors) - 1

        return newest_idx

    def _get_sectors_additional_metadata(self, sectors):
        additional_metadata = {
            "sectors_with_records_cnt": 0,
            "first_sector_with_records_idx": None,
        }

        for idx, sector in enumerate(sectors):
            if len(sector.records) > 0:
                if additional_metadata["first_sector_with_records_idx"] is None:
                    additional_metadata["first_sector_with_records_idx"] = idx
                additional_metadata["sectors_with_records_cnt"] += 1

        return additional_metadata

    def _get_sectors_range(self, sectors):
        sector_cnt = len(sectors)
        sectors_range = None

        additional_metadata = self._get_sectors_additional_metadata(sectors)

        if additional_metadata["sectors_with_records_cnt"] == 0:
            print("Memory analysis: No data records found in the memory")
            return None

        if additional_metadata["sectors_with_records_cnt"] == 1:
            print("Memory analysis: Found only one sector with data records")
            sectors_range = (
                additional_metadata["first_sector_with_records_idx"],
                additional_metadata["first_sector_with_records_idx"] + 1,
            )
        else:
            print("Memory analysis: Assuming range of storage partition by itself")
            sectors_range = (additional_metadata["first_sector_with_records_idx"], sector_cnt)

        print(f"Memory analysis: Search in sector range: {sectors_range}")
        return sectors_range

    def _order_sectors(self, sectors):
        if len(sectors) == 0:
            print("Memory analysis: No sectors to be analysed")
            return None

        sectors_range = self._get_sectors_range(sectors)
        if sectors_range is None:
            return None

        sectors = sectors[sectors_range[0] : sectors_range[1]]

        if len(sectors) > 1:
            # Calculate the oldest sector ID
            idx = (self._find_newest_sector_id(sectors) + 1) % len(sectors)
            # Reorder NVS sectors metadata from the oldest to the newest
            sectors = sectors[idx:] + sectors[:idx]

        return sectors

    def _consolidate_records(self, ordered_sectors):
        if ordered_sectors is None:
            return {}

        consolidated_records = {}
        for sector in ordered_sectors:
            if len(sector.records) == 0:
                continue

            for record_id, value in sector.records.items():
                consolidated_records[record_id] = value

        return consolidated_records
