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
                if sender_controller.inquiry_mode == hci.HCI_EXTENDED_INQUIRY_MODE:
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
                elif sender_controller.inquiry_mode == hci.HCI_INQUIRY_WITH_RSSI_MODE:
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

    def io_capability_request(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
        io_capability: int,
        oob_data_present: int,
        authentication_requirements: int,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    c.on_hci_io_capability_response_ind,
                    sender_controller.public_address,
                    io_capability,
                    oob_data_present,
                    authentication_requirements,
                )

                loop.call_soon(
                    c.on_hci_io_capability_request_ind,
                    sender_controller.public_address,
                )
                return

    def io_capability_response(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
        io_capability: int,
        oob_data_present: int,
        authentication_requirements: int,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    c.on_hci_io_capability_response_ind,
                    sender_controller.public_address,
                    io_capability,
                    oob_data_present,
                    authentication_requirements,
                )

                loop.call_soon(
                    c.on_hci_user_confirmation_request_ind,
                    sender_controller.public_address,
                    123456,
                )

                loop.call_soon(
                    sender_controller.on_hci_user_confirmation_request_ind,
                    c.public_address,
                    123456,
                )
                return

    def numeric_comparison_failed(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    c.on_numeric_comparison_failed_ind,
                    sender_controller.public_address,
                )
                loop.call_soon(
                    sender_controller.on_numeric_comparison_failed_ind,
                    c.public_address,
                )
                return

    def dhkey_check(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    c.on_dhkey_check_ind,
                    sender_controller.public_address,
                )
                return

    def dhkey_check_not_accepted(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    c.on_dhkey_check_not_accepted_ind,
                    sender_controller.public_address,
                )
                loop.call_soon(
                    sender_controller.on_dhkey_check_not_accepted_ind,
                    c.public_address,
                )
                return

    def dhkey_check_accepted(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    c.on_dhkey_check_accepted_ind,
                    sender_controller.public_address,
                )
                loop.call_soon(
                    sender_controller.on_dhkey_check_accepted_ind,
                    c.public_address,
                )

                loop.call_soon(
                    c.on_link_key_notification_ind,
                    sender_controller.public_address,
                    bytes.fromhex('00112233445566778899aabbccddeeff'),
                    hci.LinkKeyType.AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P_256,
                    16,
                )
                loop.call_soon(
                    sender_controller.on_link_key_notification_ind,
                    c.public_address,
                    bytes.fromhex('00112233445566778899aabbccddeeff'),
                    hci.LinkKeyType.AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P_256,
                    16,
                )
                return

    def start_encryption_req(
        self,
        sender_controller: bt_sim_controller,
        peer_address: hci.Address,
    ):
        loop = asyncio.get_running_loop()
        for c in self.controllers:
            if c == sender_controller:
                continue

            if c.public_address == peer_address:
                loop.call_soon(
                    c.on_start_encryption_req_accepted_ind,
                    sender_controller.public_address,
                    0x02,
                )
                loop.call_soon(
                    sender_controller.on_start_encryption_req_accepted_ind,
                    c.public_address,
                    0x02,
                )
                return
