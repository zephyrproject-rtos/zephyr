# Copyright (c) 2024 Vestas Wind Systems A/S
#
# SPDX-License-Identifier: Apache-2.0

"""
Zephyr CAN shell module support for providing a python-can bus interface for testing.
"""

import re
import logging
from typing import Optional, Tuple

from can import BusABC, CanProtocol, Message
from can.exceptions import CanInitializationError, CanOperationError
from can.typechecking import CanFilters

from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

class CanShellBus(BusABC): # pylint: disable=abstract-method
    """
    A CAN interface using the Zephyr CAN shell module.
    """

    def __init__(self, dut: DeviceAdapter, shell: Shell, channel: str,
                 can_filters: Optional[CanFilters] = None, **kwargs) -> None:
        self._dut = dut
        self._shell = shell
        self._device = channel
        self._is_filtered = False
        self._filter_ids = []

        self.channel_info = f'Zephyr CAN shell, device "{self._device}"'

        mode = 'normal'
        if 'fd' in self._get_capabilities():
            self._can_protocol = CanProtocol.CAN_FD
            mode += ' fd'
        else:
            self._can_protocol = CanProtocol.CAN_20

        self._set_mode(mode)
        self._start()

        super().__init__(channel=channel, can_filters=can_filters, **kwargs)

    def _retval(self):
        """Get return value of last shell command."""
        return int(self._shell.get_filtered_output(self._shell.exec_command('retval'))[0])

    def _get_capabilities(self) -> list[str]:
        cmd = f'can show {self._device}'

        lines = self._shell.get_filtered_output(self._shell.exec_command(cmd))
        regex_compiled = re.compile(r'capabilities:\s+(?P<caps>.*)')
        for line in lines:
            m = regex_compiled.match(line)
            if m:
                return m.group('caps').split()

        raise CanOperationError('capabilities not found')

    def _set_mode(self, mode: str) -> None:
        self._shell.exec_command(f'can mode {self._device} {mode}')
        retval = self._retval()
        if retval != 0:
            raise CanOperationError(f'failed to set mode "{mode}" (err {retval})')

    def _start(self):
        self._shell.exec_command(f'can start {self._device}')
        retval = self._retval()
        if retval != 0:
            raise CanInitializationError(f'failed to start (err {retval})')

    def _stop(self):
        self._shell.exec_command(f'can stop {self._device}')

    def send(self, msg: Message, timeout: Optional[float] = None) -> None:
        logger.debug('sending: %s', msg)

        cmd = f'can send {self._device}'
        cmd += ' -e' if msg.is_extended_id else ''
        cmd += ' -r' if msg.is_remote_frame else ''
        cmd += ' -f' if msg.is_fd else ''
        cmd += ' -b' if msg.bitrate_switch else ''

        if msg.is_extended_id:
            cmd += f' {msg.arbitration_id:08x}'
        else:
            cmd += f' {msg.arbitration_id:03x}'

        if msg.data:
            cmd += ' ' + msg.data.hex(' ', 1)

        lines = self._shell.exec_command(cmd)
        regex_compiled = re.compile(r'enqueuing\s+CAN\s+frame\s+#(?P<id>\d+)')
        frame_num = None
        for line in lines:
            m = regex_compiled.match(line)
            if m:
                frame_num = m.group('id')
                break

        if frame_num is None:
            raise CanOperationError('frame not enqueued')

        tx_regex = r'CAN\s+frame\s+#' + frame_num + r'\s+successfully\s+sent'
        self._dut.readlines_until(regex=tx_regex, timeout=timeout)

    def _add_filter(self, can_id: int, can_mask: int, extended: bool) -> None:
        """Add RX filter."""
        cmd = f'can filter add {self._device}'
        cmd += ' -e' if extended else ''

        if extended:
            cmd += f' {can_id:08x}'
            cmd += f' {can_mask:08x}'
        else:
            cmd += f' {can_id:03x}'
            cmd += f' {can_mask:03x}'

        lines = self._shell.exec_command(cmd)
        regex_compiled = re.compile(r'filter\s+ID:\s+(?P<id>\d+)')
        for line in lines:
            m = regex_compiled.match(line)
            if m:
                filter_id = int(m.group('id'))
                self._filter_ids.append(filter_id)
                return

        raise CanOperationError('filter_id not found')

    def _remove_filter(self, filter_id: int) -> None:
        """Remove RX filter."""
        if filter_id in self._filter_ids:
            self._filter_ids.remove(filter_id)

        self._shell.exec_command(f'can filter remove {self._device} {filter_id}')
        retval = self._retval()
        if retval != 0:
            raise CanOperationError(f'failed to remove filter ID {filter_id} (err {retval})')

    def _remove_all_filters(self) -> None:
        """Remove all RX filters."""
        for filter_id in self._filter_ids[:]:
            self._remove_filter(filter_id)

    def _apply_filters(self, filters: Optional[CanFilters]) -> None:
        self._remove_all_filters()

        if filters:
            self._is_filtered = True
        else:
            # Accept all frames if no hardware filters provided
            filters = [
                {'can_id': 0x0, 'can_mask': 0x0},
                {'can_id': 0x0, 'can_mask': 0x0, 'extended': True}
            ]
            self._is_filtered = False

        for can_filter in filters:
            can_id = can_filter['can_id']
            can_mask = can_filter['can_mask']
            extended = can_filter['extended'] if 'extended' in can_filter else False
            self._add_filter(can_id, can_mask, extended)

    def _recv_internal(self, timeout: Optional[float]) -> Tuple[Optional[Message], bool]:
        frame_regex = r'.*' + re.escape(self._device) + \
            r'\s+(?P<brs>\S)(?P<esi>\S)\s+(?P<can_id>\d+)\s+\[(?P<dlc>\d+)\]\s*(?P<data>[a-z0-9 ]*)'
        lines = self._dut.readlines_until(regex=frame_regex, timeout=timeout)
        msg = None

        regex_compiled = re.compile(frame_regex)
        for line in lines:
            m = regex_compiled.match(line)
            if m:
                can_id = int(m.group('can_id'), 16)
                ext = len(m.group('can_id')) == 8
                dlc = int(m.group('dlc'))
                fd = len(m.group('dlc')) == 2
                brs = m.group('brs') == 'B'
                esi = m.group('esi') == 'P'
                data = bytearray.fromhex(m.group('data'))
                msg = Message(arbitration_id=can_id,is_extended_id=ext,
                              data=data, dlc=dlc,
                              is_fd=fd, bitrate_switch=brs, error_state_indicator=esi,
                              channel=self._device, check=True)
                logger.debug('received: %s', msg)

        return msg, self._is_filtered

    def shutdown(self) -> None:
        if not self._is_shutdown:
            super().shutdown()
            self._stop()
            self._remove_all_filters()
