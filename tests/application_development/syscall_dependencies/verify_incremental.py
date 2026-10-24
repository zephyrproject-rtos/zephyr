# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check syscall generation after header edits and directory additions/removals."""

import subprocess
import sys
from pathlib import Path


def rebuild(build_dir):
    subprocess.run(
        ["cmake", "--build", str(build_dir), "--target", "syscall_list_h_target"], check=True
    )
    return (build_dir / "zephyr/include/generated/zephyr/syscall_list.h").read_text()


def main():
    build_dir, include_dir = map(Path, sys.argv[1:])
    header = include_dir / "dependency.h"
    original = header.read_text()

    assert "K_SYSCALL_DEPENDENCY_INITIAL" in rebuild(build_dir)

    header.write_text(original.replace("dependency_initial", "dependency_edited"))
    generated = rebuild(build_dir)
    assert "K_SYSCALL_DEPENDENCY_EDITED" in generated, "Header edit was not detected"
    assert "K_SYSCALL_DEPENDENCY_INITIAL" not in generated

    nested = include_dir / "nested"
    nested.mkdir()
    added = nested / "added.h"
    added.write_text(original.replace("dependency_initial", "dependency_added"))
    assert "K_SYSCALL_DEPENDENCY_ADDED" in rebuild(build_dir), "New header was not detected"

    added.unlink()
    nested.rmdir()
    assert "K_SYSCALL_DEPENDENCY_ADDED" not in rebuild(build_dir), "Removed header is still listed"

    trigger = build_dir / "zephyr/misc/generated/syscalls_subdirs.trigger"
    timestamp = trigger.stat().st_mtime_ns
    rebuild(build_dir)
    assert trigger.stat().st_mtime_ns == timestamp, "Unchanged build reran syscall discovery"

    header.write_text(original)
    assert "K_SYSCALL_DEPENDENCY_INITIAL" in rebuild(build_dir)


if __name__ == "__main__":
    main()
