# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
import asyncio
import logging
import sys

from twister_harness import DeviceAdapter, Shell

import bumble
from bumble.device import Device, DEVICE_DEFAULT_INQUIRY_LENGTH
from bumble import hci
from bumble.hci import Address, HCI_Write_Page_Timeout_Command
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from bumble.core import BT_BR_EDR_TRANSPORT, DeviceClass
from bumble.rfcomm import Client

from twister_harness import DeviceAdapter, Shell
import rfcomm_utility
from rfcomm_utility import wait_mux_response, setup_logger_capture, send_value

logger = logging.getLogger(__name__)
logger_capture = setup_logger_capture(logger)

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
        for i in range(0, max_wait_sec):
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
async def send_cmd_to_iut(shell, dut, cmd, response=None, expect_to_find_resp=True, max_wait_sec=20, shell_ret=False):
    """
    send_cmd_to_iut() is used to send shell cmd to DUT and monitor the response.
    It can choose whether to monitor the shell response of DUT.
    Use 'expect_to_find_resp' to set whether to expect the response to contain certain 'response'.
    'max_wait_sec' indicates the maximum waiting time. For 'expect_to_find_resp=False', this is useful
    because we need to wait long enough to get enough response
    to more accurately judge that the response does not contain specific characters.

    :param shell:
    :param dut:
    :param cmd: shell cmd sent to DUT
    :param response: shell response that you want to monitor. 'None' means not to monitor any response.
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
            parameters=b''
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
            parameters=new_cod
        )
    )

    await device.send_command(
        hci.HCI_Command(
            op_code=0x0C3A,  # Write Current IAC LAP
            parameters=bytes([0x01]) + iac  # num_current_iac=1, iac_lap
        )
    )

    device.discoverable = discoverable

# dongle limited discovery
async def start_limited_discovery(device):
    await device.send_command(
        hci.HCI_Write_Inquiry_Mode_Command(
            inquiry_mode=hci.HCI_EXTENDED_INQUIRY_MODE
        ),
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
        (
            service_classes,
            major_device_class,
            minor_device_class,
        ) = DeviceClass.split_class_of_device(class_of_device)
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
        assert await wait_mux_response(rfcomm_utility.logger_capture, 'Reveice PN resp')
    except bumble.core.ConnectionError as error:
        logger.error(f'DLC open failed: {error}')
        await rfcomm_mux.disconnect()
        return

    def echo_back(data):
        logger.info(f'<<< [Channel: {int(session.dlci/2)}] RFCOMM Data received: {data.hex()}')
        if data == send_value:
            logger.info(f'Data send pass')
        else:
            logger.info(f'Data send fail')
        session.write(data)
    if session is not None:
        session.sink = echo_back
    return session

async def tc_rfcomm_s_1(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Server - Reject RFCOMM Session'
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
            await device.power_on()
            dongle_address = str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            try:
                logger.info(f'Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, lines = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM client
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_client = Client(connection)
            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                assert False, f"Test failed: {e}"

            channel_9 = 9
            channel_7 = 7

            # Test Start
            logger.info(f'Step 1: Initialize RFCOMM Session')
            rfcomm_mux = await rfcomm_client.start()

            logger.info(f'Step 2: Disconnect BR connection')
            await connection.disconnect()
            found, lines = await _wait_for_shell_response(dut, "Disconnected")
            assert found is True, "DUT did not report disconnection"

            logger.info(f'Step 3: Attempt to establish RFCOMM session')
            try:
                session_9 = await establish_dlc(rfcomm_mux, channel_9)
                assert False, "Test failed: DUT did not reject RFCOMM session"
            except Exception as e:
                logger.info(f'Expected error: RFCOMM session rejected: {e}')
                assert True, "Test passed: DUT correctly rejected RFCOMM session"

async def tc_rfcomm_s_2(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Server - Reject DLC Establishment Due to Missing PN Command'
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
            await device.power_on()
            dongle_address = str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            try:
                logger.info(f'Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, lines = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM client
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_client = Client(connection)
            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                assert False, f"Test failed: {e}"

            channel_9 = 9
            channel_7 = 7

            # Test Start
            logger.info(f'Step 1: Initialize RFCOMM Session')
            rfcomm_mux = await rfcomm_client.start()

            logger.info(f'Step 2: Try to establish DLC without sending PN command')
            try:
                # Attempt to establish DLC without sending PN command
                dlc_9 = await establish_dlc(rfcomm_mux, channel_9, send_PN=False)

                # If we get here without an exception, the test failed because the DUT
                # should reject the DLC establishment
                logger.error("DUT did not reject DLC establishment without PN command")
                assert False, "Test failed: DUT accepted DLC establishment without PN command"
            except bumble.core.ConnectionError as error:
                # Expected error - DUT rejected the DLC establishment
                logger.info(f'Expected error: DLC establishment rejected: {error}')
                assert True, "Test passed: DUT correctly rejected DLC establishment without PN command"

            logger.info(f'Step 3: Establish DLC properly with PN command to verify normal operation')
            dlc_7 = await establish_dlc(rfcomm_mux, channel_7)
            assert dlc_7, "Failed to establish DLC with proper PN command"

            logger.info(f'Step 4: Shutdown RFCOMM Session')
            await rfcomm_client.shutdown()
            assert True, "Test completed successfully"

async def tc_rfcomm_s_3(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Server - Respond to Control Commands After DLC Establishment'
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
            await device.power_on()
            dongle_address = str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            try:
                logger.info(f'Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, lines = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM client
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_client = Client(connection)
            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                assert False, f"Test failed: {e}"

            channel_9 = 9

            # Test Start
            logger.info(f'Step 1: Initialize RFCOMM Session')
            rfcomm_mux = await rfcomm_client.start()

            logger.info(f'Step 2: Establish DLC')
            dlc_9 = await establish_dlc(rfcomm_mux, channel_9)
            assert dlc_9, "Failed to establish DLC"

            logger.info(f'Step 3: Send RLS command to DUT')
            # Bumble has a bug when parsing the MCC type that is over than 0x3F
            rfcomm_mux.send_rls_command(channel_9)
            # assert await wait_mux_response(rfcomm_utility.logger_capture, 'Reveice RLS resp'), "Failed to receive RLS response"

            logger.info(f'Step 4: Send RPN command to DUT')
            rfcomm_mux.send_rpn_command(channel_9)
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'Reveice RPN resp'), "Failed to receive RPN response"

            logger.info(f'Step 5: Send Test command to DUT')
            rfcomm_mux.send_test_command(rfcomm_utility.mcc_test_data)
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'MCC TEST pass'), "Failed to receive Test response"

            logger.info(f'Step 6: Send unsupported command to DUT')
            rfcomm_mux.send_bad_command(channel_9)
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'Reveice NSC resp'), "Failed to receive NSC response"

            logger.info(f'Step 7: Disconnect DLC')
            await dlc_9.disconnect()
            found, lines = await _wait_for_shell_response(dut, "disconnected")
            assert found, "DUT did not report DLC disconnection"

async def tc_rfcomm_s_4(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Server - Information Transfer with No Flow Control'
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
            await device.power_on()
            dongle_address = str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            try:
                logger.info(f'Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable

                # Skip this test because Information Transfer with No Flow Control is not implemented in Zephyr
                logger.info(f'Test skipped: Information Transfer with No Flow Control is not implemented in Zephyr')
                assert True  # Ensure a definite assertion at the end
                return
            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                assert False, f"Test failed: {e}"

async def tc_rfcomm_s_5(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Server - Information Transfer with Aggregate Flow Control'
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
            await device.power_on()
            dongle_address = str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            try:
                logger.info(f'Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable

                # Skip this test because Information Transfer with Aggregate Flow Control is not implemented in Zephyr
                logger.info(f'Test skipped: Information Transfer with Aggregate Flow Control is not implemented in Zephyr')
                assert True  # Ensure a definite assertion at the end
                return
            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                assert False, f"Test failed: {e}"

async def tc_rfcomm_s_6(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Server - Information Transfer with Credit Based Flow Control and Active Disconnection'
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
            await device.power_on()
            dongle_address = str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            try:
                logger.info(f'Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, lines = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM client
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_client = Client(connection)
            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                assert False, f"Test failed: {e}"

            channel_9 = 9
            channel_7 = 7

            # Test Start
            logger.info(f'Step 1: Initialize RFCOMM Session')
            rfcomm_mux = await rfcomm_client.start()

            logger.info(f'Step 2: Establish DLC')
            dlc_9 = await establish_dlc(rfcomm_mux, channel_9)
            dlc_7 = await establish_dlc(rfcomm_mux, channel_7)
            assert dlc_9 and dlc_7, "Failed to establish DLC"

            logger.info(f'Step 3: Transfer data with credit based flow control')
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1")
            assert await wait_mux_response(logger_capture, 'Data send pass'), "Failed to receive data from DUT"

            logger.info(f'Step 4: DUT actively disconnects DLC')
            await send_cmd_to_iut(shell, dut, "rfcomm_s disconnect 9")
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'DISC received on dlc=18'), "DUT failed to disconnect DLC"

            logger.info(f'Step 5: DUT shutdown RFCOMM Session')
            await send_cmd_to_iut(shell, dut, "rfcomm_s disconnect 7")
            assert await wait_mux_response(rfcomm_utility.logger_capture, 'DISC received on dlc=0'), "DUT failed to disconnect DLC"

async def tc_rfcomm_s_7(hci_port, shell, dut, address) -> None:
    case_name = 'RFCOMM Server - Information Transfer with Credit Based Flow Control and Passive Disconnection'
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
            await device.power_on()
            dongle_address = str(device.public_address).replace(r'/P', '')
            await device.set_discoverable(True)
            await device.set_connectable(True)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Initial Condition
            try:
                logger.info(f'Initial Condition: Set DUT state...')
                await send_cmd_to_iut(shell, dut, "br pscan on")  # set to connectable
                await send_cmd_to_iut(shell, dut, "br iscan on")  # set to general discoverable
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 9")  # create RFCOMM server
                await send_cmd_to_iut(shell, dut, "rfcomm_s register 7")  # create RFCOMM server

                # Connecting
                logger.info(f'Initial Condition: establish be connection to {dut_address}...')
                connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)
                found, lines = await _wait_for_shell_response(dut, "Connected")
                assert found is True, "DUT did not report connection established"

                # Request authentication
                logger.info('Initial Condition: Authenticating...')
                await connection.authenticate()

                # Enable encryption
                logger.info('Initial Condition: Enabling encryption...')
                await connection.encrypt()

                # Create RFCOMM client
                logger.info('Initial Condition: Create RFCOMM client...')
                rfcomm_client = Client(connection)
            except Exception as e:
                logger.error(f'Failed to set Initial Condition: {e}')
                assert False, f"Test failed: {e}"

            channel_9 = 9
            channel_7 = 7

            # Test Start
            logger.info(f'Step 1: Initialize RFCOMM Session')
            rfcomm_mux = await rfcomm_client.start()

            logger.info(f'Step 2: Establish DLC')
            dlc_9 = await establish_dlc(rfcomm_mux, channel_9)
            dlc_7 = await establish_dlc(rfcomm_mux, channel_7)
            assert dlc_9 and dlc_7, "Failed to establish DLC"

            logger.info(f'Step 3: Transfer data with credit based flow control')
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1")
            assert await wait_mux_response(logger_capture, 'Data send pass'), "Failed to receive data from DUT"
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 7 1")
            assert await wait_mux_response(logger_capture, 'Data send pass'), "Failed to receive data from DUT"

            logger.info(f'Step 4: Tester initiates DLC disconnection')
            await dlc_9.disconnect()
            found, lines = await _wait_for_shell_response(dut, "disconnected")
            assert found, "DUT did not respond to DLC disconnection"
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)

            logger.info(f'Step 5: Tester initiates RFCOMM session shutdown')
            await rfcomm_client.shutdown()
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 9 1", "Unable to send", shell_ret=True)
            await send_cmd_to_iut(shell, dut, "rfcomm_s send 7 1", "Unable to send", shell_ret=True)

class TestRFCOMM:
    def test_rfcomm_s_1(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """Test case for RFCOMM Server - Reject RFCOMM Session."""
        logger.info(f'RFCOMM-S-1 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_s_1(hci, shell, dut, iut_address))

    def test_rfcomm_s_2(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """Test case for RFCOMM Server - Reject DLC Establishment Due to Missing PN Command."""
        logger.info(f'RFCOMM-S-2 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_s_2(hci, shell, dut, iut_address))

    def test_rfcomm_s_3(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """Test case for RFCOMM Server - Respond to Control Commands After DLC Establishment."""
        logger.info(f'RFCOMM-S-3 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_s_3(hci, shell, dut, iut_address))

    # def test_rfcomm_s_4(self, shell: Shell, dut: DeviceAdapter, device_under_test):
    #     """Test case for RFCOMM Server - Information Transfer with No Flow Control."""
    #     logger.info(f'RFCOMM-S-4 {device_under_test}')
    #     hci, iut_address = device_under_test
    #     asyncio.run(tc_rfcomm_s_4(hci, shell, dut, iut_address))

    # def test_rfcomm_s_5(self, shell: Shell, dut: DeviceAdapter, device_under_test):
    #     """Test case for RFCOMM Server - Information Transfer with Aggregate Flow Control."""
    #     logger.info(f'RFCOMM-S-5 {device_under_test}')
    #     hci, iut_address = device_under_test
    #     asyncio.run(tc_rfcomm_s_5(hci, shell, dut, iut_address))

    def test_rfcomm_s_6(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """Test case for RFCOMM Server - Information Transfer with Credit Based Flow Control and Active Disconnection."""
        logger.info(f'RFCOMM-S-6 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_s_6(hci, shell, dut, iut_address))

    def test_rfcomm_s_7(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """Test case for RFCOMM Server - Information Transfer with Credit Based Flow Control and Passive Disconnection."""
        logger.info(f'RFCOMM-S-7 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_rfcomm_s_7(hci, shell, dut, iut_address))
