# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging

from pathlib import Path
from yaml import safe_load
from dataclasses import dataclass, field


logger = logging.getLogger(__name__)


class QuarantineException(Exception):
    pass


class Quarantine:
    """Handle tests under quarantine."""

    def __init__(self, quarantine_list=[]) -> None:
        self.quarantine = QuarantineData()
        for quarantine_file in quarantine_list:
            self.quarantine.extend(QuarantineData.load_data_from_yaml(quarantine_file))

    def get_matched_quarantine(self, testname, platform, architecture):
        qelem = self.quarantine.get_matched_quarantine(testname, platform, architecture)
        if qelem:
            return qelem.comment
        return None


@dataclass
class QuarantineElement:
    scenarios: list[str] = field(default_factory=list)
    platforms: list[str] = field(default_factory=list)
    architectures: list[str] = field(default_factory=list)
    comment: str = 'under quarantine'

    def __post_init__(self):
        if 'all' in self.scenarios:
            self.scenarios = []
        if 'all' in self.platforms:
            self.platforms = []
        if 'all' in self.architectures:
            self.architectures = []
        if not any([self.scenarios, self.platforms, self.architectures]):
            raise QuarantineException("At least one of filters ('scenarios', 'platforms', "
                                      "'architectures') must be specified")


@dataclass
class QuarantineData:
    qlist: list[QuarantineElement] = field(default_factory=list)

    def __post_init__(self):
        qelements = []
        for qelem in self.qlist:
            qelements.append(QuarantineElement(**qelem))
        self.qlist = qelements

    @classmethod
    def load_data_from_yaml(cls, filename: str | Path) -> QuarantineData:
        """Load quarantine from yaml file."""
        with open(filename, 'r', encoding='UTF-8') as yaml_fd:
            qlist: list(dict) = safe_load(yaml_fd)
        try:
            return cls(qlist)

        except Exception as e:
            logger.error(f'When loading {filename} received error: {e}')
            raise QuarantineException('Cannot load Quarantine data') from e

    def extend(self, qdata: QuarantineData) -> list[QuarantineElement]:
        self.qlist.extend(qdata.qlist)

    def get_matched_quarantine(self, scenario: str, platform: str,
                               architecture: str) -> QuarantineElement | None:
        """Return quarantine element if test is matched to quarantine rules"""
        for qelem in self.qlist:
            matched: bool = False
            if qelem.scenarios:
                if scenario in qelem.scenarios:
                    matched = True
                else:
                    matched = False
                    continue
            if qelem.platforms:
                if platform in qelem.platforms:
                    matched = True
                else:
                    matched = False
                    continue
            if qelem.architectures:
                if architecture in qelem.architectures:
                    matched = True
                else:
                    matched = False
                    continue
            if matched:
                return qelem

        return None
