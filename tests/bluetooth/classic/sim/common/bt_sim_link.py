# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging

import bt_sim_ll as sim_ll
import bumble.hci as hci
from bt_sim_controller import bt_sim_controller
from bumble.link import LocalLink

logger = logging.getLogger(__name__)


class bt_sim_local_link(LocalLink):
    def start_inquiry(
        self,
        sender_controller: bt_sim_controller,
        lap,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c != sender_controller and lap in c.iac_lap and (c.classic_scan_enable & 0x01):
                if sender_controller.inquiry_mode is hci.HCI_EXTENDED_INQUIRY_MODE:
                    inquiry_pdu = sim_ll.ExtendedInquiryInd(
                        num_responses=1,
                        bd_address=c.public_address,
                        Page_Scan_Repetition_Mode=0x00,
                        Reserved=0x00,
                        Class_Of_Device=c.class_of_device,
                        Clock_Offset=0x00,
                        RSSI=-60,
                        Extended_Inquiry_Response=b'\x00' * 240,
                    )
                elif sender_controller.inquiry_mode is hci.HCI_INQUIRY_WITH_RSSI_MODE:
                    inquiry_pdu = sim_ll.InquiryRssiInd(
                        num_responses=1,
                        bd_address=[c.public_address],
                        Page_Scan_Repetition_Mode=[0x00],
                        Reserved=[0x00],
                        Class_Of_Device=[c.class_of_device],
                        Clock_Offset=[0x00],
                        RSSI=[-60],
                    )
                else:
                    inquiry_pdu = sim_ll.InquiryInd(
                        num_responses=1,
                        bd_address=[c.public_address],
                        Page_Scan_Repetition_Mode=[0x00],
                        Reserved=[0x00],
                        Class_Of_Device=[c.class_of_device],
                        Clock_Offset=[0x00],
                    )
                loop.call_soon(sender_controller.on_inquiry_result_pdu, inquiry_pdu)

    def read_remote_supported_features(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
        connection_handle: int,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    sender_controller.on_read_remote_supported_features_pdu,
                    c.lmp_features[:8],
                    connection_handle,
                )
                return
