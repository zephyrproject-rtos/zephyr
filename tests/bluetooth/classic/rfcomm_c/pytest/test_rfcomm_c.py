# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import sys

import bumble
import rfcomm_utility
from bumble import hci
from bumble.core import BT_BR_EDR_TRANSPORT, DeviceClass, InvalidStateError
from bumble.device import DEVICE_DEFAULT_INQUIRY_LENGTH, Device
from bumble.hci import Address, HCI_Write_Page_Timeout_Command
from bumble.rfcomm import DLC, RFCOMM_MCC_PN, MccType, RFCOMM_Frame, Server
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from rfcomm_utility import send_value, setup_logger_capture, wait_mux_response
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)
logger_capture = setup_logger_capture(logger)


dlc_9 = None
dlc_7 = None
rfcomm_mux = None


# power on dongle
async def device_power_on(device) -> None:
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue


# wait for shell response
async def _wait_for_shell_response(dut, response, max_wait_sec=10):
    """
    _wait_for_shell_response() is used to wait for shell response.
    It will return after finding a specific 'response' or waiting long enough.
    :param dut:
    :param response: shell response that you want to monitor.
    :param max_wait_sec: maximum waiting time
    :return: found: whether the 'response' is found; lines: DUT shell response
    """
    found = False
    lines = []
    try:
        for _ in range(0, max_wait_sec):
            read_lines = dut.readlines()
            for line in read_lines:
                if response in line:
                    found = True
                    break
            lines = lines + read_lines
            await asyncio.sleep(1)
        logger.info(f'{str(lines)}')
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e
    return found, lines


# interact between script and DUT
async def send_cmd_to_iut(
    shell, dut, cmd, response=None, expect_to_find_resp=True, max_wait_sec=20, shell_ret=False
):
    """
    send_cmd_to_iut() is used to send shell cmd to DUT and monitor the response.
    It can choose whether to monitor the shell response of DUT.
    Use 'expect_to_find_resp' to set whether to expect the response to contain certain 'response'.
    'max_wait_sec' indicates the maximum waiting time.
    For 'expect_to_find_resp=False', this is useful
    because we need to wait long enough to get enough response
    to more accurately judge that the response does not contain specific characters.

    :param shell:
    :param dut:
    :param cmd: shell cmd sent to DUT
    :param response: shell response that you want to monitor.
                     'None' means not to monitor any response.
    :param expect_to_find_resp: set whether to expect the response to contain certain 'response'
    :param max_wait_sec: maximum monitoring time
    :param shell_ret: flag to indicate whether to check shell command return directly
    :return: DUT shell response
    """
    found = False
    lines = ''
    shell_cmd_ret = shell.exec_command(cmd)
    if response is not None:
        if shell_ret:
            shell_cmd_ret_str = str(shell_cmd_ret)
            if response in shell_cmd_ret_str:
                found = True
                lines = shell_cmd_ret_str
        else:
            found, lines = await _wait_for_shell_response(dut, response, max_wait_sec)
    else:
        found = True
    assert found is expect_to_find_resp
    return lines


# set limited discoverab mode of dongle
async def set_limited_discoverable(device, discoverable=True):
    # Read current class of device
    response = await device.send_command(
        hci.HCI_Command(
            op_code=0x0C23,  # Read Class of Device
            parameters=b'',
        )
    )
    current_cod = response.return_parameters.class_of_device

    if discoverable:
        # set Limited Discoverable Mode (bit 13)
        new_cod = (current_cod | 0x2000).to_bytes(3, byteorder='little')
        # Limited Inquiry Access Code(LIAC) = 0x9E8B00
        iac = hci.HCI_LIMITED_DEDICATED_INQUIRY_LAP.to_bytes(3, byteorder='little')
    else:
        mask = ~0x2000
        new_cod = (current_cod & mask).to_bytes(3, byteorder='little')
        # General Inquiry Access Code(GIAC) = 0x9E8B33
        iac = hci.HCI_GENERAL_INQUIRY_LAP.to_bytes(3, byteorder='little')

    await device.send_command(
        hci.HCI_Command(
            op_code=0x0C24,  # Write Class of Device
            parameters=new_cod,
        )
    )

    await device.send_command(
        hci.HCI_Command(
            op_code=0x0C3A,  # Write Current IAC LAP
            parameters=bytes([0x01]) + iac,  # num_current_iac=1, iac_lap
        )
    )

    device.discoverable = discoverable


