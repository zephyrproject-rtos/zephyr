# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Measure and report documentation build time.

This Sphinx extension records the wall-clock duration of each build, caches it
(keyed by builder and active build profile) in the doctree directory, and prints
a summary at the end of every build. The cache persists across incremental
builds and is removed on a clean build, so the summary can show:

- the current build time and whether it was a full or incremental build,
- the duration of the previous build of the same profile,
- the speedup of an incremental build relative to the last full build.

This makes the benefit of incremental and partial (``html-quick``) builds
visible. The timer covers the Sphinx phase (including the Doxygen, Kconfig and
board-catalog generators that run at ``builder-inited``); it does not include
the devicetree generation done by CMake before Sphinx starts.
"""

import json
import time
from pathlib import Path
from typing import Any

from sphinx.application import Sphinx
from sphinx.util import logging

__version__ = "0.1.0"

logger = logging.getLogger(__name__)

CACHE_FILENAME = "build_times.json"

# Build profile tags that change what gets built, used to keep the cached
# timings of different profiles (e.g. html vs html-quick) from clobbering each
# other.
PROFILE_TAGS = (
    "skip_doxygen",
    "skip_kconfig",
    "skip_board_catalog",
    "skip_external_content",
)


def _profile_key(app: Sphinx) -> str:
    """Cache key identifying the builder and the active partial-build profile."""
    active = [tag for tag in PROFILE_TAGS if app.tags.has(tag)]
    return app.builder.name + (f" [{'+'.join(active)}]" if active else "")


def _cache_path(app: Sphinx) -> Path:
    return Path(app.doctreedir) / CACHE_FILENAME


def _load_cache(path: Path) -> dict:
    try:
        return json.loads(path.read_text())
    except (OSError, ValueError):
        return {}


def _fmt(seconds: float) -> str:
    if seconds >= 60:
        return f"{int(seconds // 60)}m{seconds % 60:04.1f}s"
    return f"{seconds:.1f}s"


def _on_builder_inited(app: Sphinx) -> None:
    app._build_timer_start = time.monotonic()
    # Default to "everything" so a build that never fires env-before-read-docs
    # (nothing to read) is still classified sensibly.
    app._build_timer_read_count = None


def _on_before_read_docs(app: Sphinx, env, docnames) -> None:
    # Number of documents that will be (re)read this run; a small fraction of
    # the total indicates an incremental build.
    app._build_timer_read_count = len(docnames)


def _on_build_finished(app: Sphinx, exception) -> None:
    if exception is not None:
        return

    start = getattr(app, "_build_timer_start", None)
    if start is None:
        return
    elapsed = time.monotonic() - start

    total_docs = len(app.env.found_docs)
    read_count = getattr(app, "_build_timer_read_count", None)
    if read_count is None:
        read_count = 0
    # A build that (re)read (almost) all documents is treated as a full build.
    is_full = 0 < total_docs <= read_count
    kind = "full" if is_full else "incremental"

    key = _profile_key(app)
    cache_path = _cache_path(app)
    cache = _load_cache(cache_path)
    entry = cache.get(key, {})
    prev = entry.get("last")
    full_ref = entry.get("full")

    lines = [
        f"Documentation build summary - {key}",
        f"  Build time:       {_fmt(elapsed)}  ({kind})",
        f"  Documents read:   {read_count} of {total_docs}",
    ]
    if prev is not None:
        lines.append(f"  Previous build:   {_fmt(prev['seconds'])}  ({prev['kind']})")
    if full_ref is not None and not is_full and elapsed > 0:
        speedup = full_ref["seconds"] / elapsed
        lines.append(f"  Full build ref:   {_fmt(full_ref['seconds'])}  ->  {speedup:.1f}x faster")

    # Persist the updated timings.
    entry["last"] = {"seconds": elapsed, "kind": kind}
    if is_full:
        entry["full"] = {"seconds": elapsed, "kind": kind}
    cache[key] = entry
    try:
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        cache_path.write_text(json.dumps(cache, indent=2))
    except OSError as e:
        logger.warning(f"build_timer: could not write timing cache: {e}")

    # Print directly so the summary is shown regardless of Sphinx verbosity.
    width = max(len(line) for line in lines) + 2
    bar = "=" * width
    print("\n" + bar)
    for line in lines:
        print(" " + line)
    print(bar)


def setup(app: Sphinx) -> dict[str, Any]:
    # Run first at builder-inited (low priority value) so the timer starts
    # before the Doxygen/Kconfig/board-catalog generators, and last at
    # build-finished (high priority value) so it captures the whole build.
    app.connect("builder-inited", _on_builder_inited, priority=100)
    app.connect("env-before-read-docs", _on_before_read_docs)
    app.connect("build-finished", _on_build_finished, priority=900)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
