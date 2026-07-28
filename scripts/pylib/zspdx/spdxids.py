# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import re


def get_spdx_id_safe_character(c):
    """
    Converts a character to an SPDX-ID-safe character.

    Arguments:
        - c: character to test
    Returns: c if it is SPDX-ID-safe (letter, number, '-' or '.');
             '-' otherwise
    """
    if c.isalpha() or c.isdigit() or c == "-" or c == ".":
        return c
    return "-"


def convert_to_spdx_id_safe(s):
    """
    Converts a filename or other string to only SPDX-ID-safe characters.
    Note that a separate check (such as in get_unique_file_id, below) will need
    to be used to confirm that this is still a unique identifier, after
    conversion.

    Arguments:
        - s: string to be converted.
    Returns: string with all non-safe characters replaced with dashes.
    """
    return "".join([get_spdx_id_safe_character(c) for c in s])


def get_unique_file_id(filename_only, times_seen):
    """
    Find an SPDX ID that is unique among others seen so far.

    Arguments:
        - filename_only: filename only (directories omitted) seeking ID.
        - times_seen: dict of all filename-only to number of times seen.
    Returns: unique SPDX ID; updates times_seen to include it.
    """

    converted = convert_to_spdx_id_safe(filename_only)
    spdx_id = f"SPDXRef-File-{converted}"

    # determine whether spdx_id is unique so far, or not
    filename_times_seen = times_seen.get(converted, 0) + 1
    if filename_times_seen > 1:
        # we'll append the # of times seen to the end
        spdx_id += f"-{filename_times_seen}"
    else:
        # first time seeing this filename
        # edge case: if the filename itself ends in "-{number}", then we
        # need to add a "-1" to it, so that we don't end up overlapping
        # with an appended number from a similarly-named file.
        p = re.compile(r"-\d+$")
        if p.search(converted):
            spdx_id += "-1"

    times_seen[converted] = filename_times_seen
    return spdx_id
