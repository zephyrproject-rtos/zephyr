# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import dataclasses
from collections.abc import Sequence

from bumble import hci


class InquiryPdu:
    """Base Inquiry Physical Channel PDU class.

    Currently these messages don't really follow the LL spec, because LL protocol is
    context-aware and we don't have real physical transport.
    """


@dataclasses.dataclass
class InquiryInd(InquiryPdu):
    num_responses: int
    bd_address: Sequence[hci.Address]
    Page_Scan_Repetition_Mode: Sequence[int]
    Reserved: Sequence[int]
    Class_Of_Device: Sequence[int]
    Clock_Offset: Sequence[int]

    def __str__(self):
        return (
            f"InquiryRssiInd("
            f"num_responses={self.num_responses}, "
            f"bd_address={self.bd_address}, "
            f"PSRM={self.Page_Scan_Repetition_Mode}, "
            f"CoD={[hex(c) for c in self.Class_Of_Device]})"
        )


@dataclasses.dataclass
class InquiryRssiInd(InquiryPdu):
    num_responses: int
    bd_address: Sequence[hci.Address]
    Page_Scan_Repetition_Mode: Sequence[int]
    Reserved: Sequence[int]
    Class_Of_Device: Sequence[int]
    Clock_Offset: Sequence[int]
    RSSI: Sequence[int]

    def __str__(self):
        return (
            f"InquiryRssiInd("
            f"num_responses={self.num_responses}, "
            f"bd_address={self.bd_address}, "
            f"PSRM={self.Page_Scan_Repetition_Mode}, "
            f"CoD={[hex(c) for c in self.Class_Of_Device]}, "
            f"RSSI={self.RSSI})"
        )


@dataclasses.dataclass
class ExtendedInquiryInd(InquiryPdu):
    num_responses: int
    bd_address: hci.Address
    Page_Scan_Repetition_Mode: int
    Reserved: int
    Class_Of_Device: int
    Clock_Offset: int
    RSSI: int
    Extended_Inquiry_Response: bytes  # Size is 240

    def __str__(self):
        return (
            f"ExtendedInquiryInd("
            f"num_responses={self.num_responses}, "
            f"bd_address={self.bd_address}, "
            f"PSRM={self.Page_Scan_Repetition_Mode}, "
            f"CoD={hex(self.Class_Of_Device)}, "
            f"RSSI={self.RSSI},"
            f"Extended_Inquiry_Response={self.Extended_Inquiry_Response})"
        )
