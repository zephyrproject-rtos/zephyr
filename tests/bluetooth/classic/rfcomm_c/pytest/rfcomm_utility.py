# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import asyncio
import dataclasses
import enum
import logging

from bumble.core import InvalidStateError
from bumble.rfcomm import (
    DLC,
    RFCOMM_DEFAULT_INITIAL_CREDITS,
    RFCOMM_DEFAULT_MAX_FRAME_SIZE,
    RFCOMM_MCC_MSC,
    RFCOMM_MCC_PN,
    MccType,
    Multiplexer,
    RFCOMM_Frame,
)

mcc_test_data = bytes.fromhex('01 02 03 04 05')
send_value = bytes.fromhex('ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff')


class LogCaptureHandler(logging.Handler):
    def __init__(self):
        super().__init__()
        self.log_records = []

    def emit(self, record):
        self.log_records.append(record)

    def _get_logs(self):
        return [self.format(record) for record in self.log_records]

    def get_logs(self):
        logs = self._get_logs()
        self.clear_logs()
        return logs

    def clear_logs(self):
        self.log_records = []

    def check_contains(self, text):
        return any(text in self.format(record) for record in self.log_records)


def setup_logger_capture(_logger):
    _logger.setLevel(logging.INFO)
    _capture_handler = LogCaptureHandler()
    _logger.addHandler(_capture_handler)
    return _capture_handler


async def wait_mux_response(_logger_capture, response, max_wait_sec=10):
    found = False
    try:
        for _ in range(0, max_wait_sec):
            logs = _logger_capture.get_logs()
            for line in logs:
                if response in line:
                    found = True
                    break
            await asyncio.sleep(1)
    except Exception as e:
        raise e
    return found


logger = logging.getLogger(__name__)
logger_capture = setup_logger_capture(logger)

MccType.RLS = enum.IntEnum('MccType', {'RLS': 0x14}).RLS
MccType.RPN = enum.IntEnum('MccType', {'RPN': 0x24}).RPN
MccType.TEST = enum.IntEnum('MccType', {'TEST': 0x08}).TEST
MccType.BAD = enum.IntEnum('MccType', {'BAD': 0xFF}).BAD
MccType.NSC = enum.IntEnum('MccType', {'NSC': 0x04}).NSC
MccType.FCON = enum.IntEnum('MccType', {'FCON': 0x28}).FCON
MccType.FCOFF = enum.IntEnum('MccType', {'FCOFF': 0x18}).FCOFF


@dataclasses.dataclass
class RFCOMM_MCC_RLS:
    dlci: int
    error_flag: int
    break_flag: int

    @staticmethod
    def from_bytes(data: bytes) -> RFCOMM_MCC_RLS:
        pass

    def __bytes__(self) -> bytes:
        return bytes([(self.dlci << 2) | 3, (self.error_flag << 1) | self.break_flag])


@dataclasses.dataclass
class RFCOMM_MCC_RPN:
    dlci: int
    baud_rate: int = 0
    data_bits: int = 0
    stop_bits: int = 0
    parity: int = 0
    parity_type: int = 0
    flow_control: int = 0
    xon_char: int = 0
    xoff_char: int = 0
    parameter_mask: int = 0

    @staticmethod
    def from_bytes(data: bytes) -> RFCOMM_MCC_RPN:
        pass

    def __bytes__(self) -> bytes:
        return bytes(
            [
                (self.dlci << 2) | 3,
                self.baud_rate << 4 | self.data_bits,
                self.stop_bits << 4 | self.parity << 3 | self.parity_type,
                self.flow_control,
                self.xon_char,
                self.xoff_char,
                self.parameter_mask & 0xFF,
                (self.parameter_mask >> 8) & 0xFF,
            ]
        )


def send_rls_command(self, channel) -> None:
    logger.info('Send RLS command')
    rls = RFCOMM_MCC_RLS(dlci=channel << 1, error_flag=0, break_flag=0)
    mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.RLS, c_r=1, data=bytes(rls))
    RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc)
    self.send_frame(RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc))


