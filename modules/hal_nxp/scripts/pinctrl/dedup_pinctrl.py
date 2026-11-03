#!/usr/bin/env python3
#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
Deduplicates NXP SoC pinctrl files in Zephyr's binding and devicetree trees.

Many orderable part numbers of the same die/package have byte-identical
pin mux data (they differ only in temperature/speed grade). This tool
clusters files by content (ignoring the generated file comment header
and the include guard, which embed the part number), keeps one canonical
file per cluster and replaces the other members with one-line wrapper
files that #include the canonical one. Board devicetree includes keep
working unchanged.

Default mode is a dry run that only reports the clusters; pass --apply
to rewrite the tree. Files are only ever replaced when their normalized
content is byte-identical to the canonical file, so the transformation
is provable and reversible.
"""

import argparse
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

from shared import pinctrl_layout

WRAPPER_MARKER = "Content deduplicated by dedup_pinctrl.py"
DELTA_MARKER = "Delta-deduplicated by dedup_pinctrl.py"
# A delta wrapper may not exceed this fraction of the original file.
DELTA_MAX_RATIO = 0.5


def split_leading_comments(lines):
    """
    Returns (index of first content line, leading lines) skipping any
    blank lines and complete /* */ or // comment blocks at the top.
    """
    i = 0
    while i < len(lines):
        stripped = lines[i].strip()
        if stripped == "" or stripped.startswith("//"):
            i += 1
            continue
        if stripped.startswith("/*"):
            while i < len(lines) and "*/" not in lines[i]:
                i += 1
            i += 1  # consume the line containing */
            continue
        break
    return i


def normalize(text, is_header):
    """
    Returns content with the part-number-bearing boilerplate removed:
    leading comment blocks, and for .h files the include guard pair
    plus the trailing #endif. This is the cluster key.
    """
    lines = text.splitlines()
    body = lines[split_leading_comments(lines) :]

    if is_header:
        j = 0
        while j < len(body) and body[j].strip() == "":
            j += 1
        if j < len(body):
            # Generated guard styles: _ZEPHYR_DTS_BINDING_<PART>_ and
            # HAL_NXP_DTS_NXP_<PART>_PINCTRL_H_ (SIUL2 families).
            match = re.match(
                r"#ifndef\s+(\w*ZEPHYR_DTS_BINDING\w*|"
                r"\w+_PINCTRL_H_)",
                body[j].strip(),
            )
            if match:
                guard = match.group(1)
                k = j + 1
                while k < len(body) and body[k].strip() == "":
                    k += 1
                if k < len(body) and body[k].strip() == f"#define {guard}":
                    body = body[:j] + body[k + 1 :]
                    for idx in range(len(body) - 1, -1, -1):
                        stripped = body[idx].strip()
                        if stripped.startswith("#endif"):
                            body = body[:idx] + body[idx + 1 :]
                            break
                        if stripped:
                            break
    content = "\n".join(line.rstrip() for line in body).strip()

    if is_header:
        # The generators emit the same macros in a different order in
        # different runs, so cluster .h files order-insensitively: pure
        # #define lists are position-independent for the preprocessor.
        # Only do this when it is provably safe: every logical line must
        # be a #define and no macro name may be defined twice (a #if or
        # a redefinition would make ordering significant).
        logical = re.sub(r"\\\s*\n\s*", " ", content)
        lines = [line.strip() for line in logical.splitlines() if line.strip()]
        if all(line.startswith("#define") for line in lines):
            names = [macro_name(line) for line in lines]
            if None not in names and len(names) == len(set(names)):
                return "\n".join(sorted(lines))
    return content


def macro_name(define_line):
    """
    The macro name the C preprocessor sees for a #define line: the
    leading identifier only. Some generated files contain malformed
    names like "TRIG_IN5/CT_INP19_P2_7" — cpp defines TRIG_IN5 there,
    and repeated lines redefine it, making the file order-dependent.
    """
    match = re.match(r"#define\s+([A-Za-z_]\w*)", define_line)
    return match.group(1) if match else None


def leading_comment_block(text):
    """Returns the first /* */ comment block verbatim, or None."""
    lines = text.splitlines()
    if not lines or not lines[0].lstrip().startswith("/*"):
        return None
    block = []
    for line in lines:
        block.append(line)
        if "*/" in line:
            return "\n".join(block)
    return None


def guard_symbol(path):
    """
    Include guard for a wrapper, derived from its own file name.
    Never reuse the guard found in the original file: some generated
    files carry another part's guard (e.g. MCXW235BIHNAR shipped with
    the MCXW235AIHNAR guard), which would shortcut the canonical body.
    """
    stem = re.sub(r"\W", "_", path.name.replace("-pinctrl.h", ""))
    return f"_ZEPHYR_DTS_BINDING_{stem.upper()}_WRAPPER_"


def make_wrapper(original_text, original_path, canonical_name, canonical_hash):
    header = leading_comment_block(original_text)
    note = (
        f"/*\n"
        f" * {WRAPPER_MARKER}:\n"
        f" * the pin mux data of this part is identical to {canonical_name}.\n"
        f" * Do not edit; edit the canonical file instead.\n"
        f" * target-hash: {canonical_hash}\n"
        f" */"
    )
    parts = [header, note] if header else [note]
    if original_path.suffix == ".h":
        guard = guard_symbol(original_path)
        parts.append(
            f"#ifndef {guard}\n#define {guard}\n\n"
            f"#include \"{canonical_name}\"\n\n"
            f"#endif /* {guard} */"
        )
    else:
        parts.append(f"#include \"{canonical_name}\"")
    return "\n\n".join(parts) + "\n"


def collect_files(root):
    """Pinctrl files grouped per directory, skipping existing wrappers."""
    return group_files(sorted(root.rglob("*-pinctrl.*")))


def group_files(paths):
    per_dir = {}
    for path in paths:
        path = pathlib.Path(path)
        if path.suffix not in (".h", ".dtsi"):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if WRAPPER_MARKER in text or DELTA_MARKER in text:
            continue
        per_dir.setdefault(path.parent, []).append((path, text))
    return per_dir


def parse_defines(text, path):
    """
    (preamble, {macro name: logical #define line}) for a normalized
    header made of #define lines, comments and #include lines (SIUL2
    families include a dt-bindings header). Returns None when the file
    is not safely order-independent (other directives or duplicate
    macro names). The preamble (the #include lines, in order) must
    match between two files for one to become the other's delta.
    """
    if path.suffix != ".h":
        return None
    body = re.sub(r"/\*.*?\*/", "", normalize(text, True), flags=re.DOTALL)
    logical = re.sub(r"\\\s*\n\s*", " ", body)
    preamble = []
    defines = {}
    for line in (line.strip() for line in logical.splitlines()):
        if not line:
            continue
        if line.startswith("#define"):
            name = macro_name(line)
            if name is None:
                return None
            # Function-like macros must have valid parameter lists; the
            # generators emit broken lines like "#define SPIFI_IO(0)_..."
            # which are hard cpp errors, not modellable as a macro set.
            rest = re.sub(rf"#define\s+{name}", "", line, count=1)
            if rest.startswith("(") and not re.match(
                r"\(\s*([A-Za-z_]\w*(\s*,\s*[A-Za-z_]\w*)*\s*)?\)", rest
            ):
                return None
            # Repeated names (e.g. the malformed "TRIG_IN5/..." lines,
            # which cpp all parses as TRIG_IN5) follow the preprocessor
            # last-wins rule: model the effective macro set, which is
            # exactly what the delta wrapper reproduces and what the
            # gcc -dM verification compares.
            defines[name] = line
        elif line.startswith("#include"):
            preamble.append(line)
        else:
            return None
    return (tuple(preamble), defines) if defines else None


def part_rank(path):
    """Flagship rank: all digit runs of the name, concatenated."""
    return int("".join(re.findall(r"\d+", path.name)) or 0)


def flagship_order(path):
    """
    Total flagship order (smaller sorts first = more flagship): higher
    part number, then shorter name, then alphabetical. Matches the
    exact-cluster canonical choice, and breaks ties between names with
    equal digits (silicon revisions daf5a/daf5b, T/S variants), which a
    pure digit comparison cannot order.
    """
    return (-part_rank(path), len(path.name), path.name.lower())


DTSI_NODE = re.compile(r"/omit-if-no-ref/\s+(\w+):[^{]*\{[^}]*\};")


def parse_dtsi_nodes(text):
    """
    (parent label, {node label: verbatim block}) for a pinctrl dtsi
    with the regular generated shape — leading comments, one top-level
    `&parent { ... }` block made up exclusively of labelled
    /omit-if-no-ref/ nodes. Returns None for anything else.
    """
    parents = list(re.finditer(r"^&(\w+)\s*\{", text, re.MULTILINE))
    if len(parents) != 1:
        return None
    prefix = re.sub(r"/\*.*?\*/", "", text[: parents[0].start()], flags=re.DOTALL)
    end = text.rfind("};")
    suffix = text[end + 2 :] if end >= 0 else "x"
    if prefix.strip() or suffix.strip():
        return None
    body = text[parents[0].end() : end]
    nodes = {}
    for match in DTSI_NODE.finditer(body):
        if match.group(1) in nodes:
            return None
        nodes[match.group(1)] = match.group(0)
    if DTSI_NODE.sub("", body).strip() or not nodes:
        return None
    return parents[0].group(1), nodes


def dedup_files(paths, apply=False):
    """
    Deduplicates an explicit list of files (entry point for
    gen_soc_headers.py). Returns (cluster plan, delta plan, stale
    wrappers, verification failures). Stale wrappers appear when a
    regenerated file changed content that existing wrappers were
    generated against: those parts must be regenerated too.
    """
    per_dir = group_files(paths)
    clusters = plan_clusters(per_dir)
    deltas = plan_deltas(per_dir, clusters)
    failures = []
    if apply:
        _, _, failures = apply_all(clusters, deltas)
    stale = []
    for directory in {pathlib.Path(p).parent for p in paths}:
        stale += check_wrappers(directory)
    for path, reason in stale:
        print(f"STALE WRAPPER {path}: {reason} — regenerate this part's pinctrl data")
    return clusters, deltas, stale, failures


def plan_clusters(per_dir):
    """
    Yields dicts describing each dedup cluster:
    {dir, canonical, wrappers, bytes_saved, content_hash}
    """
    clusters = []
    for directory, entries in per_dir.items():
        by_hash = {}
        for path, text in entries:
            content = normalize(text, path.suffix == ".h")
            if not content:
                continue  # defensive: never cluster empty content
            key = path.suffix + hashlib.sha256(content.encode()).hexdigest()
            by_hash.setdefault(key, []).append((path, text))
        for key, members in by_hash.items():
            if len(members) < 2:
                continue

            # The canonical file is the flagship: the highest part number
            # (all digit runs of the name, concatenated) wins, so lower
            # parts wrap the flagship and never the other way around.
            # Ties prefer the shorter (base) part name.
            def flagship_key(item):
                name = item[0].name
                digits = "".join(re.findall(r"\d+", name))
                return (-int(digits or 0), len(name), name.lower())

            members.sort(key=flagship_key)
            canonical = members[0][0]
            wrappers = members[1:]
            content_hash = key[len(canonical.suffix) :][:16]
            saved = sum(p.stat().st_size for p, _ in wrappers) - sum(
                len(make_wrapper(t, p, canonical.name, content_hash).encode()) for p, t in wrappers
            )
            clusters.append(
                {
                    "dir": directory,
                    "canonical": canonical,
                    "wrappers": wrappers,
                    "bytes_saved": saved,
                    "content_hash": key,
                    "hash16": content_hash,
                }
            )
    return clusters


def include_target(path):
    """
    ("exact" | "delta", include target name) for wrapper files, or
    (None, None) for regular files.
    """
    text = path.read_text(encoding="utf-8", errors="replace")
    if WRAPPER_MARKER in text:
        kind = "exact"
    elif DELTA_MARKER in text:
        kind = "delta"
    else:
        return None, None
    match = re.search(r'#include\s+"([^"]+)"', text)
    return (kind, match.group(1)) if match else (None, None)


def flatten_wrappers(directory):
    """
    Re-points wrappers whose target has itself become an exact wrapper
    (this happens when a later-generated flagship part takes over as
    the canonical file of an existing cluster). Chains are resolved
    through exact wrappers only: a delta wrapper changes the content,
    so it is re-pointed itself but never followed through.
    """
    for path in sorted(directory.glob("*-pinctrl.*")):
        kind, target_name = include_target(path)
        if not target_name:
            continue
        seen = {path.name}
        final_name = target_name
        while final_name not in seen:
            seen.add(final_name)
            target = directory / final_name
            if not target.is_file():
                break
            next_kind, next_name = include_target(target)
            if next_kind != "exact" or not next_name:
                break
            final_name = next_name
        if final_name != target_name and (directory / final_name).is_file():
            text = path.read_text(encoding="utf-8", errors="replace")
            path.write_text(text.replace(target_name, final_name), encoding="utf-8", newline="\n")


def make_delta_wrapper(path, text, base_name, base_hash, base_defs, own_defs):
    """
    Wrapper for a part whose macros differ from the base: include the
    base, remove the macros this part does not have, then define the
    macros that differ or are additional. The resulting macro set is
    exactly the original file's.
    """
    removed = sorted(set(base_defs) - set(own_defs))
    changed = sorted(n for n in set(base_defs) & set(own_defs) if base_defs[n] != own_defs[n])
    added = sorted(set(own_defs) - set(base_defs))

    header = leading_comment_block(text)
    note = (
        f"/*\n"
        f" * {DELTA_MARKER}:\n"
        f" * based on {base_name} with this part's differences applied\n"
        f" * below ({len(removed)} removed, {len(changed)} changed,\n"
        f" * {len(added)} added). Regenerate with dedup_pinctrl.py;\n"
        f" * do not edit by hand.\n"
        f" * target-hash: {base_hash}\n"
        f" */"
    )
    guard = guard_symbol(path)
    body = [f"#include \"{base_name}\""]
    if removed:
        body.append("\n/* Signals not present on this part */")
        body += [f"#undef {name}" for name in removed]
    if changed:
        body.append("\n/* Signals with different mux data on this part */")
        for name in changed:
            body.append(f"#undef {name}")
            body.append(own_defs[name])
    if added:
        body.append("\n/* Signals specific to this part */")
        body += [own_defs[name] for name in added]

    parts = [header, note] if header else [note]
    parts.append(
        f"#ifndef {guard}\n#define {guard}\n\n" + "\n".join(body) + f"\n\n#endif /* {guard} */"
    )
    return "\n\n".join(parts) + "\n"


def make_dtsi_delta(text, base_name, base_hash, parent, base_nodes, own_nodes):
    """
    Delta wrapper for a pinctrl dtsi: include the base, /delete-node/
    the nodes this part does not have or that differ, then re-add this
    part's own definitions. The resulting device tree is exactly the
    original file's.
    """

    def squeeze(block):
        return re.sub(r"\s+", " ", block.strip())

    removed = sorted(label for label in base_nodes if label not in own_nodes)
    changed = sorted(
        label
        for label in set(base_nodes) & set(own_nodes)
        if squeeze(base_nodes[label]) != squeeze(own_nodes[label])
    )
    added = sorted(label for label in own_nodes if label not in base_nodes)

    header = leading_comment_block(text)
    note = (
        f"/*\n"
        f" * {DELTA_MARKER}:\n"
        f" * based on {base_name} with this part's differences applied\n"
        f" * below ({len(removed)} removed, {len(changed)} changed,\n"
        f" * {len(added)} added). Regenerate with dedup_pinctrl.py;\n"
        f" * do not edit by hand.\n"
        f" * target-hash: {base_hash}\n"
        f" */"
    )
    body = [f"#include \"{base_name}\""]
    if removed or changed:
        body.append("")
        body += [f"/delete-node/ &{label};" for label in removed + changed]
    if changed or added:
        body.append(f"\n&{parent} {{")
        body += ["\t" + own_nodes[label].strip() for label in changed + added]
        body.append("};")
    parts = [header, note] if header else [note]
    parts.append("\n".join(body))
    return "\n\n".join(parts) + "\n"


def plan_deltas(per_dir, clusters):
    """
    For files left over by exact clustering, finds the closest
    higher-ranked plain file of the same directory and plans a delta
    wrapper, provided it stays below DELTA_MAX_RATIO of the original
    size. Returns dicts: {path, wrapper, base, saved}.
    """
    wrapped = {p for c in clusters for p, _ in c["wrappers"]}
    deltas = []
    for _directory, entries in per_dir.items():
        plain = [(p, t) for p, t in entries if p not in wrapped]

        def parse(path, text):
            if path.suffix == ".h":
                result = parse_defines(text, path)
                if result:
                    preamble, defines = result
                    return {"parent": preamble, "items": defines, "cmp": defines}
            elif path.suffix == ".dtsi":
                result = parse_dtsi_nodes(text)
                if result:
                    parent, nodes = result
                    return {
                        "parent": parent,
                        "items": nodes,
                        "cmp": {
                            label: re.sub(r"\s+", " ", block.strip())
                            for label, block in nodes.items()
                        },
                    }
            return None

        parsed = {p: parse(p, t) for p, t in plain}

        # Highest flagship rank first: the flagship always stays plain,
        # and a base must itself stay plain (no delta-on-delta chains),
        # so the recorded base hash stays checkable.
        # Superset first within equal part numbers: a die has one full
        # mux set and derived parts are subsets, so the file with more
        # entries is the better delta base (pure removals are smaller
        # and match the hardware reality).
        def base_order(path, parsed=parsed):
            return (
                -part_rank(path),
                -len(parsed[path]["items"]),
                len(path.name),
                path.name.lower(),
            )

        candidates = sorted((it for it in plain if parsed[it[0]]), key=lambda it: base_order(it[0]))
        bases = []  # (path, parsed info, hash) of files staying plain
        for path, text in candidates:
            own = parsed[path]

            def content_hash(text=text, suffix=path.suffix):
                return hashlib.sha256(normalize(text, suffix == ".h").encode()).hexdigest()[:16]

            best = None
            for base_path, base, base_hash in bases:
                if (
                    base_path.suffix != path.suffix
                    or base["parent"] != own["parent"]
                    or not base_order(base_path) < base_order(path)
                ):
                    continue
                b, o = base["cmp"], own["cmp"]
                cost = (
                    len(set(b) - set(o))
                    + len(set(o) - set(b))
                    + sum(1 for n in set(b) & set(o) if b[n] != o[n])
                )
                if best is None or cost < best[0]:
                    best = (cost, base_path, base, base_hash)
            wrapper = None
            if best:
                _, base_path, base, base_hash = best
                if path.suffix == ".h":
                    wrapper = make_delta_wrapper(
                        path, text, base_path.name, base_hash, base["items"], own["items"]
                    )
                else:
                    wrapper = make_dtsi_delta(
                        text, base_path.name, base_hash, own["parent"], base["items"], own["items"]
                    )
                if len(wrapper.encode()) > path.stat().st_size * DELTA_MAX_RATIO:
                    wrapper = None
            if wrapper:
                deltas.append(
                    {
                        "path": path,
                        "wrapper": wrapper,
                        "base": base_path,
                        "original": text,
                        "saved": path.stat().st_size - len(wrapper.encode()),
                    }
                )
            else:
                bases.append((path, own, content_hash()))
    return deltas


def apply_deltas(deltas):
    for delta in deltas:
        delta["path"].write_text(delta["wrapper"], encoding="utf-8", newline="\n")


def _find_zephyr(start):
    """Best-effort location of the zephyr tree (for verification)."""
    if os.environ.get("ZEPHYR_BASE"):
        return pathlib.Path(os.environ["ZEPHYR_BASE"])
    path = pathlib.Path(start).resolve()
    for _ in range(12):
        if (path / "include" / "zephyr").is_dir():
            return path
        zephyr = path / "zephyr"
        if (zephyr / "include" / "zephyr").is_dir():
            return zephyr
        if path.parent == path:
            break
        path = path.parent
    return None


_GUARD = re.compile(r"ZEPHYR_DTS_BINDING|_PINCTRL_H_$")


def _macro_set(path, include_dir):
    """Effective macro dict via gcc -E -dM, ignoring include guards."""
    cmd = ["gcc", "-E", "-dM", "-x", "c"]
    if include_dir:
        cmd += ["-I", str(include_dir)]
    result = subprocess.run(cmd + [str(path)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError("gcc failed")
    macros = {}
    for line in result.stdout.splitlines():
        if not line.startswith("#define"):
            continue
        parts = line.split(None, 2)
        name = parts[1].split("(")[0]
        if not _GUARD.search(name):
            macros[name] = parts[2] if len(parts) > 2 else ""
    return macros


def _dts_tree(text, basedir, dtlib, workdir):
    """Devicetree node/property map of a pinctrl dtsi (or wrapper)."""

    def inline(content):
        return re.sub(
            r'#include\s+"([^"]+)"',
            lambda m: inline((basedir / m.group(1)).read_text(encoding="utf-8", errors="replace")),
            content,
        )

    inlined = inline(text)
    labels = sorted(set(re.findall(r"^&(\w+)", inlined, re.MULTILINE)))
    declarations = " ".join(f"{label}: {label} {{}};" for label in labels)
    harness = workdir / "verify.dts"
    harness.write_text("/dts-v1/;\n/ { " + declarations + " };\n" + inlined, encoding="utf-8")
    dt = dtlib.DT(str(harness))
    return {
        node.path: sorted((name, prop.value) for name, prop in node.props.items())
        for node in dt.node_iter()
    }


def verify_written(written):
    """
    Independently verifies every rewritten file against its original
    content: gcc -E -dM macro equality for .h, dtlib devicetree
    equality for .dtsi. Files that fail are restored to their original
    content. Returns (verified count, skipped count, failures).
    """
    gcc = shutil.which("gcc")
    zephyr = _find_zephyr(written[0][0].parent) if written else None
    include_dir = zephyr / "include" if zephyr else None
    dtlib = None
    if zephyr and (zephyr / "scripts/dts/python-devicetree/src").is_dir():
        sys.path.insert(0, str(zephyr / "scripts/dts/python-devicetree/src"))
        try:
            from devicetree import dtlib as dtlib_module

            dtlib = dtlib_module
        except ImportError:
            pass

    verified = skipped = 0
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        workdir = pathlib.Path(tmp)
        for path, original in written:
            try:
                if path.suffix == ".h" and gcc:
                    reference = workdir / "original.h"
                    reference.write_text(original, encoding="utf-8")
                    same = _macro_set(reference, include_dir) == _macro_set(path, include_dir)
                elif path.suffix == ".dtsi" and dtlib:
                    same = _dts_tree(original, path.parent, dtlib, workdir) == _dts_tree(
                        path.read_text(encoding="utf-8", errors="replace"),
                        path.parent,
                        dtlib,
                        workdir,
                    )
                else:
                    skipped += 1
                    continue
            except RuntimeError:
                # Original not processable by gcc (pre-existing invalid
                # macros): for exact wrappers fall back to normalized
                # content equality against the include target.
                kind, target_name = include_target(path)
                target = path.parent / target_name if target_name else None
                if kind == "exact" and target and target.is_file():
                    same = normalize(original, True) == normalize(
                        target.read_text(encoding="utf-8", errors="replace"), True
                    )
                else:
                    skipped += 1
                    continue
            if same:
                verified += 1
            else:
                failures.append(path)
                path.write_text(original, encoding="utf-8", newline="\n")
    return verified, skipped, failures


def check_wrappers(root):
    """
    Verifies the recorded target-hash of every existing wrapper against
    the current content of its target file. A mismatch means the target
    was changed after the wrapper was generated (e.g. only the base
    part's data pack was regenerated), so the wrapper no longer
    reproduces its original content and must be regenerated.
    """
    stale = []
    for path in sorted(pathlib.Path(root).rglob("*-pinctrl.*")):
        text = path.read_text(encoding="utf-8", errors="replace")
        if WRAPPER_MARKER not in text and DELTA_MARKER not in text:
            continue
        hash_match = re.search(r"target-hash:\s*([0-9a-f]{16})", text)
        include_match = re.search(r'#include\s+"([^"]+)"', text)
        if not hash_match or not include_match:
            continue
        target = path.parent / include_match.group(1)
        if not target.is_file():
            stale.append((path, "target missing"))
            continue
        target_text = target.read_text(encoding="utf-8", errors="replace")
        current = hashlib.sha256(
            normalize(target_text, target.suffix == ".h").encode()
        ).hexdigest()[:16]
        if current != hash_match.group(1):
            stale.append((path, f"target {target.name} changed since this wrapper was generated"))
    return stale


def apply_clusters(clusters):
    for cluster in clusters:
        canonical = cluster["canonical"]
        for path, text in cluster["wrappers"]:
            path.write_text(
                make_wrapper(text, path, canonical.name, cluster["hash16"]),
                encoding="utf-8",
                newline="\n",
            )
        # Self-check: canonical must still normalize to the cluster hash
        canonical_hash = (
            canonical.suffix
            + hashlib.sha256(
                normalize(
                    canonical.read_text(encoding="utf-8", errors="replace"),
                    canonical.suffix == ".h",
                ).encode()
            ).hexdigest()
        )
        if canonical_hash != cluster["content_hash"]:
            raise RuntimeError(f"post-apply verification failed for {canonical}")


def restamp_referrers(written):
    """
    A file rewritten in this run (e.g. an exact-cluster canonical that
    also became a delta wrapper of a bigger part) invalidates the
    target-hash its referrers recorded, even though their effective
    content is unchanged: include chains compose. Refresh those stamps.
    Only targets written by this run are restamped, so a target changed
    outside the tool still shows up as stale.
    """
    written_names = {}
    for path in written:
        written_names.setdefault(path.parent, set()).add(path.name)
    for directory, names in written_names.items():
        for path in sorted(directory.glob("*-pinctrl.*")):
            _, target_name = include_target(path)
            if not target_name or target_name not in names:
                continue
            target = directory / target_name
            new_hash = hashlib.sha256(
                normalize(
                    target.read_text(encoding="utf-8", errors="replace"), target.suffix == ".h"
                ).encode()
            ).hexdigest()[:16]
            text = path.read_text(encoding="utf-8", errors="replace")
            new_text = re.sub(r"target-hash:\s*[0-9a-f]{16}", f"target-hash: {new_hash}", text)
            if new_text != text:
                path.write_text(new_text, encoding="utf-8", newline="\n")


def apply_all(clusters, deltas):
    """
    Applies the plan, then independently verifies every written file
    against its original content (gcc -E -dM for .h, dtlib for .dtsi);
    failures are reverted to the original file. Returns the
    (verified, skipped, failures) triple from verify_written().
    """
    apply_clusters(clusters)
    apply_deltas(deltas)
    written = [p for c in clusters for p, _ in c["wrappers"]] + [d["path"] for d in deltas]
    for directory in {p.parent for p in written}:
        flatten_wrappers(directory)
    restamp_referrers(written)
    originals = [(p, t) for c in clusters for p, t in c["wrappers"]] + [
        (d["path"], d["original"]) for d in deltas
    ]
    verified, skipped, failures = verify_written(originals)
    print(
        f"Verification: {verified} verified"
        + (f", {skipped} skipped (gcc/dtlib unavailable)" if skipped else "")
        + (f", {len(failures)} FAILED AND REVERTED" if failures else "")
    )
    if skipped:
        # A skipped check is not a passed check. Say so loudly, otherwise the
        # run reads as if every rewritten file had been proven equivalent.
        print(
            f"  WARNING: {skipped} rewritten file(s) were NOT proven "
            "equivalent to their original content. Install gcc (for .h) and "
            "put zephyr/scripts/dts/python-devicetree/src on PYTHONPATH (for "
            ".dtsi) to enable the check."
        )
    for path in failures:
        print(f"  VERIFY FAILED (reverted to original): {path}")
    return verified, skipped, failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "root",
        nargs="*",
        type=pathlib.Path,
        help="Trees to deduplicate (default: NXP binding headers and SoC DTSI)",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Rewrite duplicate files as wrappers (default: dry-run report only)",
    )
    parser.add_argument("--json", metavar="FILE", help="Write the dedup plan as JSON")
    args = parser.parse_args()

    roots = args.root or [pinctrl_layout.output_root(".h"), pinctrl_layout.output_root(".dtsi")]
    for directory in roots:
        if not directory.is_dir():
            sys.exit(f"Not a directory: {directory}")
    root = pathlib.Path(os.path.commonpath(roots))

    stale = [item for directory in roots for item in check_wrappers(directory)]
    for path, reason in stale:
        print(
            f"STALE WRAPPER {path.relative_to(root)}: {reason} — "
            f"regenerate this part's pinctrl data"
        )

    per_dir = {
        path: files for directory in roots for path, files in collect_files(directory).items()
    }
    clusters = plan_clusters(per_dir)
    clusters.sort(key=lambda c: -c["bytes_saved"])
    deltas = plan_deltas(per_dir, clusters)
    deltas.sort(key=lambda d: -d["saved"])

    total_saved = 0
    total_wrappers = 0
    for cluster in clusters:
        names = ", ".join(p.name for p, _ in cluster["wrappers"])
        rel = cluster["canonical"].relative_to(root)
        print(f"{rel}  <=  {names}  (saves {cluster['bytes_saved'] / 1024:.0f}KB)")
        total_saved += cluster["bytes_saved"]
        total_wrappers += len(cluster["wrappers"])
    delta_saved = 0
    for delta in deltas:
        print(
            f"DELTA {delta['path'].relative_to(root)}  ->  "
            f"{delta['base'].name}  (saves {delta['saved'] / 1024:.0f}KB)"
        )
        delta_saved += delta["saved"]

    print(
        f"\n{len(clusters)} clusters, {total_wrappers} files become wrappers, "
        f"{total_saved / 1048576:.1f}MB saved; "
        f"{len(deltas)} delta wrappers, {delta_saved / 1048576:.1f}MB saved"
    )

    if args.json:
        payload = {
            "clusters": [
                {
                    "canonical": str(c["canonical"].relative_to(root)),
                    "wrappers": [str(p.relative_to(root)) for p, _ in c["wrappers"]],
                    "bytes_saved": c["bytes_saved"],
                    "content_hash": c["content_hash"],
                }
                for c in clusters
            ],
            "deltas": [
                {
                    "path": str(d["path"].relative_to(root)),
                    "base": str(d["base"].relative_to(root)),
                    "bytes_saved": d["saved"],
                }
                for d in deltas
            ],
        }
        pathlib.Path(args.json).write_text(json.dumps(payload, indent=2))
        print(f"Plan written to {args.json}")

    failures = []
    if args.apply:
        _, _, failures = apply_all(clusters, deltas)
        print("Applied: duplicate files rewritten as wrappers")
    elif clusters or deltas:
        print("Dry run only; use --apply to rewrite the files")

    # Stale wrappers mean a base file changed after its dependents were
    # generated; verification failures were reverted. Fail either way
    # so CI cannot miss it.
    return 1 if stale or failures else 0


if __name__ == "__main__":
    sys.exit(main())