# dongle limited discovery
async def start_limited_discovery(device):
    await device.send_command(
        hci.HCI_Write_Inquiry_Mode_Command(inquiry_mode=hci.HCI_EXTENDED_INQUIRY_MODE),
        check_result=True,
    )

    response = await device.send_command(
        hci.HCI_Inquiry_Command(
            lap=hci.HCI_LIMITED_DEDICATED_INQUIRY_LAP,
            inquiry_length=DEVICE_DEFAULT_INQUIRY_LENGTH,
            num_responses=0,  # Unlimited number of responses.
        )
    )
    if response.status != hci.HCI_Command_Status_Event.PENDING:
        device.discovering = False
        raise hci.HCI_StatusError(response)

    device.auto_restart_inquiry = False
    device.discovering = True


# device listener for receiving scan results
class DiscoveryListener(Device.Listener):
    def __init__(self):
        self.discovered_addresses = set()

    def on_inquiry_result(self, address, class_of_device, data, rssi):
        DeviceClass.split_class_of_device(class_of_device)
        found_address = str(address).replace(r'/P', '')
        logger.info(f'Found addr: {found_address}')
        self.discovered_addresses.add(found_address)

    def has_found_target_addr(self, target_addr):
        return str(target_addr).upper() in self.discovered_addresses


# establish_dlc
async def establish_dlc(rfcomm_mux, channel, send_PN=True):
    logger.info(f'Establish DLC for channel {channel}...')
    try:
        session = await rfcomm_mux.open_dlc(channel, send_PN=send_PN)
        # assert await wait_mux_response(rfcomm_utility.logger_capture, 'Receive PN')
    except bumble.core.ConnectionError as error:
        logger.error(f'DLC open failed: {error}')
        await rfcomm_mux.disconnect()
        raise error

    # dlc_echo_back
    def dlc_echo_back(data):
        logger.info(f'<<< [Channel: {int(session.dlci / 2)}] RFCOMM Data received: {data.hex()}')
        if data == send_value:
            logger.info('Data send pass')
        else:
            logger.info('Data send fail')
        session.write(data)

    if session is not None:
        session.sink = dlc_echo_back
    return session


# for dlc establishment response
def on_rfcomm_dlc(session):
    global dlc_9, dlc_7
    if session is None:
        return
    if int(session.dlci / 2) == 9:
        dlc_9 = session
    elif int(session.dlci / 2) == 7:
        dlc_7 = session
    else:
        logger.error('channel should be either 9 or 7')
        return
    logger.info(f'<<< [Channel: {int(session.dlci / 2)}] created')

    # dlc_echo_back
    def dlc_echo_back(data):
        logger.info(f'<<< [Channel: {int(session.dlci / 2)}] RFCOMM Data received: {data.hex()}')
        if data == send_value:
            logger.info('Data send pass')
        else:
            logger.info('Data send fail')
        session.write(data)

    session.sink = dlc_echo_back


# for multiplexer establishment response
def on_multiplexer_start(multiplexer):
    global rfcomm_mux
    rfcomm_mux = multiplexer
    logger.info("New multiplexer started")