def on_mcc_rls(self, c_r: bool, rls: RFCOMM_MCC_RLS) -> None:
    if c_r:
        # Command
        logger.info('Receive RLS command')
        response_rls = RFCOMM_MCC_RLS(dlci=rls.dlci, error_flag=0, break_flag=0)
        mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.RLS, c_r=0, data=bytes(response_rls))
        self.send_frame(RFCOMM_Frame.uih(c_r=0, dlci=0, information=mcc))
    else:
        # Response
        logger.info('Receive RLS response')


def send_rpn_command(self, channel: int, **parameters) -> None:
    logger.info('Send RPN command')
    rpn = RFCOMM_MCC_RPN(dlci=channel << 1, **parameters)
    mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.RPN, c_r=1, data=bytes(rpn))
    self.send_frame(RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc))


def on_mcc_rpn(self, c_r: bool, rpn: RFCOMM_MCC_RPN) -> None:
    if c_r:
        # Command
        logger.info('Receive RPN command')
        response_rpn = RFCOMM_MCC_RPN(
            dlci=rpn.dlci,
        )
        mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.RPN, c_r=0, data=bytes(response_rpn))
        self.send_frame(RFCOMM_Frame.uih(c_r=0, dlci=0, information=mcc))
    else:
        # Response
        logger.info('Receive RPN response')


def send_test_command(self, test_data: bytes) -> None:
    logger.info('Send TEST command')
    mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.TEST, c_r=1, data=test_data)
    self.send_frame(RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc))


def on_mcc_test(self, c_r: bool, test_data: bytes) -> None:
    if c_r:
        # Command
        mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.TEST, c_r=0, data=test_data)
        self.send_frame(RFCOMM_Frame.uih(c_r=0, dlci=0, information=mcc))
    else:
        # Response
        logger.info(f'Test Response received: {test_data}')
        if test_data == mcc_test_data:
            logger.info('MCC TEST pass')
        else:
            logger.info('MCC TEST fail')
        self.emit('test_response', test_data)


def send_fcon_command(self) -> None:
    logger.info('Send FCON command')
    mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.FCON, c_r=1, data=b'')
    frame = RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc)
    self.send_frame(frame)


def on_mcc_fcon(self, c_r: bool) -> None:
    if c_r:
        # Command
        logger.info('Received FCON command')
        mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.FCON, c_r=0, data=b'')
        self.send_frame(RFCOMM_Frame.uih(c_r=0, dlci=0, information=mcc))
    else:
        # Response
        logger.info('Received FCON response')


def send_fcoff_command(self) -> None:
    logger.info('Send FCOFF command')
    mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.FCOFF, c_r=1, data=b'')
    frame = RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc)
    self.send_frame(frame)


def on_mcc_fcoff(self, c_r: bool) -> None:
    if c_r:
        # Command
        logger.info('Received FCOFF command')
        mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.FCOFF, c_r=0, data=b'')
        self.send_frame(RFCOMM_Frame.uih(c_r=0, dlci=0, information=mcc))
    else:
        # Response
        logger.info('Received FCOFF response')


def on_mcc_nsc(self, c_r, nsc_data):
    if c_r:
        # Command
        nsc_data = bytes([nsc_data << 2 | 2])
        mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.NSC, c_r=0, data=nsc_data)
        frame = RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc)
        self.send_frame(frame)
    else:
        # Response
        logger.info(f'Receive NSC response: mcc_type: {hex(nsc_data)}')


def send_bad_command(self, channel) -> None:
    logger.info('Send BAD command')
    rls = RFCOMM_MCC_RLS(dlci=channel << 1, error_flag=0, break_flag=0)
    mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.BAD, c_r=1, data=bytes(rls))
    RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc)
    self.send_frame(RFCOMM_Frame.uih(c_r=1, dlci=0, information=mcc))


