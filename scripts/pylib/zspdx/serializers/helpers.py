# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

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


def generate_download_url(url: str, revision: str) -> str:
    """Generate download URL with revision if available."""
    if not revision:
        return url
    # Strip git-describe suffixes (-dirty, -off, …) so the ref is a valid git
    # object name. packageVersion retains the original string for dirty tracking.
    import re
    clean = re.sub(r'[+\-](dirty|off).*$', '', revision).strip()
    ref = clean if re.match(r'^[a-f0-9]{40}$', clean) else revision
    return f'git+{url}@{ref}'


def get_standard_licenses() -> set:
    """Get set of standard SPDX license IDs."""
    # Import here to avoid circular dependency
    from zspdx.licenses import LICENSES

    return set(LICENSES)
