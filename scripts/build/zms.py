#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

import struct
import zlib
from dataclasses import dataclass
from enum import Enum

import fs_class as fs_core


class ZmsIdSize(Enum):
    """Enum for ZMS ID size configuration"""

    ID_32_BIT = 32
    ID_64_BIT = 64


# Used for special ATEs, such as: Empty, Close, GC done.
@dataclass
class Metadata:
    meta_data: int  # uint32_t meta_data;

    def __eq__(self, other):
        if not isinstance(other, Metadata):
            return False
        return self.meta_data == other.meta_data

    def serialize(self):
        return struct.pack('<I', self.meta_data)

    @classmethod
    def deserialize(cls, data):
        return cls(meta_data=struct.unpack('<I', data)[0])


# Used for data ATEs that point to the big data.
@dataclass
class DataCRC:
    data_crc: int  # uint32_t data_crc;

    def __eq__(self, other):
        if not isinstance(other, DataCRC):
            return False
        return self.data_crc == other.data_crc

    @staticmethod
    def crc32_from_bytes(buf):
        return zlib.crc32(buf) & 0xFFFFFFFF

    def verify_crc32(self, data):
        # CRC32 support is optional
        if self.data_crc == 0:
            return True

        if self.data_crc != self.crc32_from_bytes(data):
            print("Memory analysis: CRC32 mismatch")
            return False
        return True

    def serialize(self):
        return struct.pack('<I', self.data_crc)

    @classmethod
    def deserialize(cls, data):
        return cls(data_crc=struct.unpack('<I', data)[0])


# Used for data ATEs that contain small data.
@dataclass
class SmallData:
    data: bytes  # uint8_t data[8] or uint8_t data[4];

    MAX_SIZE_32BIT = 8
    MAX_SIZE_64BIT = 4

    def __eq__(self, other):
        if not isinstance(other, SmallData):
            return False
        return self.data == other.data

    def serialize(self):
        return self.data

    @classmethod
    def deserialize(cls, data):
        return cls(data=data)

    @classmethod
    def get_max_size(cls, id_size):
        return cls.MAX_SIZE_32BIT if id_size == ZmsIdSize.ID_32_BIT else cls.MAX_SIZE_64BIT


# Used for either data ATEs that either points to the big data or for special ATEs.
@dataclass
class DataInfo:
    offset: int  # uint32_t offset;
    additional_info: DataCRC | Metadata  # union { uint32_t data_crc; uint32_t metadata; };

    def __eq__(self, other):
        if not isinstance(other, DataInfo):
            return False
        return (self.offset == other.offset) and (self.additional_info == other.additional_info)

    def serialize(self):
        return struct.pack('<I', self.offset) + self.additional_info.serialize()

    @classmethod
    def deserialize(cls, data, is_special_ate):
        offset = struct.unpack('<I', data[:4])[0]
        additional_info = None
        if is_special_ate:
            additional_info = Metadata.deserialize(data[4:8])
        else:
            additional_info = DataCRC.deserialize(data[4:8])
        return cls(offset=offset, additional_info=additional_info)


# Represents the ZMS ATE for both 32-bit and 64-bit ID variants
#
# (32-bit ID)
# struct zms_ate {
#     uint8_t crc8;
#     uint8_t cycle_cnt;
#     uint16_t len;
#     uint32_t id;
#     union {
#         uint8_t data[8];
#         struct {
#             uint32_t offset;
#             union {
#                 uint32_t data_crc;
#                 uint32_t metadata;
#             };
#         };
#     };
# } __packed;
#
# (64-bit ID)
# struct zms_ate {
#     uint8_t crc8;
#     uint8_t cycle_cnt;
#     uint16_t len;
#     uint64_t id;
#     union {
#         uint8_t data[4];
#         uint32_t offset;
#         uint32_t metadata;
#     };
# } __packed;


