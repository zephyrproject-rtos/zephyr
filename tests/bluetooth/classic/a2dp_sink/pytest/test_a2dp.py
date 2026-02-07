# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import sys

from bumble.a2dp import (
    SbcMediaCodecInformation,
)
from bumble.avdtp import (
    A2DP_SBC_CODEC_TYPE,
    AVDTP_AUDIO_MEDIA_TYPE,
    AVDTP_CLOSING_STATE,
    AVDTP_CONFIGURED_STATE,
    AVDTP_IDLE_STATE,
    AVDTP_MEDIA_TRANSPORT_SERVICE_CATEGORY,
    AVDTP_OPEN_STATE,
    AVDTP_STREAMING_STATE,
    AVDTP_TSEP_SNK,
    Listener,
    LocalStreamEndPoint,
    MediaCodecCapabilities,
    MediaPacketPump,
    Protocol,
    ServiceCapabilities,
    Stream,
    StreamEndPointProxy,
)
from bumble.core import (
    BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE,
    BT_AUDIO_SOURCE_SERVICE,
    BT_AVDTP_PROTOCOL_ID,
    BT_BR_EDR_TRANSPORT,
    BT_L2CAP_PROTOCOL_ID,
    InvalidStateError,
    ProtocolError,
)
from bumble.device import Device
from bumble.hci import (
    HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR,
    Address,
    HCI_Write_Page_Timeout_Command,
)
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.rtp import MediaPacket
from bumble.sdp import (
    SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
    SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
    SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
    SDP_PUBLIC_BROWSE_ROOT,
    SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
    SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
    DataElement,
    ServiceAttribute,
)
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


async def device_power_on(device, max_attempts: int = 5, retry_delay: float = 1.0) -> None:
    attempts = 0
    while True:
        try:
            await device.power_on()
            break
        except Exception as e:
            attempts += 1
            logger.warning(
                "Failed to power on device (attempt %d/%d): %s",
                attempts,
                max_attempts,
                e,
            )
            if attempts >= max_attempts:
                # Re-raise the last exception to fail fast instead of hanging indefinitely.
                raise
            await asyncio.sleep(retry_delay)


async def wait_for_shell_response(dut, message, max_time=10):
    found = False
    lines = []
    try:
        for _ in range(0, max_time):
            if found is False:
                read_lines = dut.readlines()
                logger.info(f"{read_lines}")
                for line in read_lines:
                    if message in line:
                        found = True
                        break
                lines = lines + read_lines
                if found:
                    break
                await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e
    return found, lines


async def send_cmd_to_iut(shell, dut, cmd, parse=None, max_time=10):
    found = False
    lines = shell.exec_command(cmd)
    if parse is not None:
        for line in lines:
            if parse in line:
                found = True
                break
        if found is False:
            found, lines = await wait_for_shell_response(dut, parse, max_time)
    else:
        found = True
    logger.info(f'{lines}')
    assert found is True
    return lines


async def bumble_acl_connect(shell, dut, device, target_address):
    connection = None
    try:
        connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
        logger.info(f'=== Connected to {connection.peer_address}!')
    except Exception as e:
        logger.error(f'Fail to connect to {target_address}!')
        raise e
    return connection


async def bumble_acl_disconnect(shell, dut, device, connection):
    await device.disconnect(
        connection, reason=HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR
    )
    found, lines = await wait_for_shell_response(dut, "Disconnected:")
    logger.info(f'lines : {lines}')
    assert found is True
    return found, lines


class Delegate(PairingDelegate):
    def __init__(
        self,
        dut,
        io_capability,
    ):
        super().__init__(
            io_capability,
        )
        self.dut = dut

    async def confirm(self, auto: bool = False):
        """
        Respond yes or no to a Pairing confirmation question.
        The `auto` parameter stands for automatic confirmation.
        """
        return True


