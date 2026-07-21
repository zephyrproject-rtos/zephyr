# Copyright (c) 2026 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

"""
Build an index associating device driver APIs, their operations, the drivers
implementing them, and the devicetree compatibles those drivers bind to.

The index is derived purely from declaration-site information, so it does not
depend on a board, a build configuration, or a Doxygen run:

- ``struct <class>_driver_api`` definitions in ``include/`` describe the API
  classes and their operations. When the struct carries the ``@driver_ops``
  Doxygen tags, each operation is additionally known to be mandatory or
  optional.
- ``DEVICE_API(<class>, <name>) = { ... }`` in driver sources tells which API
  class a driver implements, and which of its operations it actually sets.
- ``#define DT_DRV_COMPAT <compatible>`` ties a driver source to the
  devicetree compatibles it handles.

Use it as a library through get_driver_api_index(), or from the command line::

    python3 driver_api_index.py --api spi
    python3 driver_api_index.py --compatible nordic,nrf-spim
"""

import argparse
import json
import logging
import os
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

ZEPHYR_BASE = Path(__file__).parents[2]

# Folders scanned for driver implementations. Same set as the compatible ->
# driver source heuristic that this module supersedes.
DEFAULT_ROOTS = ("boards", "drivers", "modules", "soc", "subsys")

# Folders scanned for driver API class definitions.
API_ROOTS = ("include",)

logger = logging.getLogger(__name__)

_STRUCT_RE = re.compile(r"\bstruct\s+([A-Za-z_]\w*)_driver_api\s*\{")
_EXTENDS_RE = re.compile(r"\bDEVICE_API_EXTENDS\(\s*(\w+)\s*,\s*(\w+)\s*,")
_DRIVER_OPS_RE = re.compile(r"@driver_ops\{([^}]*)\}")
_BACKEND_GROUP_RE = re.compile(r"@def_driverbackendgroup\{([^,}]*),([^}]*)\}")
_DT_DRV_COMPAT_RE = re.compile(r"^\s*#\s*define\s+DT_DRV_COMPAT\s+(\S+)", re.MULTILINE)
_DEVICE_API_RE = re.compile(r"\bDEVICE_API\(\s*(\w+)\s*,\s*(\w+)\s*\)")
# Deliberately unanchored, so that the wrappers around it also count, e.g.
# SENSOR_DEVICE_DT_INST_DEFINE() which most sensor drivers use.
_DEVICE_DT_INST_DEFINE_RE = re.compile(r"DEVICE_DT_INST_DEFINE")
# A designator can reach into a nested struct, as drivers extending another API
# do (".i2c_api.transfer = ..."). Only the leading component is a member of the
# API struct being initialized, so the whole path has to be consumed at once.
_DESIGNATED_INIT_RE = re.compile(r"\.\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*|\.\s*[A-Za-z_]\w*\s*)*=")
_CONDITIONAL_RE = re.compile(r"#\s*if|\bIF_ENABLED\b|\bCOND_CODE_[01]\b")
_BINDING_COMPATIBLE_RE = re.compile(r"^compatible:\s*[\"']?([^\"'\n#]+)", re.MULTILINE)

# Struct members come in two flavours: a plain declaration using a typedef'd
# operation type ("spi_api_io transceive;") and a raw function pointer
# ("int (*pin_configure)(const struct device *port, ...);"). Both are needed:
# headers documented with @driver_ops use typedefs, but many others don't.
_MEMBER_RE = re.compile(
    r"\(\s*\*\s*(?P<fp>[A-Za-z_]\w*)\s*\)\s*\("
    r"|(?P<plain>[A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*;"
)

_PREPROCESSOR_RE = re.compile(
    r"^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b[ \t]*(.*)$", re.MULTILINE
)
# __DOXYGEN__ only exists to make Doxygen see conditionally compiled members;
# it says nothing about when an operation is actually available.
_DOXYGEN_TERM_RE = re.compile(r"\s*(\|\|)?\s*defined\(__DOXYGEN__\)\s*(\|\|)?\s*")