async def tc_rfcomm_c_1(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Client Command Transfer'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # Init Dongle State
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            channel_9 = 9
            channel_7 = 7
            try:
                logger.info('Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, _ = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM server
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_server = Server(device)
                rfcomm_server.on('start', on_multiplexer_start)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_9)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_7)

            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                AssertionError()

            # Test Start
            logger.info('Step 1: Establish DLC on channel 9 and 7')
            await send_cmd_to_iut(shell, dut, "rfcomm_s connect 9", "connected")
            await send_cmd_to_iut(shell, dut, "rfcomm_s connect 7", "connected")

            logger.info(
                'Step 2: Response to RLS Command, RPN Command, Test Command, and NSC response'
            )
            rfcomm_mux.send_rls_command(channel_9)
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'Receive RLS response'), (
                "Failed to receive RLS response"
            )

            rfcomm_mux.send_rpn_command(channel_9)
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'Receive RPN response'), (
                "Failed to receive RPN response"
            )

            rfcomm_mux.send_test_command(rfcomm_utility.mcc_test_data)
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'MCC TEST pass'), (
                "Failed to receive Test response"
            )

            rfcomm_mux.send_bad_command(channel_9)
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'Receive NSC response'), (
                "Failed to receive NSC response"
            )

            logger.info('Step 3: Disconnect DLC')
            await dlc_9.disconnect()
            found, _ = await _wait_for_shell_response(dut, "disconnected")
            assert found, "DUT did not report DLC disconnection"
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 7 1")
            assert await wait_mux_response(logger_capture, 'Data send pass'), (
                "Failed to receive data from DUT"
            )

            logger.info('Step 4: Disconnect session')
            await rfcomm_mux.disconnect()
            found, _ = await _wait_for_shell_response(dut, "disconnected")
            assert found, "DUT did not report DLC disconnection"
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 7 1", "Unable to send", shell_ret=True)


async def tc_rfcomm_c_2(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Client with Credit Based Flow Control'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # Init Dongle State
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            channel_9 = 9
            channel_7 = 7
            try:
                logger.info('Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, _ = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM server
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_server = Server(device)
                rfcomm_server.on('start', on_multiplexer_start)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_9)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_7)

            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                AssertionError()

            # Test Start
            logger.info('Step 1: Establish DLC on channel 9 and 7')
            await send_cmd_to_iut(shell, dut, "rfcomm_s connect 9", "connected")
            await send_cmd_to_iut(shell, dut, "rfcomm_s connect 7", "connected")

            logger.info('Step 2: Perform information transfer using credit based flow control')
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1")
            assert await wait_mux_response(logger_capture, 'Data send pass'), (
                "Failed to receive data on channel 9 from DUT"
            )

            await send_cmd_to_iut(shell, dut, "rfcomm_s send 7 1")
            assert await wait_mux_response(logger_capture, 'Data send pass'), (
                "Failed to receive data on channel 7 from DUT"
            )

            # Sending data from dongle to DUT
            dlc_9.write(send_value)
            dlc_7.write(send_value)
            found, _ = await _wait_for_shell_response(dut, "Incoming data")
            assert found, "DUT did not report receiving data from dongle"

            logger.info('Step 3: Initiate disconnection of the DLCs')
            await send_cmd_to_iut(shell, dut, "rfcomm_s disconnect 9", "disconnect")
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'DISC received on dlc=18')

            logger.info('Step 4: Shutdown the RFCOMM session')
            await send_cmd_to_iut(shell, dut, "rfcomm_s disconnect 7", "disconnect")
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'DISC received on dlc=0')

            # Verify resources are released
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 7 1", "Unable to send", shell_ret=True)


async def tc_rfcomm_c_3(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Client with BR Connection Disconnection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # Init Dongle State
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            channel_9 = 9
            channel_7 = 7
            try:
                logger.info('Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, _ = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM server
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_server = Server(device)
                rfcomm_server.on('start', on_multiplexer_start)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_9)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_7)

            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                AssertionError()

            # Test Start
            logger.info('Step 1: BR connection disconnection')
            # Disconnect BR connection
            await connection.disconnect()

            logger.info('Step 2: Start DLC establishment process')
            await send_cmd_to_iut(shell, dut, "rfcomm_s connect 9", "Not connected", shell_ret=True)

            logger.info('Step 3: Verify DLC establishment is rejected')

            # Try to send data to verify resources are released properly
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)


