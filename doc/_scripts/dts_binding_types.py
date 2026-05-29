# Copyright (c) 2025 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

import re
from pathlib import Path

ZEPHYR_BASE = Path(__file__).parents[2]
ZEPHYR_BINDINGS = ZEPHYR_BASE / "dts" / "bindings"
BINDINGS_TXT_PATH = ZEPHYR_BINDINGS / "binding-types.txt"

ACRONYM_PATTERN = re.compile(r'([a-zA-Z0-9-]+)\s*\((.*?)\)')
ACRONYM_PATTERN_UPPERCASE_ONLY = re.compile(r'(\b[A-Z0-9-]+)\s*\((.*?)\)')


def get_binding_type_from_path(binding_path: Path) -> str:
    """
    Get the binding type (e.g. 'gpio', 'sensor', etc.) from the binding file's path.
    """
    if binding_path.is_relative_to(ZEPHYR_BINDINGS):
        return binding_path.relative_to(ZEPHYR_BINDINGS).parts[0]
    return "misc"


def parse_text_with_acronyms(text: str, uppercase_only: bool = False) -> list:
    """
    Parse text that may contain acronyms into a list of chunks.

    Returns a list of dictionaries with structure:
    - {"type": "text", "content": "..."}
    - {"type": "acronym", "abbr": "...", "explanation": "..."}
    """
    result = []
    last_end = 0

    pattern = ACRONYM_PATTERN_UPPERCASE_ONLY if uppercase_only else ACRONYM_PATTERN
    for match in pattern.finditer(text):
        if match.start() > last_end:
            result.append({"type": "text", "content": text[last_end : match.start()]})

        abbr, explanation = match.groups()
        result.append({"type": "acronym", "abbr": abbr, "explanation": explanation})
        last_end = match.end()

    if last_end < len(text):
        result.append({"type": "text", "content": text[last_end:]})

    return result


def load_binding_types() -> dict:
    """
    Load the binding types from the registry text file.

    Returns a dictionary mapping binding types (e.g. 'gpio')
    to their parsed description (list of text/acronym chunks).
    """
    binding_types = {}
    if not BINDINGS_TXT_PATH.exists():
        return binding_types

    with open(BINDINGS_TXT_PATH) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            # Split only on the first tab
            parts = line.split('\t', 1)
            if len(parts) == 2:
                key, value = parts
                binding_types[key] = parse_text_with_acronyms(value)

    return binding_types