MANDATORY = "mandatory"
OPTIONAL = "optional"
UNKNOWN = "unknown"


def normalize_compatible(compatible: str) -> str:
    """Turn a devicetree compatible into its DT_DRV_COMPAT spelling."""
    return re.sub("[-,.@/+]", "_", compatible.lower())


@dataclass
class DriverApi:
    """A device driver API class, e.g. 'spi'."""

    cls: str
    header: str
    name: str | None = None
    doxygen_group: str | None = None
    extends: str | None = None
    ops: dict[str, str] = field(default_factory=dict)
    # Ops declared inside a preprocessor guard, mapped to that guard. Such an
    # op only exists - and is only mandatory - when the guard holds.
    op_conditions: dict[str, str] = field(default_factory=dict)

    @property
    def documented(self) -> bool:
        """Whether the API struct is annotated with the @driver_ops tags."""
        return self.name is not None

    def mandatory_ops(self, unconditional_only: bool = False) -> list[str]:
        return [
            op
            for op, kind in self.ops.items()
            if kind == MANDATORY and not (unconditional_only and op in self.op_conditions)
        ]

    def optional_ops(self) -> list[str]:
        return [op for op, kind in self.ops.items() if kind == OPTIONAL]

    def as_dict(self) -> dict:
        return {
            "name": self.name,
            "doxygen_group": self.doxygen_group,
            "header": self.header,
            "documented": self.documented,
            "extends": self.extends,
            "ops": self.ops,
            "op_conditions": self.op_conditions,
        }


@dataclass
class DriverImpl:
    """One DEVICE_API() instance, i.e. one driver implementing one API class."""

    api: str
    instance: str
    source: str
    compatibles: list[str] = field(default_factory=list)
    ops: list[str] = field(default_factory=list)
    conditional_ops: bool = False

    def as_dict(self) -> dict:
        return {
            "api": self.api,
            "instance": self.instance,
            "source": self.source,
            "compatibles": self.compatibles,
            "ops": self.ops,
            "conditional_ops": self.conditional_ops,
        }


@dataclass
class DriverApiIndex:
    """The complete driver API index."""

    apis: dict[str, DriverApi] = field(default_factory=dict)
    implementations: list[DriverImpl] = field(default_factory=list)
    # Normalized compatible -> driver source path, relative to ZEPHYR_BASE.
    driver_sources: dict[str, Path] = field(default_factory=dict)
    # Normalized compatible -> devicetree compatible, from dts/bindings.
    known_compatibles: dict[str, str] = field(default_factory=dict)

    def impls_for_api(self, cls: str) -> list[DriverImpl]:
        return [impl for impl in self.implementations if impl.api == cls]

    def impls_for_compatible(self, compatible: str) -> list[DriverImpl]:
        norm = normalize_compatible(compatible)
        return [impl for impl in self.implementations if norm in impl.compatibles]

    def impls_for_source(self, source: str) -> list[DriverImpl]:
        path = Path(source)
        if path.is_absolute():
            try:
                path = path.resolve().relative_to(ZEPHYR_BASE.resolve())
            except ValueError:
                return []
        source = path.as_posix().removeprefix("./")
        return [impl for impl in self.implementations if impl.source == source]

    def compatible_name(self, norm: str) -> str:
        """Best known devicetree spelling for a normalized compatible."""
        return self.known_compatibles.get(norm, norm)

    def compatibles(self) -> dict[str, dict]:
        """Normalized compatible -> the APIs it implements and its source."""
        result: dict[str, dict] = {}
        for impl in self.implementations:
            for norm in impl.compatibles:
                entry = result.setdefault(
                    self.compatible_name(norm),
                    {"apis": [], "source": None},
                )
                if impl.api not in entry["apis"]:
                    entry["apis"].append(impl.api)
        for norm, path in self.driver_sources.items():
            name = self.compatible_name(norm)
            if name in result:
                result[name]["source"] = path.as_posix()
        for entry in result.values():
            entry["apis"].sort()
        return dict(sorted(result.items()))

    def as_dict(self) -> dict:
        return {
            "apis": {cls: api.as_dict() for cls, api in sorted(self.apis.items())},
            "implementations": [
                impl.as_dict()
                for impl in sorted(self.implementations, key=lambda i: (i.api, i.source))
            ],
            "compatibles": self.compatibles(),
        }


