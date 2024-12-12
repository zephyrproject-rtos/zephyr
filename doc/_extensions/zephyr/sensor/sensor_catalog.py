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


class Filters:
    def __init__(self, spec: pw_sensor.InputSpec):
        self.buses: set[str] = set()
        self.channels: set[str] = set()
        self.triggers: set[str] = set()
        self.attributes: dict[str, set[str]] = {}

        for sensor_spec in spec.sensors.values():
            self.channels.update(sensor_spec.channels.keys())
            self.buses.update([bus.upper() for bus in sensor_spec.supported_buses])
            self.triggers.update([trigger.upper() for trigger in sensor_spec.triggers])
            for sensor_attribute in sensor_spec.attributes:
                attribute_name = sensor_attribute.attribute.upper()
                self.attributes.setdefault(attribute_name, set()).add(
                    sensor_attribute.channel.upper()
                )


class SensorCatalogDirective(SensorDirective):
    """A catalog of all the current sensors"""

    def run(self) -> Sequence[nodes.Node]:
        filters = Filters(self.sensor_spec)
        catalog_node = self.render(
            page=SensorDirective.RenderPage.SENSOR_CATALOG,
            context={
                "channels": sorted(
                    [channel.upper() for channel in filters.channels],
                    key=SensorDirective.sort_channel_keys,
                ),
                "buses": sorted(list(filters.buses)),
                "triggers": sorted(list(filters.triggers)),
                "attributes": {key: list(value) for key, value in filters.attributes.items()},
                "sensors": self.sensor_spec.sensors,
            },
        )
        return [catalog_node]


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive(
        "sensor-catalog",
        SensorCatalogDirective,
        override=True,
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
