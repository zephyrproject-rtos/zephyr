#!/usr/bin/env python3

# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

import argparse
import asyncio
import contextlib
import dataclasses
import json
import logging
import os
import re
import shlex
import sys
import tempfile
from collections.abc import AsyncIterator
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path

logging.basicConfig()
logger = logging.getLogger(__name__)

# Match C23 attributes with the zephyr namespace, for example:
# [[zephyr::func("syscall", "WEST_TOPDIR/zephyr/include/zephyr/kernel.h", 2174)]]
ANNOTATION = re.compile(
    r'''
    \[\[\s*zephyr::             # Attribure start
    (?P<attr>[a-zA-Z0-9_]+)     # Attribute name like 'func' or 'struct'
    (?:\((?P<args>.+)\))?       # Optional attribute arguments inside paranthesis
    \s*\]\]                     # Attribute end
    ''',
    re.VERBOSE,
)
ANNOTATION_ARG = re.compile(
    r'''
    \s*
    (?:"(?P<s>[^"]*)")          # A string argument inside double quotes
    |                           # Or
    (?P<i>\d+)                  # An integer argument
    \s*,?                       # Optional comma
    ''',
    re.VERBOSE,
)

ANNOTATION_FUNC = re.compile(
    r'''
    \s*(?P<type>.*?)            # Lazy regex for the function return type
    \s*(?P<name>[a-zA-Z0-9_]+)  # Eager regex for the function name
    \s*[(](?P<args>[^)]*)[)]    # Function arguments
    ''',
    re.VERBOSE,
)
ANNOTATION_STRUCT = re.compile(
    r'''
    \s*struct\s+                # struct keyword
    (?P<name>[a-zA-Z0-9_]+)     # The name of the struct
    ''',
    re.VERBOSE,
)

ZPP_ARGS = [
    "-E",  # Only run preprocessor
    "-P",  # No comment directives
    "-D__ZPP__",  # ZPP define
]


class AnnotationType(str, Enum):
    FUNCTION = "func"
    STRUCT = "struct"

    @classmethod
    def has_value(cls, value: str) -> bool:
        try:
            cls(value)
        except ValueError:
            return False
        return True


@dataclass
class Annotation:
    attr: AnnotationType
    args: list[str | int]
    data: dict[str, str] = field(default_factory=dict)

    def __hash__(self) -> int:
        return json.dumps(dataclasses.asdict(self)).__hash__()


@dataclass
class CompileCommand:
    command: str
    directory: Path
    file: Path


async def tee(cmd: list[str], file: Path | None) -> AsyncIterator[str]:
    """
    Execute a command and yield the output for each line.
    Optionally write the output to a file as well.
    """
    with open(file, "wb") if file else contextlib.nullcontext() as out:
        process = await asyncio.create_subprocess_exec(
            *cmd,
            limit=1024 * 1024,  # Some processes get stuck on low limits
            stdout=asyncio.subprocess.PIPE,
        )
        assert process.stdout is not None
        async for line in process.stdout:
            if out:
                out.write(line)
            yield line.decode()
        await process.wait()


async def search_chainable(
    pattern: str | re.Pattern[str], input: str, gen: AsyncIterator[str]
) -> re.Match[str] | None:
    """
    Helper function to search a regex pattern from input, if nothing is
    found we add data from the generator and try again.

    This can be used if a pattern is split over multiple lines.
    """
    while True:
        match = re.search(pattern, input)
        if match is not None:
            return match

        add = await anext(gen)
        if add is None:
            return None

        input += add


