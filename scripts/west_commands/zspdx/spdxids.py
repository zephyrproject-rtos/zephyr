# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import re

def getSPDXIDSafeCharacter(c):
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

def convertToSPDXIDSafe(s):
    """
    Converts a filename or other string to only SPDX-ID-safe characters.
    Note that a separate check (such as in getUniqueID, below) will need
    to be used to confirm that this is still a unique identifier, after
    conversion.

    Arguments:
        - s: string to be converted.
    Returns: string with all non-safe characters replaced with dashes.
    """
    return "".join([getSPDXIDSafeCharacter(c) for c in s])

def getUniqueFileID(filenameOnly, timesSeen):
    """
    Find an SPDX ID that is unique among others seen so far.

    Arguments:
        - filenameOnly: filename only (directories omitted) seeking ID.
        - timesSeen: dict of all filename-only to number of times seen.
    Returns: unique SPDX ID; updates timesSeen to include it.
    """

    converted = convertToSPDXIDSafe(filenameOnly)
    spdxID = f"SPDXRef-File-{converted}"

    # determine whether spdxID is unique so far, or not
    filenameTimesSeen = timesSeen.get(converted, 0) + 1
    if filenameTimesSeen > 1:
        # we'll append the # of times seen to the end
        spdxID += f"-{filenameTimesSeen}"
    else:
        # first time seeing this filename
        # edge case: if the filename itself ends in "-{number}", then we
        # need to add a "-1" to it, so that we don't end up overlapping
        # with an appended number from a similarly-named file.
        p = re.compile(r"-\d+$")
        if p.search(converted):
            spdxID += "-1"

    timesSeen[converted] = filenameTimesSeen
    return spdxID
