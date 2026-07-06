# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import re
from dataclasses import dataclass

from reuse.project import Project

from .licenses import LICENSES
from .util import get_hashes

_logger = logging.getLogger(__name__)

# Quiet the reuse library's own logger as it may warn about REUSE-compliance
# issues (e.g. malformed LICENSES/ file names).
logging.getLogger("reuse").setLevel(logging.ERROR)


# ScannerConfig contains settings used to configure how the SBOM
# scanning should occur.
@dataclass(eq=True)
class ScannerConfig:
    # when assembling a Component's data, should we auto-conclude the
    # Component's license, based on the licenses of its Files?
    should_conclude_component_license: bool = True

    # when assembling a Component's Files' data, should we auto-conclude
    # each File's license, based on its detected license(s)?
    should_conclude_file_licenses: bool = True

    # should we calculate SHA256 hashes for each Component's Files?
    # note that SHA1 hashes are mandatory, per SPDX 2.3
    do_sha256: bool = True

    # should we calculate MD5 hashes for each Component's Files?
    do_md5: bool = False


def split_expression(expression):
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


def check_license_valid(lic, sbom_graph):
    """
    Check whether this license ID is a valid SPDX license ID, and add it
    to the custom license IDs set for this SBOM if it isn't.

    Arguments:
        - lic: detected license ID
        - sbom_graph: SBOMGraph
    """
    if lic not in LICENSES:
        sbom_graph.custom_license_ids.add(lic)


def get_component_licenses(component):
    """
    Extract lists of all concluded and infoInFile licenses seen.

    Arguments:
        - component: SBOMComponent
    Returns: sorted list of concluded license exprs,
             sorted list of infoInFile ID's
    """
    lics_concluded = set()
    lics_from_files = set()
    for f in component.files.values():
        lics_concluded.add(f.concluded_license)
        for lic_info in f.license_info_in_file:
            lics_from_files.add(lic_info)
    return sorted(list(lics_concluded)), sorted(list(lics_from_files))


def normalize_expression(lics_concluded):
    """
    Combine array of license expressions into one AND'd expression,
    adding parens where needed.

    Arguments:
        - lics_concluded: array of license expressions
    Returns: string with single AND'd expression.
    """
    # return appropriate for simple cases
    if len(lics_concluded) == 0:
        return "NOASSERTION"
    if len(lics_concluded) == 1:
        return lics_concluded[0]

    # more than one, so we'll need to combine them
    # if and only if an expression has spaces, it needs parens
    revised = []
    for lic in lics_concluded:
        if lic in ["NONE", "NOASSERTION"]:
            continue
        if " " in lic:
            revised.append(f"({lic})")
        else:
            revised.append(lic)
    return " AND ".join(revised)


def get_reuse_info(project, file_path):
    """
    Retrieve SPDX license expressions and copyright notices for a file using
    the REUSE library.

    Arguments:
        - project: reuse.project.Project for the file's component, rooted at
          the component's base_dir so that REUSE.toml are found; may be None
        - file_path: path to file to scan

    Returns: (list of license expression strings, list of copyright strings)
    """
    if project is None:
        return [], []

    _logger.debug("  - getting REUSE info for %s", file_path)

    try:
        infos = project.reuse_info_of(file_path)
        licenses = []
        copyrights = []

        for info in infos:
            for expr in info.spdx_expressions:
                licenses.append(str(expr))
            for notice in info.copyright_notices:
                copyrights.append(str(notice))

        return licenses, copyrights
    except Exception:
        _logger.warning("Error getting REUSE info for %s", file_path, exc_info=True)
        return [], []


def scan_sbom_graph(cfg, sbom_graph):
    """
    Scan for licenses and calculate hashes for all Files and Components
    in this SBOM graph.

    Arguments:
        - cfg: ScannerConfig
        - sbom_graph: SBOMGraph
    """
    for component in sbom_graph.components.values():
        _logger.info("scanning files in component %s", component.name)

        # build the REUSE project once per component, rooted at the
        # component's own base_dir, and reuse it for all of its files
        try:
            reuse_project = Project.from_directory(str(component.base_dir))
        except Exception:
            _logger.warning(
                "Error building REUSE project for %s", component.base_dir, exc_info=True
            )
            reuse_project = None

        # first, gather File data for this component
        for f in component.files.values():
            # set relpath based on component's base_dir
            f.relative_path = os.path.relpath(f.path, component.base_dir)

            # get hashes for file
            hashes = get_hashes(f.path)
            if not hashes:
                _logger.warning("unable to get hashes for file %s; skipping", f.path)
                continue
            h_sha1, h_sha256, h_md5 = hashes
            f.hashes["SHA1"] = h_sha1
            if cfg.do_sha256:
                f.hashes["SHA256"] = h_sha256
            if cfg.do_md5:
                f.hashes["MD5"] = h_md5

            # get licenses for file
            reuse_licenses, copyrights = get_reuse_info(reuse_project, f.path)

            if reuse_licenses:
                if cfg.should_conclude_file_licenses:
                    f.concluded_license = " AND ".join(
                        f"({e})" if " " in e else e for e in reuse_licenses
                    )
                f.license_info_in_file = sorted(
                    {lic for expr in reuse_licenses for lic in split_expression(expr)}
                )

            if copyrights:
                f.copyright_text = "<text>\n" + "\n".join(copyrights) + "\n</text>"

            # check if any custom license IDs should be flagged for SBOM
            for lic in f.license_info_in_file:
                check_license_valid(lic, sbom_graph)

        # now, assemble the Component data
        lics_concluded, lics_from_files = get_component_licenses(component)
        if cfg.should_conclude_component_license:
            component.concluded_license = normalize_expression(lics_concluded)
        component.license_info_from_files = lics_from_files


# Backward compatibility alias (deprecated)
def scan_document(cfg, doc):
    """
    Deprecated: Use scan_sbom_graph instead.
    This function is kept for backward compatibility during migration.
    """
    # This would need to convert Document to SBOMGraph, but since we're
    # migrating away from Document, this should not be called in new code.
    raise NotImplementedError(
        "scan_document is deprecated. Use scan_sbom_graph with SBOMGraph instead."
    )