_MASK_RE = re.compile(
    r"/\*.*?\*/"  # block comment
    r"|//[^\n]*"  # line comment
    r"|\"(?:\\.|[^\"\\\n])*\""  # string literal
    r"|'(?:\\.|[^'\\\n])*'",  # character literal
    re.DOTALL,
)
_NOT_NEWLINE_RE = re.compile(r"[^\n]")


def _blank_comments_and_strings(text: str) -> str:
    """Return text with comment and string contents replaced by spaces.

    The result has the same length as the input, so offsets remain valid. It is
    used for all structural scanning (brace matching, macro lookup) so that
    braces or keywords appearing inside comments and literals are ignored.
    """
    return _MASK_RE.sub(lambda m: _NOT_NEWLINE_RE.sub(" ", m.group(0)), text)


def _match_block(masked: str, open_idx: int) -> int | None:
    """Index of the '}' matching the '{' at open_idx, or None if unbalanced."""
    depth = 0
    for i in range(open_idx, len(masked)):
        c = masked[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
    return None


def _top_level_spans(masked: str, start: int, end: int) -> list[tuple[int, int]]:
    """Spans of [start, end) that are not nested inside a '{ }' block."""
    spans = []
    depth = 0
    span_start = start
    for i in range(start, end):
        c = masked[i]
        if c == "{":
            if depth == 0:
                spans.append((span_start, i))
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                span_start = i + 1
    if depth == 0 and span_start < end:
        spans.append((span_start, end))
    return spans


def _preceding_doc(text: str, pos: int) -> str:
    """The '/** ... */' comment immediately preceding pos, if any."""
    end = text.rfind("*/", 0, pos)
    if end == -1:
        return ""
    # A brace or a semicolon in between means the comment documents something
    # else - typically the enclosing struct, or the previous member.
    if any(c in text[end:pos] for c in ";{}"):
        return ""
    start = text.rfind("/**", 0, end)
    if start == -1:
        return ""
    return text[start : end + 2]


def _format_condition(kind: str, expression: str) -> str:
    expression = _DOXYGEN_TERM_RE.sub("", expression).strip()
    if kind == "ifdef":
        return expression
    if kind == "ifndef":
        return f"!{expression}"
    # "defined(CONFIG_FOO)" alone reads better as just "CONFIG_FOO".
    if match := re.fullmatch(r"defined\s*\(\s*(\w+)\s*\)", expression):
        return match.group(1)
    return expression


def _condition_marks(masked: str, start: int, end: int) -> list[tuple[int, str | None]]:
    """Preprocessor guard in effect, as (offset, condition) boundaries."""
    stack: list[str] = []
    marks: list[tuple[int, str | None]] = [(start, None)]
    for match in _PREPROCESSOR_RE.finditer(masked, start, end):
        kind, expression = match.group(1), match.group(2).strip()
        if kind in ("if", "ifdef", "ifndef"):
            stack.append(_format_condition(kind, expression))
        elif kind == "elif" and stack:
            stack[-1] = _format_condition("if", expression)
        elif kind == "else" and stack:
            stack[-1] = f"!({stack[-1]})"
        elif kind == "endif" and stack:
            stack.pop()
        marks.append((match.end(), " && ".join(c for c in stack if c) or None))
    return marks


def _condition_at(marks: list[tuple[int, str | None]], pos: int) -> str | None:
    condition = None
    for offset, value in marks:
        if offset > pos:
            break
        condition = value
    return condition


def _op_kind(doc: str) -> str:
    if "driver_ops_mandatory" in doc:
        return MANDATORY
    if "driver_ops_optional" in doc:
        return OPTIONAL
    return UNKNOWN


def _iter_sources(roots, base: Path, suffixes=(".c", ".h")):
    for root in roots:
        for dirpath, dirnames, filenames in os.walk(base / root):
            dirnames.sort()
            for filename in sorted(filenames):
                if filename.endswith(suffixes):
                    yield Path(dirpath) / filename


def _read(path: Path) -> str | None:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as e:
        logger.warning("skipping %s: %s", path, e)
        return None


def _parse_api_header(path: Path, text: str, base: Path) -> list[DriverApi]:
    masked = _blank_comments_and_strings(text)
    relative = path.relative_to(base).as_posix()
    groups = list(_BACKEND_GROUP_RE.finditer(text))
    apis = []

    for match in _STRUCT_RE.finditer(masked):
        open_idx = match.end() - 1
        close_idx = _match_block(masked, open_idx)
        if close_idx is None:
            logger.warning("unbalanced braces in %s for %s", relative, match.group(1))
            continue

        api = DriverApi(cls=match.group(1), header=relative)

        struct_doc = _preceding_doc(text, match.start())
        if name := _DRIVER_OPS_RE.search(struct_doc):
            api.name = name.group(1).strip()

        # Nearest @def_driverbackendgroup before the struct, else the first one
        # in the file. Both spellings appear, with and without a space.
        before = [g for g in groups if g.start() < match.start()]
        if group := (before[-1] if before else (groups[0] if groups else None)):
            api.doxygen_group = group.group(2).strip()

        marks = _condition_marks(masked, open_idx + 1, close_idx)
        for span_start, span_end in _top_level_spans(masked, open_idx + 1, close_idx):
            for member in _MEMBER_RE.finditer(masked, span_start, span_end):
                op = member.group("fp") or member.group("plain")
                api.ops[op] = _op_kind(_preceding_doc(text, member.start()))
                if condition := _condition_at(marks, member.start()):
                    api.op_conditions[op] = condition

        apis.append(api)

    return apis


def _parse_driver_source(path: Path, text: str, base: Path):
    """Return (compatibles, has_device_dt_inst_define, implementations)."""
    masked = _blank_comments_and_strings(text)
    relative = path.relative_to(base).as_posix()

    compatibles = sorted({m.group(1) for m in _DT_DRV_COMPAT_RE.finditer(masked)})
    impls = []

    for match in _DEVICE_API_RE.finditer(masked):
        impl = DriverImpl(
            api=match.group(1),
            instance=match.group(2),
            source=relative,
            # A copy: several instances in one file must not share a list.
            compatibles=list(compatibles),
        )

        # The API struct is not always initialized in place; some drivers only
        # declare it here and populate it elsewhere.
        rest = masked[match.end() : match.end() + 64]
        if assign := re.match(r"\s*=\s*\{", rest):
            open_idx = match.end() + assign.end() - 1
            close_idx = _match_block(masked, open_idx)
            if close_idx is None:
                logger.warning("unbalanced braces in %s for %s", relative, impl.instance)
            else:
                body = text[open_idx + 1 : close_idx]
                impl.conditional_ops = bool(_CONDITIONAL_RE.search(body))
                for span_start, span_end in _top_level_spans(masked, open_idx + 1, close_idx):
                    for init in _DESIGNATED_INIT_RE.finditer(masked, span_start, span_end):
                        if init.group(1) not in impl.ops:
                            impl.ops.append(init.group(1))

        impls.append(impl)

    return compatibles, bool(_DEVICE_DT_INST_DEFINE_RE.search(masked)), impls


def load_driver_sources(base: Path = ZEPHYR_BASE, roots=DEFAULT_ROOTS) -> dict[str, Path]:
    """Map normalized compatibles to the source file implementing them.

    A file is considered the driver source for a compatible if:

    - it contains both a "#define DT_DRV_COMPAT <compatible>" and a
      DEVICE_DT_INST_DEFINE() call, and is the only such file, or
    - it is the only file in the tree defining DT_DRV_COMPAT for it.
    """
    return get_driver_api_index(base=base, roots=roots, bindings=False).driver_sources


def get_driver_api_index(
    base: Path = ZEPHYR_BASE,
    roots=DEFAULT_ROOTS,
    api_roots=API_ROOTS,
    bindings: bool = True,
) -> DriverApiIndex:
    """Scan the tree and build the driver API index."""
    index = DriverApiIndex()

    for path in _iter_sources(api_roots, base, suffixes=(".h",)):
        if (text := _read(path)) is None:
            continue
        for api in _parse_api_header(path, text, base):
            index.apis[api.cls] = api
        for child, parent in _EXTENDS_RE.findall(_blank_comments_and_strings(text)):
            if child in index.apis:
                index.apis[child].extends = parent

    # Candidates for the "defines DT_DRV_COMPAT and instantiates devices"
    # rule, and all DT_DRV_COMPAT sightings for the single-occurrence fallback.
    definers: dict[str, Path | None] = {}
    occurrences: dict[str, list[Path]] = defaultdict(list)

    for path in _iter_sources(roots, base):
        if (text := _read(path)) is None:
            continue
        compatibles, instantiates, impls = _parse_driver_source(path, text, base)
        index.implementations.extend(impls)

        relative = path.relative_to(base)
        for compatible in compatibles:
            occurrences[compatible].append(relative)
            if instantiates:
                # None marks an ambiguous compatible, defined by several files.
                definers[compatible] = None if compatible in definers else relative

    index.driver_sources = {c: p for c, p in definers.items() if p is not None}
    for compatible, paths in occurrences.items():
        if compatible in index.driver_sources or len(paths) != 1:
            continue
        path = paths[0]
        # Headers don't implement anything; point at the enclosing folder.
        index.driver_sources[compatible] = path.parent if path.suffix == ".h" else path

    if bindings:
        index.known_compatibles = load_known_compatibles(base)

    return index


def load_known_compatibles(base: Path = ZEPHYR_BASE) -> dict[str, str]:
    """Map normalized compatibles to their devicetree spelling."""
    known = {}
    for path in sorted((base / "dts" / "bindings").rglob("*.yaml")):
        if (text := _read(path)) is None:
            continue
        if match := _BINDING_COMPATIBLE_RE.search(text):
            compatible = match.group(1).strip()
            known[normalize_compatible(compatible)] = compatible
    return known


def _format_ops(impl: DriverImpl, api: DriverApi | None) -> str:
    """Implemented ops, with unknown-to-the-API ones flagged with a '?'."""
    known = api.ops if api else {}
    return ", ".join(op if op in known else f"{op}?" for op in impl.ops)


def _condition_suffix(api: DriverApi, op: str) -> str:
    condition = api.op_conditions.get(op)
    return f"  [only if {condition}]" if condition else ""


def _print_table(headers: list[str], rows: list[list[str]]) -> None:
    if not rows:
        print("(none)")
        return
    widths = [max(len(str(r[i])) for r in [headers, *rows]) for i in range(len(headers))]
    for row in [headers, ["-" * w for w in widths], *rows]:
        print("  ".join(str(c).ljust(w) for c, w in zip(row, widths, strict=True)).rstrip())


def _cmd_list_apis(index: DriverApiIndex) -> None:
    counts: dict[str, int] = defaultdict(int)
    for impl in index.implementations:
        counts[impl.api] += 1

    rows = []
    for cls, api in sorted(index.apis.items()):
        rows.append(
            [
                cls,
                api.name or "-",
                "yes" if api.documented else "no",
                str(len(api.ops)),
                str(counts[cls]),
                api.doxygen_group or "-",
            ]
        )
    _print_table(["CLASS", "NAME", "DOCUMENTED", "OPS", "IMPLS", "DOXYGEN GROUP"], rows)
    documented = sum(1 for api in index.apis.values() if api.documented)
    print(f"\n{len(index.apis)} API classes, {documented} documented with @driver_ops")


def _cmd_api(index: DriverApiIndex, cls: str, op: str | None) -> int:
    api = index.apis.get(cls)
    if api is None:
        print(f"error: unknown API class '{cls}' (see --list-apis)", file=sys.stderr)
        return 1

    print(f"{api.name or cls} API ({cls}) - {api.header}")
    if api.extends:
        print(f"extends: {api.extends}")
    if api.documented:
        print("operations:")
        for name, kind in api.ops.items():
            label = "REQ" if kind == MANDATORY else "OPT" if kind == OPTIONAL else "   "
            print(f"  {label} {name}{_condition_suffix(api, name)}")
    else:
        print(f"operations (not documented with @driver_ops): {', '.join(api.ops)}")
    print()

    impls = index.impls_for_api(cls)
    if op:
        impls = [impl for impl in impls if op in impl.ops]
        print(f"implementations providing '{op}':")

    # Mandatory ops are implemented by everyone, so listing them per driver is
    # noise; the optional ones are what tells implementations apart.
    optional = set(api.optional_ops())
    rows = []
    for impl in sorted(impls, key=lambda i: i.source):
        compatibles = ", ".join(index.compatible_name(c) for c in impl.compatibles) or "-"
        if api.documented:
            ops = ", ".join(name for name in impl.ops if name in optional) or "-"
        else:
            ops = _format_ops(impl, api)
        rows.append([compatibles, impl.source, ops])
    ops_header = "OPTIONAL OPS IMPLEMENTED" if api.documented else "IMPLEMENTED OPS"
    _print_table(["COMPATIBLE", "SOURCE", ops_header], rows)

    compatibles = {c for impl in impls for c in impl.compatibles}
    print(f"\n{len(impls)} implementations, {len(compatibles)} compatibles")
    if not op and api.documented:
        for name in api.optional_ops():
            count = sum(1 for impl in index.impls_for_api(cls) if name in impl.ops)
            print(f"  {count} implement optional '{name}'")
    return 0


def _print_impls(index: DriverApiIndex, impls: list[DriverImpl]) -> None:
    for impl in sorted(impls, key=lambda i: (i.api, i.source)):
        api = index.apis.get(impl.api)
        print(f"{api.name or impl.api if api else impl.api} API ({impl.api})")
        print(f"  source:   {impl.source}")
        print(f"  instance: {impl.instance}")
        if impl.compatibles:
            print(f"  binds:    {', '.join(index.compatible_name(c) for c in impl.compatibles)}")
        if api and api.documented:
            for name, kind in api.ops.items():
                mark = "x" if name in impl.ops else " "
                label = "REQ" if kind == MANDATORY else "OPT"
                print(f"  [{mark}] {label} {name}{_condition_suffix(api, name)}")
            for name in impl.ops:
                if name not in api.ops:
                    print(f"  [x]     {name} (not an operation of the API struct)")
        else:
            print(f"  ops:      {_format_ops(impl, api)}")
        if impl.conditional_ops:
            print("  note:     some operations are set conditionally (Kconfig-dependent)")
        print()


def _cmd_compatible(index: DriverApiIndex, compatible: str) -> int:
    norm = normalize_compatible(compatible)
    impls = index.impls_for_compatible(norm)
    # Echo back what the user typed when no binding gives a better spelling.
    name = index.known_compatibles.get(norm, compatible)

    if not impls:
        source = index.driver_sources.get(norm)
        known = norm in index.known_compatibles
        print(f"{name}: no driver API implementation found")
        if source:
            print(f"  likely driver source: {source.as_posix()}")
        if not known and index.known_compatibles:
            print("  note: no devicetree binding declares this compatible")
        return 0

    print(f"{name}\n")
    _print_impls(index, impls)
    return 0


def _cmd_driver(index: DriverApiIndex, source: str) -> int:
    impls = index.impls_for_source(source)
    if not impls:
        print(f"{source}: no DEVICE_API() instance found")
        return 0
    _print_impls(index, impls)
    return 0


def _cmd_check(index: DriverApiIndex) -> None:
    undocumented = [cls for cls, api in sorted(index.apis.items()) if not api.documented]
    print(f"== API classes without @driver_ops tags ({len(undocumented)})")
    print("\n".join(f"  {cls}" for cls in undocumented) or "  (none)")

    missing = []
    unknown = []
    for impl in sorted(index.implementations, key=lambda i: (i.api, i.source)):
        api = index.apis.get(impl.api)
        if api is None:
            unknown.append(f"  {impl.source}: unknown API class '{impl.api}'")
            continue
        if not impl.ops:
            # Declared here, initialized elsewhere; nothing to check.
            continue
        # Ops guarded by a preprocessor condition are only mandatory when that
        # condition holds, which this static scan cannot know.
        for op in api.mandatory_ops(unconditional_only=True):
            if op not in impl.ops:
                missing.append(f"  {impl.source}: {impl.api} implementation misses '{op}'")
        for op in impl.ops:
            if op not in api.ops:
                unknown.append(f"  {impl.source}: '{op}' is not a member of {impl.api}_driver_api")

    print(f"\n== Implementations missing a mandatory operation ({len(missing)})")
    print("\n".join(missing) or "  (none)")
    print(f"\n== Unrecognized API classes or operations ({len(unknown)})")
    print("\n".join(unknown) or "  (none)")

    # Not listed one by one: most are bindings that legitimately have no device
    # driver behind them, such as connectors, partitions or pin control nodes.
    implemented = {c for impl in index.implementations for c in impl.compatibles}
    orphans = len(set(index.known_compatibles) - implemented)
    print(f"\n== Compatibles with a binding but no implementation found ({orphans})")
    print(f"  out of {len(index.known_compatibles)} compatibles declared by a binding")
    print("  use --json to get the full picture")


def parse_args(argv=None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    query = parser.add_mutually_exclusive_group(required=True)
    query.add_argument("--list-apis", action="store_true", help="list all driver API classes")
    query.add_argument("--api", metavar="CLASS", help="list the drivers implementing an API class")
    query.add_argument("--compatible", metavar="COMPAT", help="list the APIs a compatible provides")
    query.add_argument("--driver", metavar="PATH", help="list the APIs a driver source provides")
    query.add_argument("--check", action="store_true", help="report anomalies in the index")
    query.add_argument("--json", action="store_true", help="dump the whole index as JSON")

    parser.add_argument("--op", metavar="NAME", help="with --api, only drivers providing this op")
    parser.add_argument("-o", "--output", metavar="FILE", help="with --json, write to FILE")
    parser.add_argument(
        "--root",
        metavar="DIR",
        action="append",
        default=[],
        help="extra folder to scan for drivers (repeatable)",
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="show scanning warnings")

    args = parser.parse_args(argv)
    if args.op and not args.api:
        parser.error("--op requires --api")
    if args.output and not args.json:
        parser.error("--output requires --json")
    return args


def main(argv=None) -> int:
    args = parse_args(argv)
    logging.basicConfig(
        level=logging.WARNING if args.verbose else logging.ERROR, format="%(message)s"
    )

    index = get_driver_api_index(roots=(*DEFAULT_ROOTS, *args.root))

    if args.list_apis:
        _cmd_list_apis(index)
    elif args.api:
        return _cmd_api(index, args.api, args.op)
    elif args.compatible:
        return _cmd_compatible(index, args.compatible)
    elif args.driver:
        return _cmd_driver(index, args.driver)
    elif args.check:
        _cmd_check(index)
    elif args.json:
        content = json.dumps(index.as_dict(), indent=2)
        if args.output:
            Path(args.output).write_text(content + "\n", encoding="utf-8")
        else:
            print(content)
    return 0


if __name__ == "__main__":
    sys.exit(main())
