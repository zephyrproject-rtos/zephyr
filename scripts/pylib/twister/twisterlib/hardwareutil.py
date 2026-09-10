# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
from collections.abc import Iterator
from contextlib import contextmanager

from twisterlib.error import NoDeviceAvailableException, TwisterException
from twisterlib.hardwaredata import CompoundHardwareData, HardwareData
from twisterlib.hardwaremap import DUT, HardwareMap
from twisterlib.testsuitedata import HarnessConfig

logger = logging.getLogger('twister')


class HardwareReservationManager:
    """Manages hardware reservations for test execution."""

    def __init__(self, hwm: HardwareMap, platform: str, harness_config: HarnessConfig):
        self.hwm = hwm
        self.platform = platform
        self.harness_config = harness_config
        self.reserved_duts: list[DUT] = []

    def reserve_duts(self) -> None:
        """Reserve devices for the test instance."""
        matched_duts = self._get_matched_duts(self.platform, self.harness_config.fixture)

        if not self.harness_config.required_devices:
            dut = self.reserve_dut(matched_duts)
            self.reserved_duts.append(dut)
        else:
            if not self.is_runnable():
                # should not happen as is_runnable is checked in testplan generation
                raise TwisterException(f"Required devices for {self.platform} are not available")
            try:
                while dut := self.reserve_dut(matched_duts):
                    if self._can_match_required_devices(dut):
                        break
                    self.release_dut(dut)
                    matched_duts.remove(dut)
                self.reserved_duts.append(dut)
                self._reserve_required_devices()
            except NoDeviceAvailableException:
                # release any reserved devices without logging and updating counters
                for dut in self.reserved_duts:
                    self.release_dut(dut)
                self.reserved_duts = []
                raise  # re-raise to be handled in runner to postpone processing

        # Increment counter only on main DUT
        dut.counter_increment()
        self._log_after_reservation()

    def release_duts(self, failed: bool = False) -> None:
        """Release all reserved devices."""
        if not self.reserved_duts:
            return
        for dut in self.reserved_duts:
            self.release_dut(dut)

        # Increment failures only on main DUT
        if failed:
            self.reserved_duts[0].failures_increment()

        self._log_after_release()
        self.reserved_duts = []

    def is_runnable(self) -> bool:
        """Check if the required devices for the test instance are available."""
        matched_duts = self._get_matched_duts(
            self.platform, self.harness_config.fixture, raise_exception=False
        )
        if not matched_duts:
            return False
        if self.harness_config.required_devices:
            return any(self._can_match_required_devices(dut) for dut in matched_duts)
        return True

    def _reserve_required_devices(self) -> None:
        """Reserve the required devices (not main DUT) for the test instance."""
        if not self.harness_config.required_devices or not self.reserved_duts:
            return
        main_dut = self.reserved_duts[0]
        exclude_dev_ids = {main_dut.id}
        fixtures_params = self._get_required_fixtures_params(main_dut)
        for req_dev in self.harness_config.required_devices:
            matched_req_duts = self._get_matched_duts(
                platform=req_dev.platform or self.platform,
                fixture=req_dev.fixture,
                exclude_dev_ids=exclude_dev_ids,
                fixtures_params=fixtures_params,
            )
            req_dut = self.reserve_dut(matched_req_duts)
            self.reserved_duts.append(req_dut)
            exclude_dev_ids.add(req_dut.id)

    def _get_matched_duts(
        self,
        platform: str,
        fixture: list[str] | None = None,
        exclude_dev_ids: set[str] | None = None,
        fixtures_params: dict | None = None,
        raise_exception: bool = True,
    ) -> list[DUT]:
        """Get a list of DUTs that match the specified device and fixture criteria."""
        matched_duts: list[DUT] = []
        for d in self.hwm.duts:
            if d.platform != platform or (d.serial is None and d.serial_pty is None):
                continue
            if fixture and not all(f in (f.split(sep=':')[0] for f in d.fixtures) for f in fixture):
                continue
            matched_duts.append(d)

        # multi DUT tests: Exclude devices that are already used by current test
        if exclude_dev_ids:
            matched_duts = [d for d in matched_duts if d.id not in exclude_dev_ids]

        # multi DUT tests: Exclude devices that do not match the fixture parameters
        # e.g. main DUT requires fixture1:param1, so exclude devices with fixture1:param2 etc.
        if fixtures_params:
            for d in matched_duts.copy():
                sep_fixtures = self._get_required_fixtures_params(d)
                for f in fixtures_params:
                    if f not in sep_fixtures or sep_fixtures[f] != fixtures_params[f]:
                        matched_duts.remove(d)
                        break

        if not matched_duts and raise_exception:
            msg = f"No device - {platform=}"
            if fixture:
                msg += f" {fixture=}"
            logger.error(msg)
            raise TwisterException(msg)
        return matched_duts

    def _can_match_required_devices(self, dut: DUT) -> bool:
        """Check if the required devices exists and can be matched with the provided DUT."""
        exclude_dev_ids = {dut.id}
        fixtures_params = self._get_required_fixtures_params(dut)
        for req_dev in self.harness_config.required_devices:
            matched_req_duts = self._get_matched_duts(
                platform=req_dev.platform or self.platform,
                fixture=req_dev.fixture,
                exclude_dev_ids=exclude_dev_ids,
                fixtures_params=fixtures_params,
                raise_exception=False,
            )
            if not matched_req_duts:
                # not found a required device matching the criteria for this DUT
                return False
            # exclude first matched device from next iterations,
            # if more required devices are needed
            exclude_dev_ids.add(matched_req_duts[0].id)
        return True

    def _get_required_fixtures_params(self, dut: DUT) -> dict:
        """Get a dictionary of fixture parameters for the provided DUT,
        do not consider fixtures that are not required by the test instance"""
        if not self.harness_config.fixture:
            return {}
        fixtures_params = {}
        for f in dut.fixtures:
            fp = f.split(':', 1)
            if fp[0] not in self.harness_config.fixture:
                continue
            if len(fp) == 2:
                fixtures_params[fp[0]] = fp[1]
        return fixtures_params

    def _log_after_reservation(self) -> None:
        if not self.reserved_duts:
            return
        dut = self.reserved_duts[0]
        logger.debug(
            f"Retain DUT:{dut.platform}, Id:{dut.id}, "
            f"counter:{dut.counter}, failures:{dut.failures}"
        )
        # log if there is more devices reserved for multi dut tests
        if len(self.reserved_duts) > 1:
            logger.debug(
                "Retain required device(s) for multi DUT test: "
                + ", ".join([f"{d.platform} id {d.id}" for d in self.reserved_duts[1:]])
            )

    def _log_after_release(self) -> None:
        if not self.reserved_duts:
            return
        dut = self.reserved_duts[0]
        logger.debug(
            f"Release DUT:{dut.platform}, Id:{dut.id}, "
            f"counter:{dut.counter}, failures:{dut.failures}"
        )
        # log if there is more devices released for multi dut tests
        if len(self.reserved_duts) > 1:
            logger.debug(
                "Release required device(s) for multi DUT test: "
                + ", ".join([f"{d.platform} id {d.id}" for d in self.reserved_duts[1:]])
            )

    @staticmethod
    @contextmanager
    def acquire_dut_locks(duts: list[DUT]) -> Iterator[None]:
        try:
            for d in duts:
                d.lock.acquire()
            yield
        finally:
            for d in duts:
                d.lock.release()

    def reserve_dut(self, duts_found: list[DUT]) -> DUT:
        """Reserve DUT from the list of matched DUTs, selecting the one with less failures."""
        for d in sorted(duts_found, key=lambda _dut: _dut.failures):
            # get all DUTs with the same id
            duts_shared_hw = [_d for _d in self.hwm.duts if _d.id == d.id]
            with self.acquire_dut_locks(duts_shared_hw):
                if d.available:
                    for _d in duts_shared_hw:
                        _d.available = 0
                    return d
        raise NoDeviceAvailableException(f"No free {duts_found[0].platform} devices available")

    def release_dut(self, dut: DUT) -> None:
        """Release a previously reserved DUT, making it available for future reservations."""
        # get all DUTs with the same id
        duts_shared_hw = [_d for _d in self.hwm.duts if _d.id == dut.id]
        with self.acquire_dut_locks(duts_shared_hw):
            for _d in duts_shared_hw:
                _d.available = 1

    def _get_other_duts_with_same_id(self, hardware: DUT) -> list[DUT]:
        """Get all DUTs that share the same hardware ID as the provided DUT,
        excluding the provided DUT itself."""
        duts: list[DUT] = []
        # get all DUTs with the same id
        duts_shared_hw = [_d for _d in self.hwm.duts if _d.id == hardware.id]
        serials = {hardware.serial}
        for d in duts_shared_hw:
            if d.serial and d.serial not in serials:
                duts.append(d)
                serials.add(d.serial)
        return duts

    def create_compound_hardware_data(self, hardware: DUT) -> CompoundHardwareData:
        """Create a CompoundHardwareData object for the provided DUT,
        including any auxiliary connections that share the same hardware ID."""
        compound_hardware = CompoundHardwareData.from_dict(hardware.to_dict())
        for dut in self._get_other_duts_with_same_id(hardware):
            compound_hardware.entries.append(HardwareData(**dut.to_dict()))
        return compound_hardware

    def get_reserved_duts_as_compound_hardware_data(self) -> list[CompoundHardwareData]:
        """Get the list of currently reserved devices as CompoundHardwareData."""
        duts = []
        for dut in self.reserved_duts:
            duts.append(self.create_compound_hardware_data(dut))
        return duts
