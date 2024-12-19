# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: Apache-2.0

from collections.abc import Sequence
from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.util import logging

from zephyr.sensor import SensorDirective

__version__ = "0.0.1"
logger = logging.getLogger(__name__)


class SensorDetailsDirective(SensorDirective):
    """A sensor details card"""

    option_spec = {
        "compatible": directives.unchanged,
    }

    def run(self) -> Sequence[nodes.Node]:
        if "compatible" not in self.options:
            logger.error("Missing 'compatible' option")
            return []

        detail_node = self.render(
            page=SensorDirective.RenderPage.SENSOR_DETAIL_CARD,
            context={
                "sensor": self.sensor_spec.sensors[self.options["compatible"]],
                "bus_references": {
                    "espi": "espi_api",
                    "i2c": "i2c_api",
                    "i3c": "i3c_api",
                    "spi": "spi_api",
                },
            },
        )

        self.state.parent += detail_node
        return []


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive(
        "sensor-details",
        SensorDetailsDirective,
        override=True,
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