async def tc_rfcomm_c_4(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Client with Aggregate Flow Control'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # Init Dongle State
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            channel_9 = 9
            channel_7 = 7
            try:
                logger.info('Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, _ = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM server
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_server = Server(device)
                rfcomm_server.on('start', on_multiplexer_start)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_9)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_7)

            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                AssertionError()

            # Test Start
            logger.info(
                'Step 1: Establish DLC on channel 9 and 7 with aggregate flow control enabled'
            )

            def accept(self) -> None:
                # Simulate an rfcomm server device that does not support CFC
                if self.state != DLC.State.INIT:
                    raise InvalidStateError('invalid state')

                pn = RFCOMM_MCC_PN(
                    dlci=self.dlci,
                    cl=0x00,
                    priority=7,
                    ack_timer=0,
                    max_frame_size=self.rx_max_frame_size,
                    max_retransmissions=0,
                    initial_credits=self.rx_initial_credits,
                )
                mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.PN, c_r=0, data=bytes(pn))
                logger.debug(f'>>> PN Response: {pn}')
                self.send_frame(RFCOMM_Frame.uih(c_r=self.c_r, dlci=0, information=mcc))
                self.change_state(DLC.State.CONNECTING)

            DLC.accept = accept
            await send_cmd_to_iut(shell, dut, "rfcomm_s connect 9", "connected")
            await send_cmd_to_iut(shell, dut, "rfcomm_s connect 7", "connected")

            logger.info('Step 2: DUT transfer data after received FCOFF command from Tester')
            rfcomm_mux.send_fcoff_command()
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'Received FCOFF response')

            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1")
            assert not await wait_mux_response(logger_capture, 'Data send pass', max_wait_sec=5), (
                "DUT should not send data"
            )

            logger.info('Step 3: The blocked data can be sent after FCON command')
            rfcomm_mux.send_fcon_command()
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'Received FCON response')
            assert await wait_mux_response(logger_capture, 'Data send pass')

            logger.info('Step 4: Initiate disconnection of the DLCs')
            await send_cmd_to_iut(shell, dut, "rfcomm_s disconnect 9", "disconnect")
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'DISC received on dlc=18')

            logger.info('Step 5: Shutdown the RFCOMM session')
            await send_cmd_to_iut(shell, dut, "rfcomm_s disconnect 7", "disconnect")
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'DISC received on dlc=0')

            # Verify resources are released
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 7 1", "Unable to send", shell_ret=True)


async def tc_rfcomm_c_5(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Client with Failed PN Response'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # Init Dongle State
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            channel_9 = 9
            channel_7 = 7
            try:
                logger.info('Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, _ = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM server
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_server = Server(device)
                rfcomm_server.on('start', on_multiplexer_start)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_9)
                rfcomm_server.listen(on_rfcomm_dlc, channel=channel_7)

            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                AssertionError()

            # Test Start
            def accept(self) -> None:
                if self.state != DLC.State.INIT:
                    raise InvalidStateError('invalid state')
                logger.info('Ignore PN cmd')
                self.change_state(DLC.State.CONNECTING)

            DLC.accept = accept

            logger.info('Step 1: Start a DLC from the client device')
            await send_cmd_to_iut(
                shell, dut, "rfcomm_s connect 9", " connected", expect_to_find_resp=False
            )

            logger.info('Step 2: Simulate a failure to receive PN response from the remote device')

            logger.info('Step 3: Verify DLC establishment is rejectede')
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)


class TestRFCOMM:
    def test_rfcomm_c_1(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """RFCOMM Client Command Transfer."""
        logger.info(f'RFCOMM-C-1 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_c_1(hci, shell, dut, iut_address))

    def test_rfcomm_c_2(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """RFCOMM Client with Credit Based Flow Control."""
        logger.info(f'RFCOMM-C-2 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_c_2(hci, shell, dut, iut_address))

    def test_rfcomm_c_3(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """RFCOMM Client with BR Connection Disconnection."""
        logger.info(f'RFCOMM-C-3 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_c_3(hci, shell, dut, iut_address))

    def test_rfcomm_c_4(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """RFCOMM Client with Aggregate Flow Control."""
        logger.info(f'RFCOMM-C-4 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_c_4(hci, shell, dut, iut_address))

    def test_rfcomm_c_5(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """RFCOMM Client with Failed PN Response."""
        logger.info(f'RFCOMM-C-5 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_c_5(hci, shell, dut, iut_address))
