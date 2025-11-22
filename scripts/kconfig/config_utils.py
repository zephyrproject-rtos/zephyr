#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Common utilities for Kconfig configuration interfaces.

This module provides shared functionality for menuconfig.py and guiconfig.py.
"""

import re

from kconfiglib import Choice, Symbol


def score_search_matches(search_str, nodes):
    """
    Scores and sorts search results for Kconfig nodes based on relevance.

    This implements a basic scoring system where:
    - A match in a symbol's name is given more weight than a match in its prompt
    - Field-length normalization is applied so that the shorter the field, the higher its relevance

    Args:
        search_str: The search string (space-separated regexes)
        nodes: List of MenuNode objects to search through

    Returns:
        List of tuples (node, score) sorted by score (highest first)
    """
    # Parse the search string into regexes
    try:
        regexes = [re.compile(regex.lower()) for regex in search_str.split()]
        # If no regexes, all nodes match, order is unchanged
        if len(regexes) == 0:
            return [(node, 1) for node in nodes]
    except re.error:
        # Invalid regex - return empty results
        return []

    NAME_WEIGHT = 2.0
    PROMPT_WEIGHT = 1.0

    scored_results = []

    for node in nodes:
        # Get lowercase versions for matching
        name = ""
        if isinstance(node.item, Symbol | Choice):
            name = node.item.name.lower() if node.item.name else ""

        prompt = node.prompt[0].lower() if node.prompt else ""

        # Check if all regexes match in either name or prompt
        name_matches = name and all(regex.search(name) for regex in regexes)
        prompt_matches = prompt and all(regex.search(prompt) for regex in regexes)

        if not (name_matches or prompt_matches):
            continue

        # Apply field-length normalization (shorter fields = higher relevance)
        score = 0
        if name_matches:
            score += NAME_WEIGHT / (len(name) ** 0.5)
        if prompt_matches:
            score += PROMPT_WEIGHT / (len(prompt) ** 0.5)

        scored_results.append((node, score))

    # Sort by score (highest first)
    scored_results.sort(key=lambda x: x[1], reverse=True)

    return scored_results