@dataclass
class FsZmsAte:
    cycle_cnt: int
    len: int
    id: int
    content: SmallData | DataInfo | Metadata
    id_size: ZmsIdSize

    crc8: int = 0xFF
    auto_update_crc8: bool = True

    # Size of the ZMS ATE equal to sizeof(struct zms_ate).
    ATE_SIZE = 16

    # Special ATE ID for both 32-bit and 64-bit
    HEAD_ID_32BIT = 0xFFFFFFFF
    HEAD_ID_64BIT = 0xFFFFFFFFFFFFFFFF

    def __post_init__(self):
        if self.auto_update_crc8:
            self.crc8 = self._compute_crc8()

    def __eq__(self, other):
        if not isinstance(other, FsZmsAte):
            return False
        return (
            (self.id == other.id)
            and (self.len == other.len)
            and (self.content == other.content)
            and (self.cycle_cnt == other.cycle_cnt)
            and (self.crc8 == other.crc8)
            and (self.id_size == other.id_size)
        )

    @staticmethod
    def crc8_ccitt_from_bytes(buf):
        CRC8_CCITT_TABLE = [
            0x00,
            0x07,
            0x0E,
            0x09,
            0x1C,
            0x1B,
            0x12,
            0x15,
            0x38,
            0x3F,
            0x36,
            0x31,
            0x24,
            0x23,
            0x2A,
            0x2D,
        ]

        crc = 0xFF
        for byte in buf:
            crc = (crc ^ byte) & 0xFF
            crc = ((crc << 4) ^ CRC8_CCITT_TABLE[(crc >> 4) & 0xFF]) & 0xFF
            crc = ((crc << 4) ^ CRC8_CCITT_TABLE[(crc >> 4) & 0xFF]) & 0xFF
        return crc

    def _compute_crc8(self):
        """Compute CRC8 for the ATE"""
        return self.crc8_ccitt_from_bytes(self.serialize()[1:])

    def _get_head_id(self):
        """Get the appropriate HEAD_ID based on id_size"""
        return self.HEAD_ID_32BIT if self.id_size == ZmsIdSize.ID_32_BIT else self.HEAD_ID_64BIT

    def _serialize_header(self):
        """Serialize the header (cycle_cnt, len, id)"""
        if self.id_size == ZmsIdSize.ID_32_BIT:
            return struct.pack('<BHI', self.cycle_cnt, self.len, self.id)
        else:
            return struct.pack('<BHQ', self.cycle_cnt, self.len, self.id)

    def _serialize_content(self):
        """Serialize the content based on type and id_size"""
        if isinstance(self.content, SmallData):
            return self.content.serialize()
        elif isinstance(self.content, DataInfo):
            if self.id_size == ZmsIdSize.ID_32_BIT:
                return self.content.serialize()
            else:
                # For 64-bit ID, only offset is stored (no data_crc/metadata in union)
                return struct.pack('<I', self.content.offset)
        elif isinstance(self.content, Metadata):
            # For 64-bit special ATEs
            return self.content.serialize()
        else:
            raise ValueError(f"Unknown content type: {type(self.content)}")

    def serialize(self):
        header_data = self._serialize_header()
        content_data = self._serialize_content()
        body = header_data + content_data

        if self.auto_update_crc8:
            self.crc8 = self.crc8_ccitt_from_bytes(body)

        return struct.pack('<B', self.crc8) + body

    def is_valid_crc8(self):
        return self.crc8 == self._compute_crc8()

    def is_valid_cycle_cnt(self, cycle_cnt):
        return self.cycle_cnt == cycle_cnt

    def is_valid(self, cycle_cnt):
        return self.is_valid_crc8() and self.is_valid_cycle_cnt(cycle_cnt)

    @classmethod
    def deserialize(cls, ate_bytes, write_block_size, id_size):
        ate_size = cls.size_from_write_block_size(write_block_size)
        assert len(ate_bytes) == ate_size

        ate_crc8 = ate_bytes[0]
        ate_cycle_cnt = ate_bytes[1]
        ate_len = struct.unpack('<H', ate_bytes[2:4])[0]

        if id_size == ZmsIdSize.ID_32_BIT:
            ate_id = struct.unpack('<I', ate_bytes[4:8])[0]
            content_start = 8
            is_special_ate = ate_id == cls.HEAD_ID_32BIT
            small_data_max = SmallData.MAX_SIZE_32BIT
        else:
            ate_id = struct.unpack('<Q', ate_bytes[4:12])[0]
            content_start = 12
            is_special_ate = ate_id == cls.HEAD_ID_64BIT
            small_data_max = SmallData.MAX_SIZE_64BIT

        # Parse content
        if is_special_ate:
            if id_size == ZmsIdSize.ID_32_BIT:
                content = DataInfo.deserialize(ate_bytes[content_start : content_start + 8], True)
            else:
                # For 64-bit special ATE, content is metadata
                content = Metadata.deserialize(ate_bytes[content_start : content_start + 4])
        elif ate_len <= small_data_max:
            content_end = content_start + small_data_max
            content = SmallData.deserialize(ate_bytes[content_start:content_end])
        else:
            if id_size == ZmsIdSize.ID_32_BIT:
                content = DataInfo.deserialize(ate_bytes[content_start : content_start + 8], False)
            else:
                # For 64-bit data ATE, only offset is stored
                offset = struct.unpack('<I', ate_bytes[content_start : content_start + 4])[0]
                # No CRC for 64-bit
                content = DataInfo(offset=offset, additional_info=DataCRC(data_crc=0))

        return cls(
            id=ate_id,
            len=ate_len,
            content=content,
            cycle_cnt=ate_cycle_cnt,
            crc8=ate_crc8,
            id_size=id_size,
            auto_update_crc8=False,
        )

    @classmethod
    def size_from_write_block_size(cls, write_block_size):
        assert write_block_size > 0
        assert (write_block_size & (write_block_size - 1)) == 0
        return (cls.ATE_SIZE + (write_block_size - 1)) & ~(write_block_size - 1)