def source_codec_capabilities():
    return MediaCodecCapabilities(
        media_type=AVDTP_AUDIO_MEDIA_TYPE,
        media_codec_type=A2DP_SBC_CODEC_TYPE,
        media_codec_information=SbcMediaCodecInformation(
            sampling_frequency=SbcMediaCodecInformation.SamplingFrequency.SF_48000
            | SbcMediaCodecInformation.SamplingFrequency.SF_44100
            | SbcMediaCodecInformation.SamplingFrequency.SF_32000
            | SbcMediaCodecInformation.SamplingFrequency.SF_16000,
            channel_mode=SbcMediaCodecInformation.ChannelMode.MONO
            | SbcMediaCodecInformation.ChannelMode.DUAL_CHANNEL
            | SbcMediaCodecInformation.ChannelMode.STEREO
            | SbcMediaCodecInformation.ChannelMode.JOINT_STEREO,
            block_length=SbcMediaCodecInformation.BlockLength.BL_4
            | SbcMediaCodecInformation.BlockLength.BL_8
            | SbcMediaCodecInformation.BlockLength.BL_12
            | SbcMediaCodecInformation.BlockLength.BL_16,
            subbands=SbcMediaCodecInformation.Subbands.S_4 | SbcMediaCodecInformation.Subbands.S_8,
            allocation_method=SbcMediaCodecInformation.AllocationMethod.LOUDNESS
            | SbcMediaCodecInformation.AllocationMethod.SNR,
            minimum_bitpool_value=2,
            maximum_bitpool_value=53,
        ),
    )


def sbc_codec_configuration():
    return MediaCodecCapabilities(
        media_type=AVDTP_AUDIO_MEDIA_TYPE,
        media_codec_type=A2DP_SBC_CODEC_TYPE,
        media_codec_information=SbcMediaCodecInformation(
            sampling_frequency=SbcMediaCodecInformation.SamplingFrequency.SF_48000,
            channel_mode=SbcMediaCodecInformation.ChannelMode.MONO,
            block_length=SbcMediaCodecInformation.BlockLength.BL_16,
            subbands=SbcMediaCodecInformation.Subbands.S_8,
            allocation_method=SbcMediaCodecInformation.AllocationMethod.LOUDNESS,
            minimum_bitpool_value=20,
            maximum_bitpool_value=30,
        ),
    )


AVDTP_PSM = 0x0019
AVDTP_VERSION = 0x0100
SDP_SERVICE_RECORDS_A2DP = {
    0x00010001: [
        ServiceAttribute(
            SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
            DataElement.unsigned_integer_32(0x00010001),
        ),
        ServiceAttribute(
            SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(SDP_PUBLIC_BROWSE_ROOT)]),
        ),
        ServiceAttribute(
            SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(BT_AUDIO_SOURCE_SERVICE)]),
        ),
        ServiceAttribute(
            SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_L2CAP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVDTP_PSM),
                        ]
                    ),
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_AVDTP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVDTP_VERSION),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE),
                            DataElement.unsigned_integer_16(AVDTP_VERSION),
                        ]
                    )
                ]
            ),
        ),
    ]
}


class local_stream(Stream):
    async def configure(self, configuration) -> None:
        if self.state != AVDTP_IDLE_STATE:
            raise InvalidStateError('current state is not IDLE')

        await self.remote_endpoint.set_configuration(self.local_endpoint.seid, configuration)
        self.change_state(AVDTP_CONFIGURED_STATE)


async def local_create_stream(
    self, source: LocalStreamEndPoint, sink: StreamEndPointProxy, configuration
) -> Stream:
    # Check that the source isn't already used in a stream
    if source.in_use:
        raise InvalidStateError('source already in use')

    # Create or reuse a new stream to associate the source and the sink
    if source.seid in self.streams:
        stream = self.streams[source.seid]
    else:
        stream = local_stream(self, source, sink)
        self.streams[source.seid] = stream

    # The stream can now be configured
    await stream.configure(configuration)

    return stream


async def generate_packets(packet_count):
    timestamp = 0
    for sequence_number in range(packet_count):
        payload = bytes([sequence_number % 256])
        packet = MediaPacket(2, 0, 0, 0, sequence_number, timestamp, 0, [], 96, payload)
        packet.timestamp_seconds = timestamp / 44100
        timestamp += 10
        yield packet


