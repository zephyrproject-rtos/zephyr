# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import struct
from collections.abc import Callable

import bt_sim_hci as sim_hci
import bt_sim_ll as sim_ll
from bumble import hci
from bumble.controller import Connection, ScoLink
from bumble.controller import Controller as controller
from bumble.core import PhysicalTransport

logger = logging.getLogger(__name__)


class bt_classic_discovery:
    def __init__(
        self,
        iac_lap,
        count,
        interval=1.25,
        on_inquiry: Callable | None = None,
        on_complete: Callable | None = None,
    ):
        self.flag = False
        self.count = count
        self.interval = interval
        self._task = None
        self.on_inquiry = on_inquiry
        self.on_complete = on_complete
        self._is_running = False
        self.iac_lap = iac_lap

    def start(
        self,
        count,
        interval=1.25,
        on_inquiry: Callable | None = None,
        on_complete: Callable | None = None,
    ):
        """Start periodic timeout callback"""
        self.flag = True
        self._is_running = True
        self.count = count
        self.interval = interval
        if on_inquiry:
            self.on_complete = on_inquiry
        if on_complete:
            self.on_complete = on_complete
        if self._task:
            self._task.cancel()
        self._task = asyncio.create_task(self._periodic_callback(interval))

    async def _periodic_callback(self, interval):
        """Execute callback periodically"""
        try:
            while self._is_running:
                await asyncio.sleep(interval)
                if self._is_running and self.on_inquiry:
                    if asyncio.iscoroutinefunction(self.on_inquiry):
                        await self.on_inquiry()
                    else:
                        self.on_inquiry()
                if self.count > 0:
                    self.count = self.count - 1
                if self.count == 0 and self.on_complete:
                    if asyncio.iscoroutinefunction(self.on_complete):
                        await self.on_complete()
                    else:
                        self.on_complete()
                    self.stop()
        except asyncio.CancelledError:
            pass

    def stop(self):
        """Stop periodic execution"""
        self.flag = False
        self._is_running = False
        if self._task:
            self._task.cancel()
            self._task = None


