# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import re

# Regex patterns for external reference validation
CPE23TYPE_REGEX = (
    r'^cpe:2\.3:[aho\*\-](:(((\?*|\*?)([a-zA-Z0-9\-\._]|(\\[\\\*\?!"#$$%&\'\(\)\+,\/:;<=>@\[\]\^'
    r"`\{\|}~]))+(\?*|\*?))|[\*\-])){5}(:(([a-zA-Z]{2,3}(-([a-zA-Z]{2}|[0-9]{3}))?)|[\*\-]))(:(((\?*"
    r'|\*?)([a-zA-Z0-9\-\._]|(\\[\\\*\?!"#$$%&\'\(\)\+,\/:;<=>@\[\]\^`\{\|}~]))+(\?*|\*?))|[\*\-])){4}$'
)
PURL_REGEX = r"^pkg:.+(\/.+)?\/.+(@.+)?(\?.+)?(#.+)?$"


def normalize_spdx_name(name: str) -> str:
    """Replace '_' by '-' since it's not allowed in SPDX ID."""
    return name.replace("_", "-")


# west suffixes a revision it could not confirm: "-dirty" for a modified tree,
# "-off" for a checkout that is not on the manifest revision.
_REVISION_SUFFIX_RE = re.compile(r'[+\-](dirty|off).*$')
_COMMIT_RE = re.compile(r'^[a-f0-9]{40}$')


def resolvable_revision(revision: str) -> str:
    """Return the part of *revision* something can actually fetch.

    A suffixed revision still names the commit it was taken from, but is
    neither a git object name nor a package version, so anything meant to be
    resolved needs the commit alone. The suffix is worth keeping where it is
    read rather than followed, which is why packageVersion holds the original.
    """
    if not revision:
        return revision
    clean = _REVISION_SUFFIX_RE.sub('', revision).strip()
    return clean if _COMMIT_RE.match(clean) else revision


def generate_download_url(url: str, revision: str) -> str:
    """Generate download URL with revision if available."""
    if not revision:
        return url
    return f'git+{url}@{resolvable_revision(revision)}'


def get_standard_licenses() -> set:
    """Get set of standard SPDX license IDs."""
    # Import here to avoid circular dependency
    from zspdx.licenses import LICENSES

    return set(LICENSES)
