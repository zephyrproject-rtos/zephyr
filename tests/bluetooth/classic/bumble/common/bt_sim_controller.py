# Copyright 2025 NXP
# Copyright (c) 2026 Silicon Laboratories Inc.
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import dataclasses
import logging

import bt_sim_hci as sim_hci
from bumble import hci, lmp
from bumble.controller import Connection, Controller, SspState

logger = logging.getLogger(__name__)

# Bumble's pairing always produces a 128-bit link key.
ENCRYPTION_KEY_SIZE = 16


@lmp.Packet.subclass
@dataclasses.dataclass
class LmpStartEncryptionReq(lmp.Packet):
    '''
    See Bluetooth spec @ Vol 2, Part C - 4.2.5 Encryption

    Bumble's lmp module has no such packet. Nothing on the simulated link is
    encrypted, so the random number of the real PDU is left out.
    '''

    opcode = lmp.Opcode.LMP_START_ENCRYPTION_REQ


class bt_sim_controller(Controller):
    """Bumble's controller plus the BR/EDR behaviour it does not implement.

    Connections, Secure Simple Pairing, remote features and names come from
    Bumble. What is added: inquiry, class of device, encryption and the
    configuration commands the Zephyr Host requires at bt_enable().
    """

    # Not BR_EDR_NOT_SUPPORTED, which Bumble sets by default.
    lmp_features: hci.LmpFeatureMask = hci.LmpFeatureMask.LE_SUPPORTED_CONTROLLER

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.iac_lap = [hci.HCI_GENERAL_INQUIRY_LAP]
        self.class_of_device = 0
        self.inquiry_mode = hci.HCI_STANDARD_INQUIRY_MODE
        self.inquiry_task = None
        self.deferred_events = None
        self.pending_authentication = set()
        self.pairing_reported = set()

    def on_hci_command_packet(self, command: hci.HCI_Command) -> None:
        # Bumble sends the events of a procedure from inside the command handler,
        # the Host expects them after the command's own response.
        self.deferred_events = []
        super().on_hci_command_packet(command)
        events, self.deferred_events = self.deferred_events, None
        for event in events:
            self.send_hci_packet(event)

    def on_hci_command(self, command: hci.HCI_Command) -> hci.HCI_StatusReturnParameters | None:
        # Bumble only answers an unsupported command if it is known to be synchronous,
        # the Host would wait for the answer to any other until its command timeout.
        if isinstance(command, hci.HCI_SyncCommand):
            return super().on_hci_command(command)
        logger.warning('--- Unsupported command %s', command)
        self._send_hci_command_status(hci.HCI_ErrorCode.UNKNOWN_HCI_COMMAND_ERROR, command.op_code)
        return None

    def send_hci_packet(self, packet: hci.HCI_Packet) -> None:
        if self.deferred_events is not None and not isinstance(
            packet, hci.HCI_Command_Complete_Event | hci.HCI_Command_Status_Event
        ):
            self.deferred_events.append(packet)
            return
        # Bumble's Secure Simple Pairing reports completion to both sides and once
        # per failure. A Host gets one report of a pairing, and Authentication
        # Complete only for its own Authentication Requested.
        if isinstance(packet, hci.HCI_Authentication_Complete_Event):
            if packet.connection_handle not in self.pending_authentication:
                return
            self.pending_authentication.remove(packet.connection_handle)
        elif isinstance(packet, hci.HCI_Simple_Pairing_Complete_Event):
            if packet.bd_addr in self.pairing_reported:
                return
            self.pairing_reported.add(packet.bd_addr)
        elif isinstance(packet, hci.HCI_Disconnection_Complete_Event):
            self.pending_authentication.discard(packet.connection_handle)
        super().send_hci_packet(packet)

    def _start_pairing(self, connection: Connection, is_initiator: bool) -> None:
        # Bumble keeps the state of an earlier pairing on the link.
        connection.ssp_state = SspState(is_initiator=is_initiator)
        self.pairing_reported.discard(connection.peer_address)

    def on_lmp_packet(self, sender_address: hci.Address, packet: lmp.Packet) -> None:
        if isinstance(packet, LmpStartEncryptionReq):
            self._on_lmp_start_encryption_req(sender_address)
            return
        if isinstance(packet, lmp.LmpIoCapabilityReq):
            # A new pairing on a link that has paired before starts over.
            connection = self.classic_connections.get(sender_address)
            if connection is not None:
                self._start_pairing(connection, is_initiator=False)
        super().on_lmp_packet(sender_address, packet)

    def on_hci_reset_command(
        self, command: hci.HCI_Reset_Command
    ) -> hci.HCI_StatusReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.2 Reset Command
        '''
        self._stop_inquiry()
        return super().on_hci_reset_command(command)

    def on_hci_write_page_timeout_command(
        self, _command: hci.HCI_Write_Page_Timeout_Command
    ) -> hci.HCI_StatusReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.16 Write Page Timeout Command
        '''
        return hci.HCI_StatusReturnParameters(hci.HCI_ErrorCode.SUCCESS)

    def on_hci_read_default_link_policy_settings_command(
        self, _command: sim_hci.HCI_Read_Default_Link_Policy_Settings_Command
    ) -> sim_hci.HCI_Read_Default_Link_Policy_Settings_ReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.2.11 Read Default Link Policy Settings Command
        '''
        # pylint: disable-next=unexpected-keyword-arg  # inherited from a Bumble class
        return sim_hci.HCI_Read_Default_Link_Policy_Settings_ReturnParameters(
            status=hci.HCI_ErrorCode.SUCCESS, default_link_policy_settings=0
        )

    def on_hci_host_number_of_completed_packets_command(self, _command: hci.HCI_Command) -> None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.40 Host Number Of Completed Packets Command
        '''

    def on_hci_read_class_of_device_command(
        self, _command: hci.HCI_Read_Class_Of_Device_Command
    ) -> hci.HCI_Read_Class_Of_Device_ReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.25 Read Class of Device Command
        '''
        return hci.HCI_Read_Class_Of_Device_ReturnParameters(
            status=hci.HCI_ErrorCode.SUCCESS, class_of_device=self.class_of_device
        )

    def on_hci_write_class_of_device_command(
        self, command: hci.HCI_Write_Class_Of_Device_Command
    ) -> hci.HCI_StatusReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.26 Write Class of Device Command
        '''
        self.class_of_device = command.class_of_device
        return hci.HCI_StatusReturnParameters(hci.HCI_ErrorCode.SUCCESS)

    def on_hci_write_current_iac_lap_command(
        self, command: sim_hci.HCI_Write_Current_IAC_LAP_Command
    ) -> hci.HCI_StatusReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.45 Write Current IAC LAP Command
        '''
        self.iac_lap = list(command.iac_lap)
        return hci.HCI_StatusReturnParameters(hci.HCI_ErrorCode.SUCCESS)

    def on_hci_write_inquiry_mode_command(
        self, command: hci.HCI_Write_Inquiry_Mode_Command
    ) -> hci.HCI_StatusReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.3.50 Write Inquiry Mode Command
        '''
        self.inquiry_mode = command.inquiry_mode
        return hci.HCI_StatusReturnParameters(hci.HCI_ErrorCode.SUCCESS)

    def on_hci_inquiry_command(self, command: hci.HCI_Inquiry_Command) -> None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.1.1 Inquiry Command
        '''
        self._send_hci_command_status(hci.HCI_COMMAND_STATUS_PENDING, command.op_code)
        self._stop_inquiry()
        self.inquiry_task = asyncio.create_task(self._inquiry(command.lap, command.inquiry_length))

    def on_hci_inquiry_cancel_command(
        self, _command: hci.HCI_Inquiry_Cancel_Command
    ) -> hci.HCI_StatusReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.1.2 Inquiry Cancel Command
        '''
        self._stop_inquiry()
        return hci.HCI_StatusReturnParameters(hci.HCI_ErrorCode.SUCCESS)

    def _stop_inquiry(self) -> None:
        if self.inquiry_task is not None:
            self.inquiry_task.cancel()
            self.inquiry_task = None

    async def _inquiry(self, lap: int, inquiry_length: int) -> None:
        # Inquiry Length is in units of 1.28 s.
        for _ in range(inquiry_length):
            self._send_inquiry_results(lap)
            await asyncio.sleep(1.28)
        self.inquiry_task = None
        self.send_hci_packet(hci.HCI_Inquiry_Complete_Event(status=hci.HCI_ErrorCode.SUCCESS))

    def _send_inquiry_results(self, lap: int) -> None:
        for peer in self.link.controllers:
            if peer != self and lap in peer.iac_lap and peer.classic_scan_enable & 0x01:
                self.send_hci_packet(self._inquiry_result(peer))

    def _inquiry_result(self, peer: 'bt_sim_controller') -> hci.HCI_Event:
        if self.inquiry_mode == hci.HCI_EXTENDED_INQUIRY_MODE:
            return hci.HCI_Extended_Inquiry_Result_Event(
                num_responses=1,
                bd_addr=peer.public_address,
                page_scan_repetition_mode=0,
                reserved=0,
                class_of_device=peer.class_of_device,
                clock_offset=0,
                rssi=-60,
                extended_inquiry_response=bytes(240),
            )
        if self.inquiry_mode == hci.HCI_INQUIRY_WITH_RSSI_MODE:
            return hci.HCI_Inquiry_Result_With_RSSI_Event(
                bd_addr=[peer.public_address],
                page_scan_repetition_mode=[0],
                reserved=[0],
                class_of_device=[peer.class_of_device],
                clock_offset=[0],
                rssi=[-60],
            )
        return hci.HCI_Inquiry_Result_Event(
            bd_addr=[peer.public_address],
            page_scan_repetition_mode=[0],
            reserved_0=[0],
            reserved_1=[0],
            class_of_device=[peer.class_of_device],
            clock_offset=[0],
        )

    def on_hci_authentication_requested_command(
        self, command: hci.HCI_Authentication_Requested_Command
    ) -> None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.1.15 Authentication Requested Command

        Bumble goes straight to the IO capabilities. A real controller first
        asks the Host for a stored link key.
        '''
        connection = self.find_classic_connection_by_handle(command.connection_handle)
        if connection is None:
            self._send_hci_command_status(
                hci.HCI_ErrorCode.UNKNOWN_CONNECTION_IDENTIFIER_ERROR, command.op_code
            )
            return
        if command.connection_handle in self.pending_authentication:
            self._send_hci_command_status(
                hci.HCI_ErrorCode.HOST_BUSY_PAIRING_ERROR, command.op_code
            )
            return
        self._send_hci_command_status(hci.HCI_COMMAND_STATUS_PENDING, command.op_code)
        self.pending_authentication.add(command.connection_handle)
        self._start_pairing(connection, is_initiator=True)
        self.send_hci_packet(hci.HCI_Link_Key_Request_Event(bd_addr=connection.peer_address))

    def on_hci_set_connection_encryption_command(
        self, command: hci.HCI_Set_Connection_Encryption_Command
    ) -> None:
        '''
        See Bluetooth spec Vol 4, Part E - 7.1.16 Set Connection Encryption Command
        '''
        connection = self.find_classic_connection_by_handle(command.connection_handle)
        if connection is None:
            self._send_hci_command_status(
                hci.HCI_ErrorCode.UNKNOWN_CONNECTION_IDENTIFIER_ERROR, command.op_code
            )
            return
        pending = self.classic_pending_commands.get(connection.peer_address, {}).get(
            lmp.Opcode.LMP_START_ENCRYPTION_REQ
        )
        if command.encryption_enable == 0 or (pending is not None and not pending.done()):
            self._send_hci_command_status(
                hci.HCI_ErrorCode.COMMAND_DISALLOWED_ERROR, command.op_code
            )
            return
        self._send_hci_command_status(hci.HCI_COMMAND_STATUS_PENDING, command.op_code)

        future = self.send_lmp_packet(connection.peer_address, LmpStartEncryptionReq())

        def on_response(future: asyncio.Future[int]) -> None:
            # The link may have gone while the peer was answering.
            if self.classic_connections.get(connection.peer_address) is connection:
                self._send_encryption_change(connection.handle, future.result())

        future.add_done_callback(on_response)

    def _on_lmp_start_encryption_req(self, peer_address: hci.Address) -> None:
        connection = self.classic_connections.get(peer_address)
        if connection is None:
            logger.warning('!!! no connection for %s', peer_address)
            return
        self.send_lmp_packet(peer_address, lmp.LmpAccepted(lmp.Opcode.LMP_START_ENCRYPTION_REQ))
        self._send_encryption_change(connection.handle, hci.HCI_ErrorCode.SUCCESS)

    def _send_encryption_change(self, connection_handle: int, status: int) -> None:
        enabled = hci.HCI_Encryption_Change_Event.Enabled
        self.send_hci_packet(
            hci.HCI_Encryption_Change_Event(
                status=status,
                connection_handle=connection_handle,
                encryption_enabled=(
                    enabled.AES_CCM if status == hci.HCI_ErrorCode.SUCCESS else enabled.OFF
                ),
            )
        )

    def on_hci_read_encryption_key_size_command(
        self, command: hci.HCI_Read_Encryption_Key_Size_Command
    ) -> hci.HCI_Read_Encryption_Key_Size_ReturnParameters:
        '''
        See Bluetooth spec Vol 4, Part E - 7.5.7 Read Encryption Key Size Command
        '''
        connection = self.find_classic_connection_by_handle(command.connection_handle)
        return hci.HCI_Read_Encryption_Key_Size_ReturnParameters(
            status=(
                hci.HCI_ErrorCode.SUCCESS
                if connection is not None
                else hci.HCI_ErrorCode.UNKNOWN_CONNECTION_IDENTIFIER_ERROR
            ),
            connection_handle=command.connection_handle,
            key_size=ENCRYPTION_KEY_SIZE if connection is not None else 0,
        )
