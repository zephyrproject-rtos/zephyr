# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import re

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

    def get_matched_quarantine(self, testname, platform, architecture, simulation):
        qelem = self.quarantine.get_matched_quarantine(testname, platform, architecture, simulation)
        if qelem:
            logger.debug('%s quarantined with reason: %s' % (testname, qelem.comment))
            return qelem.comment
        return None


@dataclass
class QuarantineElement:
    scenarios: list[str] = field(default_factory=list)
    platforms: list[str] = field(default_factory=list)
    architectures: list[str] = field(default_factory=list)
    simulations: list[str] = field(default_factory=list)
    comment: str = 'NA'
    re_scenarios: list = field(default_factory=list)
    re_platforms: list = field(default_factory=list)
    re_architectures: list = field(default_factory=list)
    re_simulations: list = field(default_factory=list)

    def __post_init__(self):
        # If there is no entry in filters then take all possible values.
        # To keep backward compatibility, 'all' keyword might be still used.
        if 'all' in self.scenarios:
            self.scenarios = []
        if 'all' in self.platforms:
            self.platforms = []
        if 'all' in self.architectures:
            self.architectures = []
        if 'all' in self.simulations:
            self.simulations = []
        # keep precompiled regexp entiries to speed-up matching
        self.re_scenarios = [re.compile(pat) for pat in self.scenarios]
        self.re_platforms = [re.compile(pat) for pat in self.platforms]
        self.re_architectures = [re.compile(pat) for pat in self.architectures]
        self.re_simulations = [re.compile(pat) for pat in self.simulations]

        # However, at least one of the filters ('scenarios', platforms' ...)
        # must be given (there is no sense to put all possible configuration
        # into quarantine)
        if not any([self.scenarios, self.platforms, self.architectures, self.simulations]):
            raise QuarantineException("At least one of filters ('scenarios', 'platforms' ...) "
                                      "must be specified")


@dataclass
class QuarantineData:
    qlist: list[QuarantineElement] = field(default_factory=list)

    def __post_init__(self):
        qelements = []
        for qelem in self.qlist:
            if isinstance(qelem, QuarantineElement):
                qelements.append(qelem)
            else:
                qelements.append(QuarantineElement(**qelem))
        self.qlist = qelements

    @classmethod
    def load_data_from_yaml(cls, filename: str | Path) -> QuarantineData:
        """Load quarantine from yaml file."""
        with open(filename, 'r', encoding='UTF-8') as yaml_fd:
            qlist_raw_data: list[dict] = safe_load(yaml_fd)
        try:
            if not qlist_raw_data:
                # in case of loading empty quarantine file
                return cls()
            return cls(qlist_raw_data)

        except Exception as e:
            logger.error(f'When loading {filename} received error: {e}')
            raise QuarantineException('Cannot load Quarantine data') from e

    def extend(self, qdata: QuarantineData) -> None:
        self.qlist.extend(qdata.qlist)

    def get_matched_quarantine(self,
                               scenario: str,
                               platform: str,
                               architecture: str,
                               simulation: str) -> QuarantineElement | None:
        """Return quarantine element if test is matched to quarantine rules"""
        for qelem in self.qlist:
            matched: bool = False
            if (qelem.scenarios
                    and (matched := _is_element_matched(scenario, qelem.re_scenarios)) is False):
                continue
            if (qelem.platforms
                    and (matched := _is_element_matched(platform, qelem.re_platforms)) is False):
                continue
            if (qelem.architectures
                    and (matched := _is_element_matched(architecture, qelem.re_architectures)) is False):
                continue
            if (qelem.simulations
                    and (matched := _is_element_matched(simulation, qelem.re_simulations)) is False):
                continue

            if matched:
                return qelem
        return None


def _is_element_matched(element: str, list_of_elements: list[re.Pattern]) -> bool:
    """Return True if given element is matching to any of elements from the list"""
    for pattern in list_of_elements:
        if pattern.fullmatch(element):
            return True
    return False
