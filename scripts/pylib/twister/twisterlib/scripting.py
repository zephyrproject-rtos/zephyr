#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations
import logging
import re
from pathlib import Path
from yaml import safe_load
from dataclasses import dataclass, field
import sys

logger = logging.getLogger(__name__)


#Handles test scripting configurations.
class Scripting:
    def __init__(self, scripting_files: list[Path | str] = None) -> None:
        self.scripting = ScriptingData()
        scripting_files = scripting_files or []
        for scripting_file in scripting_files:
            self.scripting.extend(ScriptingData.load_from_yaml(scripting_file))

    #Finds and returns the scripting element that matches the given test name and platform.
    def get_matched_scripting(self, testname: str, platform: str) -> ScriptingElement | None:
        matched_scripting = self.scripting.find_matching_scripting(testname, platform)
        if matched_scripting:
            logger.debug(f'{testname} scripting with reason: {matched_scripting.comment}')
            return matched_scripting
        return None

@dataclass
#Represents a single scripting element with associated scripts and metadata.
class ScriptingElement:
    scenarios: list[str] = field(default_factory=list)
    platforms: list[str] = field(default_factory=list)
    pre_script: str | None = None
    post_flash_script: str | None = None
    post_script: str | None = None
    comment: str = 'NA'
    re_scenarios: list[re.Pattern] = field(init=False, default_factory=list)
    re_platforms: list[re.Pattern] = field(init=False, default_factory=list)

    #Compiles regex patterns for scenarios and platforms, and validates the element.
    def __post_init__(self):
        self.re_scenarios = [re.compile(pat) for pat in self.scenarios if pat != 'all']
        self.re_platforms = [re.compile(pat) for pat in self.platforms if pat != 'all']
        if not any([self.scenarios, self.platforms, self.pre_script, self.post_flash_script, self.post_script]):
            logger.error("At least one of the properties must be specified")
            sys.exit(1)

@dataclass
#Holds a collection of scripting elements.
class ScriptingData:
    elements: list[ScriptingElement] = field(default_factory=list)

    #Ensures all elements are ScriptingElement instances.
    def __post_init__(self):
        self.elements = [elem if isinstance(elem, ScriptingElement) else ScriptingElement(**elem) for elem in self.elements]

    @classmethod
    #Loads scripting data from a YAML file.
    def load_from_yaml(cls, filename: Path | str) -> ScriptingData:
        try:
            with open(filename, 'r', encoding='UTF-8') as file:
                raw_data = safe_load(file) or []
            return cls(raw_data)
        except Exception as e:
            logger.error(f'Error loading {filename}: {e}')
            sys.exit(1)

    #Extends the current scripting data with another set of scripting data.
    def extend(self, other: ScriptingData) -> None:
        self.elements.extend(other.elements)

    #Finds a scripting element that matches the given scenario and platform.
    def find_matching_scripting(self, scenario: str, platform: str) -> ScriptingElement | None:
        for element in self.elements:
            if element.scenarios and not _matches_element(scenario, element.re_scenarios):
                continue
            if element.platforms and not _matches_element(platform, element.re_platforms):
                continue
            return element
        return None

#Checks if the given element matches any of the provided regex patterns.
def _matches_element(element: str, patterns: list[re.Pattern]) -> bool:
    return any(pattern.match(element) for pattern in patterns)
