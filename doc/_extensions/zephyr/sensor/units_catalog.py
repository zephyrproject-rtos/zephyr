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


class UnitsCatalogDirective(SensorDirective):
    """A catalog of the available sensor units"""

    def run(self) -> Sequence[nodes.Node]:
        current_section = self.state.parent

        unit_names = list(self.sensor_spec.units.keys())
        unit_names.sort()

        for key in unit_names:
            unit: pw_sensor.UnitsSpec = self.sensor_spec.units[key]
            unit_section = self.create_section(
                title=unit.name.title(),
                content=None,  # TODO add unit.description
                id_string=f"sensor-units-{key}",
            )

            unit_section += nodes.admonition(
                "",
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("This unit is measured in "),
                    nodes.math(text=unit.symbol),
                    classes=["admonition-title"],
                ),
                nodes.paragraph(
                    "",
                    "",
                    nodes.Text("To use this unit in a sensor.yaml file, use: "),
                    nodes.literal(text=key),
                ),
                classes=["note"],
            )
            current_section.append(unit_section)

        return []


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive(
        "sensor-units-catalog",
        UnitsCatalogDirective,
        override=True,
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