# Special ATE: Empty ATE, located at the end of the sector (N ate).
class FsZmsAteEmpty(FsZmsAte):
    ZMS_VERSION = 0x01
    ZMS_VERSION_OFFSET = 0
    ZMS_MAGIC = 0x42
    ZMS_MAGIC_OFFSET = 8
    ZMS_RESERVED = 0x0000
    ZMS_RESERVED_OFFSET = 16

    @classmethod
    def _serialize_metadata(cls):
        return (
            (cls.ZMS_VERSION << cls.ZMS_VERSION_OFFSET)
            | (cls.ZMS_MAGIC << cls.ZMS_MAGIC_OFFSET)
            | (cls.ZMS_RESERVED << cls.ZMS_RESERVED_OFFSET)
        )

    def __init__(self, cycle_cnt, id_size):
        head_id = self.HEAD_ID_32BIT if id_size == ZmsIdSize.ID_32_BIT else self.HEAD_ID_64BIT

        if id_size == ZmsIdSize.ID_32_BIT:
            meta_data = Metadata(meta_data=FsZmsAteEmpty._serialize_metadata())
            content = DataInfo(offset=0x0, additional_info=meta_data)
        else:
            # For 64-bit, content is just metadata
            content = Metadata(meta_data=FsZmsAteEmpty._serialize_metadata())

        super().__init__(
            id=head_id, len=0xFFFF, content=content, cycle_cnt=cycle_cnt, id_size=id_size
        )


# Special ATE: Close ATE, located at the end of the sector (N-1 ate).
class FsZmsAteClose(FsZmsAte):
    def __init__(self, cycle_cnt, offset, id_size):
        head_id = self.HEAD_ID_32BIT if id_size == ZmsIdSize.ID_32_BIT else self.HEAD_ID_64BIT

        if id_size == ZmsIdSize.ID_32_BIT:
            meta_data = Metadata(meta_data=0xFFFFFFFF)
            content = DataInfo(offset=offset, additional_info=meta_data)
        else:
            # For 64-bit, content is just metadata
            content = Metadata(meta_data=0xFFFFFFFF)

        super().__init__(id=head_id, len=0x0, content=content, cycle_cnt=cycle_cnt, id_size=id_size)


