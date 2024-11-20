# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: Apache-2.0

from typing import Any

from sphinx.application import Sphinx

__version__ = "0.0.1"

def setup(app: Sphinx) -> dict[str, Any]:
    """Introduce the sensor utilities"""

    # Add the sensor_yaml_files configuration value
    app.add_config_value(
        name="sensor_yaml_files", default="", rebuild="html", types=str
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
