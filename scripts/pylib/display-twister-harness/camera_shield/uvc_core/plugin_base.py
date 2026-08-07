# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

from abc import ABC, abstractmethod


class DetectionPlugin(ABC):
    def __init__(self, name, config: dict):
        self.name = name
        self.config = config
        self.update_count = 0
        self.old_result = {}

    @abstractmethod
    def initialize(self):
        """initialize"""

    @abstractmethod
    def process_frame(self, frame) -> dict:
        """process_frame"""

    @abstractmethod
    def handle_results(self, result, frame):
        """handle_results"""

    @abstractmethod
    def shutdown(self) -> list:
        """release resources"""


class PluginManager:
    def __init__(self):
        self.plugins = {}

    def register_plugin(self, name: str, plugin_class):
        self.plugins[name] = plugin_class

    def create_plugin(self, name: str, config: dict) -> DetectionPlugin:
        return self.plugins[name](config)