def on_uih_frame(self, frame: RFCOMM_Frame) -> None:
    (mcc_type, c_r, value) = RFCOMM_Frame.parse_mcc(frame.information)
    if mcc_type == MccType.PN:
        logger.info('Receive PN')
        pn = RFCOMM_MCC_PN.from_bytes(value)
        self.on_mcc_pn(c_r, pn)
    elif mcc_type == MccType.MSC:
        logger.info('Receive MSC')
        mcs = RFCOMM_MCC_MSC.from_bytes(value)
        self.on_mcc_msc(c_r, mcs)
    elif mcc_type == MccType.RLS:
        logger.info('Receive RLS')
        mcs = RFCOMM_MCC_RLS.from_bytes(value)
        self.on_mcc_rls(c_r, mcs)
    elif mcc_type == MccType.RPN:
        logger.info('Receive RPN')
        mcs = RFCOMM_MCC_RPN.from_bytes(value)
        self.on_mcc_rpn(c_r, mcs)
    elif mcc_type == MccType.TEST:
        logger.info('Receive TEST')
        self.on_mcc_test(c_r, value)
    elif mcc_type == MccType.FCON:
        logger.info('Receive FCON')
        self.on_mcc_fcon(c_r)
    elif mcc_type == MccType.FCOFF:
        logger.info('Receive FCOFF')
        self.on_mcc_fcoff(c_r)
    else:
        logger.info('Receive NSC')
        self.on_mcc_nsc(c_r, mcc_type)


def on_disc_frame_mux(self, _frame: RFCOMM_Frame) -> None:
    logger.info('DISC received on dlc=0')
    self.change_state(Multiplexer.State.DISCONNECTED)
    self.send_frame(
        RFCOMM_Frame.ua(c_r=0 if self.role == Multiplexer.Role.INITIATOR else 1, dlci=0)
    )


def on_disc_frame_dlc(self, _frame: RFCOMM_Frame) -> None:
    logger.info(f'DISC received on dlc={self.dlci}')
    self.send_frame(RFCOMM_Frame.ua(c_r=1 - self.c_r, dlci=self.dlci))


async def open_dlc(
    self,
    channel: int,
    max_frame_size: int = RFCOMM_DEFAULT_MAX_FRAME_SIZE,
    initial_credits: int = RFCOMM_DEFAULT_INITIAL_CREDITS,
    send_PN=True,
    support_CFC=True,
) -> DLC:
    if self.state != Multiplexer.State.CONNECTED:
        if self.state == Multiplexer.State.OPENING:
            raise InvalidStateError('open already in progress')

        raise InvalidStateError('not connected')

    if support_CFC:
        cl = 0xF0
    else:
        cl = 0x00
    self.open_pn = RFCOMM_MCC_PN(
        dlci=channel << 1,
        cl=cl,
        priority=7,
        ack_timer=0,
        max_frame_size=max_frame_size,
        max_retransmissions=0,
        initial_credits=initial_credits,
    )

    mcc = RFCOMM_Frame.make_mcc(mcc_type=MccType.PN, c_r=1, data=bytes(self.open_pn))
    self.open_result = asyncio.get_running_loop().create_future()
    self.change_state(Multiplexer.State.OPENING)
    if send_PN:
        logger.debug(f'>>> Sending MCC: {self.open_pn}')
        self.send_frame(
            RFCOMM_Frame.uih(
                c_r=1 if self.role == Multiplexer.Role.INITIATOR else 0,
                dlci=0,
                information=mcc,
            )
        )
        return await self.open_result
    else:
        if self.state == Multiplexer.State.OPENING:
            assert self.open_pn
            dlc = DLC(
                self,
                dlci=channel << 1,
                tx_max_frame_size=self.open_pn.max_frame_size,
                tx_initial_credits=self.open_pn.initial_credits,
                rx_max_frame_size=self.open_pn.max_frame_size,
                rx_initial_credits=self.open_pn.initial_credits,
            )
            self.dlcs[channel << 1] = dlc
            self.open_pn = None
            dlc.connect()
            return await self.open_result


Multiplexer.send_rls_command = send_rls_command
Multiplexer.on_mcc_rls = on_mcc_rls

Multiplexer.send_rpn_command = send_rpn_command
Multiplexer.on_mcc_rpn = on_mcc_rpn

Multiplexer.send_test_command = send_test_command
Multiplexer.on_mcc_test = on_mcc_test

Multiplexer.send_fcon_command = send_fcon_command
Multiplexer.on_mcc_fcon = on_mcc_fcon

Multiplexer.send_fcoff_command = send_fcoff_command
Multiplexer.on_mcc_fcoff = on_mcc_fcoff

Multiplexer.on_mcc_nsc = on_mcc_nsc

Multiplexer.send_bad_command = send_bad_command

Multiplexer.on_uih_frame = on_uih_frame
Multiplexer.on_disc_frame = on_disc_frame_mux

Multiplexer.open_dlc = open_dlc

DLC.on_disc_frame = on_disc_frame_dlc