class bt_sim_controller(controller):
    # Enable BR/EDR feature
    lmp_features: bytes = bytes.fromhex(
        '0000000040000000'
    )  # BR/EDR Supported, LE Supported (Controller)

    inquiry_mode: int = 0
    page_timeout: int = 0x2000
    default_link_policy_settings: int = 0
    discovery: bt_classic_discovery = None
    iac_lap: list = [hci.HCI_GENERAL_INQUIRY_LAP]
    class_of_device: int = 0

    def start_inquiry(self, lap) -> None:
        logger.debug("[%s] >>> Inquiry LAP %s", self.name, lap)
        if self.link:
            self.link.start_inquiry(self, lap)

    def on_inquiry_complete(self) -> None:
        logger.debug("Inquiry complete")
        self.send_hci_packet(hci.HCI_Inquiry_Complete_Event(status=hci.HCI_SUCCESS))

    def on_inquiry_timeout(self) -> None:
        logger.debug("Inquiry timeout")
        self.start_inquiry(self.discovery.iac_lap)

    def on_hci_write_page_timeout_command(
        self, command: hci.HCI_Write_Page_Timeout_Command
    ) -> bytes | None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.18 Write Page Timeout Command
        '''
        self.page_timeout = command.page_timeout
        return bytes([hci.HCI_SUCCESS])

    def on_hci_write_inquiry_mode_command(
        self, command: hci.HCI_Write_Inquiry_Mode_Command
    ) -> bytes | None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.18 Write Inquiry Mode Command
        '''
        self.inquiry_mode = command.inquiry_mode
        return bytes([hci.HCI_SUCCESS])

    def on_hci_read_default_link_policy_settings_command(
        self, _command: sim_hci.HCI_Read_Default_Link_Policy_Settings_Command
    ) -> bytes | None:
        '''
        See Bluetooth spec 7.2.11 Read Default Link Policy Settings command
        '''
        return struct.pack(
            '<BH',
            hci.HCI_SUCCESS,
            self.default_link_policy_settings,
        )

    def on_inquiry_result_ind(self, packet: sim_ll.InquiryInd) -> None:
        self.send_hci_packet(
            hci.HCI_Inquiry_Result_Event(
                bd_addr=packet.bd_address,
                page_scan_repetition_mode=packet.Page_Scan_Repetition_Mode,
                reserved_0=packet.Reserved,
                reserved_1=packet.Reserved,
                class_of_device=packet.Class_Of_Device,
                clock_offset=packet.Clock_Offset,
            )
        )

    def on_inquiry_result_with_rssi_ind(self, packet: sim_ll.InquiryRssiInd) -> None:
        self.send_hci_packet(
            hci.HCI_Inquiry_Result_With_RSSI_Event(
                bd_addr=packet.bd_address,
                page_scan_repetition_mode=packet.Page_Scan_Repetition_Mode,
                reserved=packet.Reserved,
                class_of_device=packet.Class_Of_Device,
                clock_offset=packet.Clock_Offset,
                rssi=packet.RSSI,
            )
        )

    def on_extended_inquiry_result_ind(self, packet: sim_ll.ExtendedInquiryInd) -> None:
        self.send_hci_packet(
            hci.HCI_Extended_Inquiry_Result_Event(
                num_responses=packet.num_responses,
                bd_addr=packet.bd_address,
                page_scan_repetition_mode=packet.Page_Scan_Repetition_Mode,
                reserved=packet.Reserved,
                class_of_device=packet.Class_Of_Device,
                clock_offset=packet.Clock_Offset,
                rssi=packet.RSSI,
                extended_inquiry_response=packet.Extended_Inquiry_Response,
            )
        )

    def on_inquiry_result_pdu(self, packet: sim_ll.InquiryPdu) -> None:
        logger.debug("[%s] <<< Inquiry PDU: %s", self.name, packet)
        if isinstance(packet, sim_ll.InquiryInd):
            self.on_inquiry_result_ind(packet)
        elif isinstance(packet, sim_ll.InquiryRssiInd):
            self.on_inquiry_result_with_rssi_ind(packet)
        elif isinstance(packet, sim_ll.ExtendedInquiryInd):
            self.on_extended_inquiry_result_ind(packet)
        else:
            logger.warning("[%s] Unknown inquiry PDU type: %s", self.name, type(packet))

    def on_hci_inquiry_command(self, command: hci.HCI_Inquiry_Command) -> None:
        '''
        See Bluetooth spec 7.1.1 Inquiry command
        '''
        logger.debug(
            f'Inqury LAP {command.lap} length {command.inquiry_length} '
            f'num_responses {command.num_responses}'
        )

        self.discovery = bt_classic_discovery(
            command.lap,
            command.inquiry_length,
            on_inquiry=self.on_inquiry_timeout,
            on_complete=self.on_inquiry_complete,
        )
        self.discovery.start(command.inquiry_length)

        self.send_hci_packet(
            hci.HCI_Command_Status_Event(
                status=hci.HCI_SUCCESS,
                num_hci_command_packets=1,
                command_opcode=command.op_code,
            )
        )

        self.start_inquiry(command.lap)

    def on_hci_inquiry_cancel_command(
        self, _command: hci.HCI_Inquiry_Cancel_Command
    ) -> bytes | None:
        '''
        See Bluetooth spec 7.1.2 Inquiry Cancel command
        '''
        if self.discovery is not None:
            self.discovery.stop()

        return bytes([hci.HCI_SUCCESS])

    def on_hci_write_current_iac_lap_command(
        self, command: sim_hci.HCI_Write_Current_Iac_Lap_Command
    ) -> bytes | None:
        '''
        See Bluetooth spec 7.3.45 Write Current IAC LAP command
        '''
        self.iac_lap = []

        for iac in command.iac_lap:
            self.iac_lap.append(iac)
        return bytes([hci.HCI_SUCCESS])

    def on_hci_read_class_of_device_command(
        self, _command: hci.HCI_Read_Class_Of_Device_Command
    ) -> bytes | None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.25 Read Class of Device Command
        '''
        return struct.pack('<BI', hci.HCI_SUCCESS, self.class_of_device)[:4]

    def on_hci_write_class_of_device_command(
        self, command: hci.HCI_Write_Class_Of_Device_Command
    ) -> bytes | None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.26 Write Class of Device Command
        '''
        self.class_of_device = command.class_of_device
        return bytes([hci.HCI_SUCCESS])

    def on_classic_connection_request(self, peer_address: hci.Address, link_type: int) -> None:
        if link_type == hci.HCI_Connection_Complete_Event.LinkType.ACL:
            self.classic_connections[peer_address] = Connection(
                controller=self,
                handle=0,
                role=hci.Role.PERIPHERAL,
                peer_address=peer_address,
                link=self.link,
                transport=PhysicalTransport.BR_EDR,
                link_type=link_type,
                classic_allow_role_switch=self.classic_allow_role_switch,
            )
        else:
            self.sco_links[peer_address] = ScoLink(
                handle=0,
                link_type=link_type,
                peer_address=peer_address,
            )
        self.send_hci_packet(
            hci.HCI_Connection_Request_Event(
                bd_addr=peer_address,
                class_of_device=self.class_of_device,
                link_type=link_type,
            )
        )

    def on_read_remote_supported_features_pdu(
        self, lmp_features: bytes, connection_handle: int
    ) -> None:
        logger.debug("[%s] <<< Read Remote Supported Features PDU %s", self.name, lmp_features)
        self.send_hci_packet(
            hci.HCI_Read_Remote_Supported_Features_Complete_Event(
                status=hci.HCI_SUCCESS,
                connection_handle=connection_handle,
                lmp_features=lmp_features,
            )
        )

    def read_remote_supported_features(
        self, peer_address: hci.Address, connection_handle: int
    ) -> None:
        logger.debug("[%s] >>> Peer Address %s", self.name, peer_address)
        if self.link:
            self.link.read_remote_supported_features(self, peer_address, connection_handle)
        else:
            logger.warning("[%s] No link available", self.name)

    def on_hci_read_remote_supported_features_command(
        self, command: hci.HCI_Read_Remote_Supported_Features_Command
    ) -> None:
        '''
        See Bluetooth spec 7.1.1 Inquiry command
        '''
        logger.debug(f'Connect handle {command.connection_handle}')

        peer_address = None

        for address, connection in self.classic_connections.items():
            if connection.handle == command.connection_handle:
                peer_address = address
                break

        if peer_address is None:
            status = hci.HCI_UNKNOWN_CONNECTION_IDENTIFIER_ERROR
        else:
            status = hci.HCI_SUCCESS

        self.send_hci_packet(
            hci.HCI_Command_Status_Event(
                status=status,
                num_hci_command_packets=1,
                command_opcode=command.op_code,
            )
        )

        if peer_address is not None:
            self.read_remote_supported_features(peer_address, command.connection_handle)
        else:
            logger.warning("[%s] Unknown connection handle", self.name)

    def on_hci_host_number_of_completed_packets_command(
        self, command: sim_hci.HCI_Host_Number_Of_Completed_Packets_Command
    ) -> None:
        '''
        See Bluetooth spec 7.1.1 Inquiry command
        '''
        logger.debug(
            f'Connect handle {command.connection_handle}'
            f'num_of_completed_packets {command.num_of_completed_packets}'
        )