async def a2dp_sink_case_register_ep(hci_port, shell, dut, address, snoop_file) -> None:
    """
    Test API register_ep
    """
    logger.info('<<< a2dp_sink_case_register_ep ...')

    async with await open_transport_or_link(hci_port) as _:
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "a2dp_sink register_cb", 'success')
        logger.info("stack only supports sbc")
        await send_cmd_to_iut(
            shell,
            dut,
            "a2dp_sink register_ep sink meg",
            'register_ep - <type: sink or source> <value: sbc>',
        )
        await send_cmd_to_iut(
            shell, dut, "a2dp_sink register_ep sink sbc", 'SBC sink endpoint is registered'
        )


async def a2dp_sink_case_connect(hci_port, shell, dut, address, snoop_file) -> None:
    """
    Test API a2dp connect
    """
    logger.info('<<< a2dp_sink_case_connect ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found = False
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        logger.info("a2dp source creates an a2dp connection to sink")
        err = 0
        try:
            await Protocol.connect(connection)
        except ProtocolError as error:
            err = error.error_code
        assert err == 0x03

        logger.info("create l2cap authentication and encryption")
        await device.authenticate(connection)
        await device.encrypt(connection)
        found = False
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        logger.info("create a2dp connection after authentication successfully")
        await Protocol.connect(connection)
        found, _ = await wait_for_shell_response(dut, "a2dp connected")
        assert found is True

        await bumble_acl_disconnect(shell, dut, device, connection)

        logger.info("create environment: authenticate fail.")

        async def fail_confirm(self, auto: bool = False):
            """
            Respond yes or no to a Pairing confirmation question.
            The `auto` parameter stands for automatic confirmation.
            """
            return False

        success_confirm = delegate.confirm
        delegate.confirm = fail_confirm
        await device.keystore.delete_all()
        await send_cmd_to_iut(shell, dut, "br clear all", None)

        logger.info("create acl connection")
        target_address = address.split(" ")[0]
        await bumble_acl_connect(shell, dut, device, target_address)
        found = False
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        logger.info("a2dp sink creates an a2dp connection to source")

        def on_avdtp_connection(server):
            source = server
            packet_pump = MediaPacketPump(generate_packets(100))
            source.add_source(
                codec_capabilities=source_codec_capabilities(), packet_pump=packet_pump
            )
            logger.info("a2dp_sink connect successfully")

        listener = Listener.for_device(device)
        listener.on('connection', on_avdtp_connection)

        shell.exec_command("a2dp_sink connect")
        await wait_for_shell_response(dut, "a2dp disconnected")

        logger.info("create env: authenticate successfully")
        delegate.confirm = success_confirm
        await device.keystore.delete_all()
        await send_cmd_to_iut(shell, dut, "br clear all", None)

        logger.info("create acl connection")
        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found = False
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await send_cmd_to_iut(shell, dut, 'a2dp_sink connect', 'a2dp connected')
        await send_cmd_to_iut(shell, dut, 'a2dp_sink disconnect', 'a2dp disconnected')

        await bumble_acl_disconnect(shell, dut, device, connection)


async def a2dp_sink_case_disconnect(hci_port, shell, dut, address, snoop_file) -> None:
    """
    Test API a2dp disconnect
    """
    logger.info('<<< a2dp_sink_case_disconnect ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found = False
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        logger.info("create l2cap authentication and encryption")
        await device.authenticate(connection)
        await device.encrypt(connection)
        found = False
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        logger.info("create a2dp connection")
        source = await Protocol.connect(connection)
        found, _ = await wait_for_shell_response(dut, "a2dp connected")

        logger.info("source creates a2dp disconnect")
        await source.l2cap_channel.disconnect()
        found, _ = await wait_for_shell_response(dut, "a2dp disconnected")

        await send_cmd_to_iut(shell, dut, 'a2dp_sink disconnect', 'a2dp is not connected')

        logger.info("create a2dp connection")
        await Protocol.connect(connection)
        found, _ = await wait_for_shell_response(dut, "a2dp connected")

        await send_cmd_to_iut(shell, dut, 'a2dp_sink disconnect', 'a2dp disconnected')
        await send_cmd_to_iut(shell, dut, 'a2dp_sink disconnect', 'a2dp is not connected')


async def a2dp_sink_case_discover_get_cap(hci_port, shell, dut, address, snoop_file) -> None:
    """
    Test API a2dp discover and get capabilities
    """
    logger.info('<<< a2dp_sink_case_discover_get_cap ...')

    async def a2dp_disconnect(protocol: Protocol):
        if protocol is not None:
            await protocol.l2cap_channel.disconnect()

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.sdp_service_records = SDP_SERVICE_RECORDS_A2DP

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found = False
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found = False
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        logger.info("a2dp source creates an a2dp connection to sink")
        source = await Protocol.connect(connection)
        found, _ = await wait_for_shell_response(dut, "a2dp connected")

        logger.info("a2dp source register endpoint")
        packet_pump = MediaPacketPump(generate_packets(100))
        source.add_source(codec_capabilities=source_codec_capabilities(), packet_pump=packet_pump)

        logger.info("a2dp source sends a2dp discover cmd to sink")
        remote_endpoints = await source.discover_remote_endpoints()
        for endpoint in remote_endpoints:
            logger.info(endpoint)
        assert len(remote_endpoints) >= 1

        logger.info("a2dp sink sends a2dp discover cmd to source")
        await send_cmd_to_iut(shell, dut, 'a2dp_sink discover_peer_eps', 'Bitpool Range:')

        await a2dp_disconnect(source)
        found, _ = await wait_for_shell_response(dut, "a2dp disconnected")
        assert found is True

        await bumble_acl_disconnect(shell, dut, device, connection)


async def a2dp_sink_case_sink_to_source(hci_port, shell, dut, address, snoop_file) -> None:
    """
    Test All a2dp commands are sent from sink to source
    """
    logger.info('<<< a2dp_sink_case_sink_to_source ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found = False
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found = False
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        source = await Protocol.connect(connection)
        packet_pump = MediaPacketPump(generate_packets(100))
        source_endpoint = source.add_source(
            codec_capabilities=source_codec_capabilities(), packet_pump=packet_pump
        )
        remote_endpoints = await source.discover_remote_endpoints()
        for endpoint in remote_endpoints:
            logger.info(endpoint)
        assert len(remote_endpoints) >= 1

        remote_sink_ep = list(remote_endpoints)[0]
        assert remote_sink_ep.in_use == 0
        assert remote_sink_ep.media_type == AVDTP_AUDIO_MEDIA_TYPE
        assert remote_sink_ep.tsep == AVDTP_TSEP_SNK

        set_config = sbc_codec_configuration()
        sbc_codec_config = [ServiceCapabilities(AVDTP_MEDIA_TRANSPORT_SERVICE_CATEGORY), set_config]

        origin_create_stream_func = source.create_stream
        source.create_stream = local_create_stream

        stream = await source.create_stream(
            source, source_endpoint, remote_sink_ep, sbc_codec_config
        )

        await stream.open()
        assert stream.state == AVDTP_OPEN_STATE

        await stream.start()
        assert stream.state == AVDTP_STREAMING_STATE

        await asyncio.sleep(3)

        await stream.stop()
        assert stream.state == AVDTP_OPEN_STATE

        await stream.close()
        assert stream.state in (AVDTP_CLOSING_STATE, AVDTP_IDLE_STATE)

        source.create_stream = origin_create_stream_func

        await bumble_acl_disconnect(shell, dut, device, connection)


async def a2dp_sink_case_source_to_sink(hci_port, shell, dut, address, snoop_file) -> None:
    """
    Test All a2dp commands are sent from source to sink
    """
    logger.info('<<< a2dp_sink_case_source_to_sink ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_RECORDS_A2DP

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        source_endpoint = None
        source = None

        def on_avdtp_connection(server):
            nonlocal source_endpoint
            nonlocal source
            source = server
            packet_pump = MediaPacketPump(generate_packets(100))
            source_endpoint = source.add_source(
                codec_capabilities=source_codec_capabilities(), packet_pump=packet_pump
            )
            logger.info("a2dp_sink connect successfully")

        # Create a listener to wait for AVDTP connections
        listener = Listener.for_device(device)
        listener.on('connection', on_avdtp_connection)

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found = False
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await send_cmd_to_iut(shell, dut, 'a2dp_sink connect', 'a2dp connected')

        await send_cmd_to_iut(shell, dut, 'a2dp_sink discover_peer_eps', 'Bitpool Range:')

        await send_cmd_to_iut(shell, dut, 'a2dp_sink configure', 'success to configure')
        for acp_id in source.streams:
            if acp_id == source_endpoint.seid:
                stream = source.streams[acp_id]
                break

        await send_cmd_to_iut(shell, dut, 'a2dp_sink establish', 'success to establish')
        assert stream.state == AVDTP_OPEN_STATE

        await send_cmd_to_iut(shell, dut, 'a2dp_sink start', 'success to start')
        assert stream.state == AVDTP_STREAMING_STATE

        await send_cmd_to_iut(shell, dut, 'a2dp_sink suspend', "success to suspend")
        assert stream.state == AVDTP_OPEN_STATE

        await send_cmd_to_iut(shell, dut, 'a2dp_sink release', "success to release")
        assert stream.state in (AVDTP_CLOSING_STATE, AVDTP_IDLE_STATE)

        await bumble_acl_disconnect(shell, dut, device, connection)


class TestA2dpSink:
    def test_a2dp_sink_case_register_ep(self, shell: Shell, dut: DeviceAdapter, a2dp_sink_dut):
        logger.info(f'test_a2dp_sink_case_register_ep {a2dp_sink_dut}')
        hci, iut_address = a2dp_sink_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(a2dp_sink_case_register_ep(hci, shell, dut, iut_address, snoop_file))

    def test_a2dp_sink_case_connect(self, shell: Shell, dut: DeviceAdapter, a2dp_sink_dut):
        logger.info(f'test_a2dp_sink_case_connect {a2dp_sink_dut}')
        hci, iut_address = a2dp_sink_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(a2dp_sink_case_connect(hci, shell, dut, iut_address, snoop_file))

    def test_a2dp_sink_case_disconnect(self, shell: Shell, dut: DeviceAdapter, a2dp_sink_dut):
        logger.info(f'test_a2dp_sink_case_disconnect {a2dp_sink_dut}')
        hci, iut_address = a2dp_sink_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(a2dp_sink_case_disconnect(hci, shell, dut, iut_address, snoop_file))

    def test_a2dp_sink_case_discover_get_cap(self, shell: Shell, dut: DeviceAdapter, a2dp_sink_dut):
        logger.info(f'test_a2dp_sink_case_discover_get_cap {a2dp_sink_dut}')
        hci, iut_address = a2dp_sink_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(a2dp_sink_case_discover_get_cap(hci, shell, dut, iut_address, snoop_file))

    def test_a2dp_sink_case_sink_to_source(self, shell: Shell, dut: DeviceAdapter, a2dp_sink_dut):
        logger.info(f'test_a2dp_sink_case_sink_to_source {a2dp_sink_dut}')
        hci, iut_address = a2dp_sink_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(a2dp_sink_case_sink_to_source(hci, shell, dut, iut_address, snoop_file))

    def test_a2dp_sink_case_source_to_sink(self, shell: Shell, dut: DeviceAdapter, a2dp_sink_dut):
        logger.info(f'test_a2dp_sink_case_source_to_sink {a2dp_sink_dut}')
        hci, iut_address = a2dp_sink_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(a2dp_sink_case_source_to_sink(hci, shell, dut, iut_address, snoop_file))
