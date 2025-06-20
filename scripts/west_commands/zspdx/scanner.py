# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import hashlib
import os
import re
from dataclasses import dataclass

from reuse.project import Project
from west import log

from zspdx.licenses import LICENSES
from zspdx.util import getHashes


# ScannerConfig contains settings used to configure how the SPDX
# Document scanning should occur.
@dataclass(eq=True)
class ScannerConfig:
    # when assembling a Package's data, should we auto-conclude the
    # Package's license, based on the licenses of its Files?
    shouldConcludePackageLicense: bool = True

    # when assembling a Package's Files' data, should we auto-conclude
    # each File's license, based on its detected license(s)?
    shouldConcludeFileLicenses: bool = True

    # number of lines to scan for SPDX-License-Identifier (0 = all)
    # defaults to 20
    numLinesScanned: int = 20

    # should we calculate SHA256 hashes for each Package's Files?
    # note that SHA1 hashes are mandatory, per SPDX 2.3
    doSHA256: bool = True

    # should we calculate MD5 hashes for each Package's Files?
    doMD5: bool = False


def parseLineForExpression(line):
    """Return parsed SPDX expression if tag found in line, or None otherwise."""
    p = line.partition("SPDX-License-Identifier:")
    if p[2] == "":
        return None
    # strip away trailing comment marks and whitespace, if any
    expression = p[2].strip()
    expression = expression.rstrip("/*")
    expression = expression.strip()
    return expression


def getExpressionData(filePath, numLines):
    """
    Scans the specified file for the first SPDX-License-Identifier:
    tag in the file.

    Arguments:
        - filePath: path to file to scan.
        - numLines: number of lines to scan for an expression before
                    giving up. If 0, will scan the entire file.
    Returns: parsed expression if found; None if not found.
    """
    log.dbg(f"  - getting licenses for {filePath}")

    with open(filePath) as f:
        try:
            for lineno, line in enumerate(f, start=1):
                if lineno > numLines > 0:
                    break
                expression = parseLineForExpression(line)
                if expression is not None:
                    return expression
        except UnicodeDecodeError:
            # invalid UTF-8 content
            return None

    # if we get here, we didn't find an expression
    return None


def splitExpression(expression):
    """
    Parse a license expression into its constituent identifiers.

    Arguments:
        - expression: SPDX license expression
    Returns: array of split identifiers
    """
    # remove parens and plus sign
    e2 = re.sub(r'\(|\)|\+', "", expression, flags=re.IGNORECASE)

    # remove word operators, ignoring case, leaving a blank space
    e3 = re.sub(r' AND | OR | WITH ', " ", e2, flags=re.IGNORECASE)

    # and split on space
    e4 = e3.split(" ")

    return sorted(e4)


def calculateVerificationCode(pkg):
    """
    Calculate the SPDX Package Verification Code for all files in the package.

    Arguments:
        - pkg: Package
    Returns: verification code as string
    """
    hashes = []
    for f in pkg.files.values():
        hashes.append(f.sha1)
    hashes.sort()
    filelist = "".join(hashes)

    hSHA1 = hashlib.sha1(usedforsecurity=False)
    hSHA1.update(filelist.encode('utf-8'))
    return hSHA1.hexdigest()


def checkLicenseValid(lic, doc):
    """
    Check whether this license ID is a valid SPDX license ID, and add it
    to the custom license IDs set for this Document if it isn't.

    Arguments:
        - lic: detected license ID
        - doc: Document
    """
    if lic not in LICENSES:
        doc.customLicenseIDs.add(lic)


def getPackageLicenses(pkg):
    """
    Extract lists of all concluded and infoInFile licenses seen.

    Arguments:
        - pkg: Package
    Returns: sorted list of concluded license exprs,
             sorted list of infoInFile ID's
    """
    licsConcluded = set()
    licsFromFiles = set()
    for f in pkg.files.values():
        licsConcluded.add(f.concludedLicense)
        for licInfo in f.licenseInfoInFile:
            licsFromFiles.add(licInfo)
    return sorted(list(licsConcluded)), sorted(list(licsFromFiles))


def normalizeExpression(licsConcluded):
    """
    Combine array of license expressions into one AND'd expression,
    adding parens where needed.

    Arguments:
        - licsConcluded: array of license expressions
    Returns: string with single AND'd expression.
    """
    # return appropriate for simple cases
    if len(licsConcluded) == 0:
        return "NOASSERTION"
    if len(licsConcluded) == 1:
        return licsConcluded[0]

    # more than one, so we'll need to combine them
    # if and only if an expression has spaces, it needs parens
    revised = []
    for lic in licsConcluded:
        if lic in ["NONE", "NOASSERTION"]:
            continue
        if " " in lic:
            revised.append(f"({lic})")
        else:
            revised.append(lic)
    return " AND ".join(revised)


def getCopyrightInfo(filePath):
    """
    Scans the specified file for copyright information using REUSE tools.

    Arguments:
        - filePath: path to file to scan

    Returns: list of copyright statements if found; empty list if not found
    """
    log.dbg(f"  - getting copyright info for {filePath}")

    try:
        project = Project(os.path.dirname(filePath))
        infos = project.reuse_info_of(filePath)
        copyrights = []

        for info in infos:
            if info.copyright_lines:
                copyrights.extend(info.copyright_lines)

        return copyrights
    except Exception as e:
        log.wrn(f"Error getting copyright info for {filePath}: {e}")
        return []


def scanDocument(cfg, doc):
    """
    Scan for licenses and calculate hashes for all Files and Packages
    in this Document.

    Arguments:
        - cfg: ScannerConfig
        - doc: Document
    """
    for pkg in doc.pkgs.values():
        log.inf(f"scanning files in package {pkg.cfg.name} in document {doc.cfg.name}")

        # first, gather File data for this package
        for f in pkg.files.values():
            # set relpath based on package's relativeBaseDir
            f.relpath = os.path.relpath(f.abspath, pkg.cfg.relativeBaseDir)

            # get hashes for file
            hashes = getHashes(f.abspath)
            if not hashes:
                log.wrn(f"unable to get hashes for file {f.abspath}; skipping")
                continue
            hSHA1, hSHA256, hMD5 = hashes
            f.sha1 = hSHA1
            if cfg.doSHA256:
                f.sha256 = hSHA256
            if cfg.doMD5:
                f.md5 = hMD5

            # get licenses for file
            expression = getExpressionData(f.abspath, cfg.numLinesScanned)
            if expression:
                if cfg.shouldConcludeFileLicenses:
                    f.concludedLicense = expression
                f.licenseInfoInFile = splitExpression(expression)

            if copyrights := getCopyrightInfo(f.abspath):
                f.copyrightText = "<text>\n" + "\n".join(copyrights) + "\n</text>"

            # check if any custom license IDs should be flagged for document
            for lic in f.licenseInfoInFile:
                checkLicenseValid(lic, doc)

        # now, assemble the Package data
        licsConcluded, licsFromFiles = getPackageLicenses(pkg)
        if cfg.shouldConcludePackageLicense:
            pkg.concludedLicense = normalizeExpression(licsConcluded)
        pkg.licenseInfoFromFiles = licsFromFiles
        pkg.verificationCode = calculateVerificationCode(pkg)
