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


class TriggerCatalogDirective(SensorDirective):
    """A catalog of the available sensor triggers"""

    def run(self) -> Sequence[nodes.Node]:
        current_section = self.state.parent

        trigger_names = list(self.sensor_spec.triggers.keys())
        trigger_names.sort()

        for key in trigger_names:
            trigger: pw_sensor.TriggerSpec = self.sensor_spec.triggers[key]
            trigger_section = self.create_section(
                title=trigger.name.title(),
                content=trigger.description,
                id_string=f"sensor-trigger-{key}",
            )

            trigger_section += nodes.admonition(
                "",
                nodes.paragraph(
                    "",
                    "How to use?",
                    classes=["admonition-title"],
                ),
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("To use this trigger in a sensor.yaml file use: "),
                    nodes.literal(text=key),
                ),
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("To use this trigger in C/C++, use: "),
                    nodes.literal(text=f"SENSOR_TRIG_{key.upper()}"),
                ),
                classes=["note"],
            )

            current_section += trigger_section
        return []


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive(
        "sensor-trigger-catalog",
        TriggerCatalogDirective,
        override=True,
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
