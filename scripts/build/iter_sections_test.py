#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import json

import iter_sections as sut


def test_parse_tagged_items_reads_names_and_inheritance(tmp_path):
    source = tmp_path / "struct_tags.json"
    source.write_text(
        json.dumps(
            {
                "__subsystem": [
                    "gpio",
                    {"name": "gpio_sx1509", "extends": "gpio"},
                ]
            }
        )
    )

    assert sut.parse_tagged_items(source, "__subsystem") == (
        ["gpio", "gpio_sx1509"],
        {"gpio": ["gpio_sx1509"]},
        {"gpio_sx1509"},
    )
