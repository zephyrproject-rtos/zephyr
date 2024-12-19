# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: Apache-2.0

from collections.abc import Sequence
from typing import Any

import pw_sensor.constants_generator as pw_sensor
from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util import logging

from zephyr.sensor import SensorDirective

__version__ = "0.0.1"
logger = logging.getLogger(__name__)


class ChannelCatalogDirective(SensorDirective):
    """A catalog of the available sensor channels"""

    def run(self) -> Sequence[nodes.Node]:
        current_section = self.state.parent

        channel_names = list(self.sensor_spec.channels.keys())
        channel_names.sort(key=SensorDirective.sort_channel_keys)

        for key in channel_names:
            channel: pw_sensor.ChannelSpec = self.sensor_spec.channels[key]
            channel_section = self.create_section(
                title=channel.name.title(),
                content=channel.description,
                id_string=f"sensor-chan-{key}",
            )

            channel_section += nodes.admonition(
                "",
                nodes.paragraph(
                    "",
                    "How to use?",
                    classes=["admonition-title"],
                ),
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("To use this channel in a sensor.yaml file use: "),
                    nodes.literal(text=key),
                ),
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("To use this channel in C/C++, use: "),
                    nodes.literal(text=f"SENSOR_CHAN_{key.upper()}"),
                ),
                classes=["note"],
            )

            current_section += channel_section
        return []


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive(
        "sensor-channel-catalog",
        ChannelCatalogDirective,
        override=True,
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