async def process_command(
    cmd: CompileCommand, tmpdir: Path
) -> tuple[set[Annotation], list[str]] | None:
    # Only parse c files
    if cmd.file.suffix != ".c":
        logger.info("SKIP(filetype): %s", cmd.file)
        return None
    if not cmd.file.exists():
        logger.info("SKIP(missing): %s", cmd.file)
        return None

    # Use the argument parser to drop the output argument and keep everything else
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument("-o", "--output")
    parser.add_argument("-DZPP", action="store_true", dest="zpp")
    command_args, command_remaining = parser.parse_known_args(
        shlex.split(cmd.command, posix=os.name != "nt")
    )

    if not command_args.zpp:
        logger.info("SKIP(nozpp): %s", cmd.file)
        return None

    result = set()
    dependencies = []
    logger.info(f"ZPP: {cmd.file}")

    output = tee(
        command_remaining + ZPP_ARGS + ["-M", "-MG"],
        cmd.directory / f"{command_args.output}.zpp.d" if args.intermediate else None,
    )

    # First run, collect includes and create temporary empty header files for missing ones
    async for line in output:
        includes = line.split(":", 1)[-1].strip(' \t\n\r\\').split()
        for include in includes:
            hdr = Path(include)
            if not hdr.is_absolute():
                # A missing header file needs to exist, create an empty dummy
                abs_hdr = tmpdir / hdr
                abs_hdr.parent.mkdir(parents=True, exist_ok=True)
                abs_hdr.touch()
            elif not hdr.resolve().is_relative_to(cmd.directory):
                # Do not add files that are in the build/output directory
                dependencies.append(str(hdr.resolve()))

    # Second run, actual preprocessor call
    output = tee(
        command_remaining + ZPP_ARGS + [f"-I{tmpdir}"],
        cmd.directory / f"{command_args.output}.zpp.i" if args.intermediate else None,
    )

    async for line in output:
        match = re.search(ANNOTATION, line)
        if match is None:
            continue

        if not AnnotationType.has_value(match.group("attr")):
            continue

        # remove the annotation from the line itself
        line = line.replace(match.group(0), "")

        match_args = []
        if match.group("args") is not None:
            # split into string or int arguments
            for m in re.finditer(ANNOTATION_ARG, match.group("args")):
                match_args.append(m.group("s") or int(m.group("i")))

        annotation = Annotation(
            attr=AnnotationType(match.group("attr")),
            args=match_args,
        )

        match annotation.attr:
            case AnnotationType.FUNCTION:
                match_func = await search_chainable(ANNOTATION_FUNC, line, output)
                assert match_func is not None

                annotation.data["type"] = match_func.group("type")
                annotation.data["name"] = match_func.group("name")
                annotation.data["args"] = match_func.group("args")
            case AnnotationType.STRUCT:
                match_struct = await search_chainable(ANNOTATION_STRUCT, line, output)
                assert match_struct is not None

                annotation.data["name"] = match_struct.group("name")

        result.add(annotation)

    logger.debug("annotations %s", result)
    logger.debug("dependencies %s", dependencies)

    return result, dependencies


async def process_command_with_sem(
    cmd: CompileCommand, sem: asyncio.Semaphore
) -> tuple[set[Annotation], list[str]] | None:
    async with sem:
        with tempfile.TemporaryDirectory() as tmpdir:
            return await process_command(cmd, Path(tmpdir))


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbosity",
        action="count",
        help="print more diagnostic messages (option can be given multiple times)",
        default=0,
    )
    parser.add_argument(
        "-i",
        "--intermediate",
        action="store_true",
        help="Store intermediate files",
    )
    parser.add_argument(
        '-j',
        '--jobs',
        nargs='?',
        const=-1,
        default=1,
        type=int,
        action='store',
        help='''Use multiple jobs to parallelize commands.
                Pass no number or -1 to run commands on all cores.''',
    )
    parser.add_argument("-d", "--depfile", action="store", help="The depfile output file")
    parser.add_argument("-o", "--output", action="store", help="The json output file")
    parser.add_argument(
        "database",
        help="Database file, typically compile_commands.json in the build directory.",
    )

    args = parser.parse_args()


async def main():
    parse_args()

    levels = [logging.WARNING, logging.INFO, logging.DEBUG]
    logger.setLevel(levels[args.verbosity])
    logger.info(f'ZPP_ARGS: {" ".join(ZPP_ARGS)}')

    with open(args.database) as fp:
        db_json = json.load(fp)

    jobs = args.jobs if args.jobs > 0 else os.cpu_count() or sys.maxsize
    sem = asyncio.Semaphore(jobs)

    # Collect all targets the we depend on
    deps = set()
    result = set()

    cmds = [
        CompileCommand(
            command=item.get("command"),
            directory=Path(item.get("directory")),
            file=Path(item.get("file")),
        )
        for item in db_json
    ]

    annotations = await asyncio.gather(*[process_command_with_sem(cmd, sem) for cmd in cmds])

    for a in annotations:
        if a is None:
            continue

        result.update(a[0])
        deps.update(a[1])

    if args.output:
        with open(args.output, "w") as out:
            json.dump([dataclasses.asdict(obj) for obj in result], out, indent=2)

        if args.depfile:
            with open(args.depfile, "w") as out:
                out.write(f"{args.output}: \\\n ")
                out.write(" \\\n ".join(filter(None, sorted(deps))))


if __name__ == "__main__":
    asyncio.run(main())
