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


class AttributeCatalogDirective(SensorDirective):
    """A catalog of the available sensor attributes"""

    def run(self) -> Sequence[nodes.Node]:
        current_section = self.state.parent

        attribute_names = list(self.sensor_spec.attributes.keys())
        attribute_names.sort()

        for key in attribute_names:
            attribute: pw_sensor.AttributeSpec = self.sensor_spec.attributes[key]
            attribute_section = self.create_section(
                title=attribute.name.title(),
                content=attribute.description,
                id_string=f"sensor-attribute-{key}",
            )

            attribute_section += nodes.admonition(
                "",
                nodes.paragraph(
                    "",
                    text="How to use?",
                    classes=["admonition-title"],
                ),
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("To use this attribute in sensor.yaml file use: "),
                    nodes.literal(text=key),
                ),
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("To use this attribute in C/C++, use: "),
                    nodes.literal(text=f"SENSOR_ATTR_{key.upper()}"),
                ),
                classes=["note"],
            )

            current_section += attribute_section
        return []


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive(
        "sensor-attribute-catalog",
        AttributeCatalogDirective,
        override=True,
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