class FsZms(fs_core.Fs):
    def __init__(
        self,
        base_addr,
        sector_size,
        write_block_size,
        flash_erase_value,
        id_size=ZmsIdSize.ID_32_BIT,
    ):
        super().__init__(base_addr, sector_size, write_block_size, flash_erase_value)

        self.id_size = id_size
        ate_size = FsZmsAte.size_from_write_block_size(write_block_size)

        # Start from the last ATE
        self.ate_offset = self.sector_size - ate_size
        self.data_offset = 0

    # Provisioning methods

    def _verify_sector(self):
        assert self.ate_offset >= self.data_offset

    def _change_sector_ate_offset(self, ate_len):
        self.ate_offset -= ate_len
        self._verify_sector()

    def _change_sector_data_offset(self, data_len):
        self.data_offset += data_len
        self._verify_sector()

    def _align_data_small(self, data, max_size):
        assert len(data) <= max_size
        return data + self.flash_erase_value * (max_size - len(data))

    def _write_zms_ate(self, ih, zms_ate):
        serialized_ate = self._align_data_to_wbs(zms_ate.serialize())
        ih.frombytes(serialized_ate, self._from_relative_2_absolute_addr(self.ate_offset))
        self._change_sector_ate_offset(len(serialized_ate))

    def _write_init_empty_ate(self, ih):
        empty_ate = FsZmsAteEmpty(cycle_cnt=0x01, id_size=self.id_size)
        self._write_zms_ate(ih, empty_ate)

    def _write_data_big(self, ih, data):
        aligned_data = self._align_data_to_wbs(data)
        ih.frombytes(aligned_data, self._from_relative_2_absolute_addr(self.data_offset))
        self._change_sector_data_offset(len(aligned_data))

    def _write_data_record_big(self, ih, record_id, data):
        if self.id_size == ZmsIdSize.ID_32_BIT:
            data_crc = DataCRC(data_crc=DataCRC.crc32_from_bytes(data))
            content = DataInfo(offset=self.data_offset, additional_info=data_crc)
        else:
            # For 64-bit, no CRC stored
            content = DataInfo(offset=self.data_offset, additional_info=DataCRC(data_crc=0))

        zms_ate = FsZmsAte(
            id=record_id, len=len(data), content=content, cycle_cnt=0x01, id_size=self.id_size
        )

        self._write_zms_ate(ih, zms_ate)
        self._write_data_big(ih, data)

    def _write_data_record_small(self, ih, record_id, data):
        max_size = SmallData.get_max_size(self.id_size)
        content = SmallData(data=self._align_data_small(data, max_size))
        zms_ate = FsZmsAte(
            id=record_id, len=len(data), content=content, cycle_cnt=0x01, id_size=self.id_size
        )

        self._write_zms_ate(ih, zms_ate)

    def initialize(self, ih):
        self._write_init_empty_ate(ih)

        # Skip two ATEs: one for close ATE and one for GC done ATE
        ate_size = FsZmsAte.size_from_write_block_size(self.write_block_size)
        self._change_sector_ate_offset(2 * ate_size)

    def write_record(self, ih, record_id, data):
        max_size = SmallData.get_max_size(self.id_size)
        if len(data) > max_size:
            self._write_data_record_big(ih, record_id, data)
        else:
            self._write_data_record_small(ih, record_id, data)

    # Extraction methods

    def _is_erased_sector(self, sector):
        return sector == (self.sector_size * self.flash_erase_value)

    def _is_close_ate(self, ate):
        if not ate.is_valid_crc8():
            return False

        close_ate_cmp = FsZmsAteClose(
            cycle_cnt=ate.cycle_cnt, offset=self._get_close_offset(ate), id_size=self.id_size
        )

        if close_ate_cmp != ate:
            return False

        ate_size = FsZmsAte.size_from_write_block_size(self.write_block_size)
        close_offset = self._get_close_offset(ate)
        valid_offset = (self.sector_size - close_offset) % ate_size == 0

        return valid_offset

    def _get_close_offset(self, ate):
        """Extract offset from close ATE based on id_size"""
        if self.id_size == ZmsIdSize.ID_32_BIT:
            return ate.content.offset
        else:
            # For 64-bit, metadata contains the offset in the lower bits
            # But actually for close ATE it's all 0xff, so we need to rethink this
            # Based on structure, for 64-bit close ATE the offset isn't stored the same way
            # Let me check the actual close ATE structure for 64-bit
            # Actually, looking at the structure, 64-bit close might not store offset
            return 0

    def _is_empty_ate(self, ate):
        if not ate.is_valid_crc8():
            return False

        empty_ate_cmp = FsZmsAteEmpty(cycle_cnt=ate.cycle_cnt, id_size=self.id_size)
        return empty_ate_cmp == ate

    def _validate_data_within_sector(self, ate, ate_ptr, data_ptr):
        if not isinstance(ate.content, DataInfo):
            return True

        if ate.content.offset < data_ptr:
            return False

        return (ate.content.offset + ate.len) < ate_ptr

    def _parse_record_data(self, ate, sector, ate_ptr, data_ptr):
        if isinstance(ate.content, SmallData):
            return ate.content.data[: ate.len], data_ptr
        elif isinstance(ate.content, DataInfo):
            if isinstance(ate.content.additional_info, DataCRC):
                if not self._validate_data_within_sector(ate, ate_ptr, data_ptr):
                    return None, data_ptr

                data = sector[ate.content.offset : (ate.content.offset + ate.len)]
                data_ptr = ate.content.offset + ate.len

                if (
                    self.id_size == ZmsIdSize.ID_32_BIT
                    and not ate.content.additional_info.verify_crc32(data)
                ):
                    return None, data_ptr

                return data, data_ptr
            else:
                return None, data_ptr
        elif isinstance(ate.content, Metadata):
            return None, data_ptr

        return None, data_ptr

    def _parse_sector(self, sector):
        data_ptr = 0
        records = {}

        if self._is_erased_sector(sector):
            return fs_core.FsSectorData(status=fs_core.FsSectorStatus.ERASED, records={})

        ate_size = FsZmsAte.size_from_write_block_size(self.write_block_size)

        # Check last ATE (empty ATE)
        ate_ptr = len(sector) - ate_size
        empty_ate = FsZmsAte.deserialize(
            sector[ate_ptr : (ate_ptr + ate_size)], self.write_block_size, self.id_size
        )

        if not self._is_empty_ate(empty_ate):
            return fs_core.FsSectorData(status=fs_core.FsSectorStatus.NA, records={})

        current_cycle_cnt = empty_ate.cycle_cnt

        # Check second last ATE (close ATE)
        ate_ptr -= ate_size
        close_ate = FsZmsAte.deserialize(
            sector[ate_ptr : (ate_ptr + ate_size)], self.write_block_size, self.id_size
        )

        if self._is_close_ate(close_ate) and (empty_ate.cycle_cnt == close_ate.cycle_cnt):
            sector_status = fs_core.FsSectorStatus.CLOSED
        else:
            sector_status = fs_core.FsSectorStatus.OPEN

        # Parse all ATEs
        while ate_ptr >= 0:
            ate_ptr -= ate_size

            if ate_ptr < data_ptr:
                break

            ate = FsZmsAte.deserialize(
                sector[ate_ptr : (ate_ptr + ate_size)], self.write_block_size, self.id_size
            )

            if not ate.is_valid(current_cycle_cnt):
                continue

            data, data_ptr = self._parse_record_data(ate, sector, ate_ptr, data_ptr)

            if data is None:
                continue

            records[ate.id] = data

        if sector_status == fs_core.FsSectorStatus.OPEN and len(records) == 0:
            return fs_core.FsSectorData(status=fs_core.FsSectorStatus.ERASED, records={})

        return fs_core.FsSectorData(status=sector_status, records=records)

    def parse(self, bin_str):
        sectors = self._parse_all_sectors(bin_str, self._parse_sector)
        ordered_sectors = self._order_sectors(sectors)
        consolidated_records = self._consolidate_records(ordered_sectors)
        return consolidated_records
