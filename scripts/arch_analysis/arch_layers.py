#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 The Zephyr Project Contributors
#
"""Standalone architectural-layering / independence analyzer for Zephyr.

This reproduces the *method* of the ECLAIR ``B.INDEPENDENCE`` service used in
the Zephyr Safety WG "Verification of Software Architectural Constraints" talk,
but depends on nothing beyond the Zephyr SDK binutils (objdump / nm) that are
already used to build the image. No commercial static analyzer is required.

Pipeline
--------
1. Discover every object file (``*.obj`` / ``*.o``) produced by a Zephyr build.
2. For each object, recover the source path it was compiled from (from
   ``compile_commands.json``) and map that path to a *component* -- its area in
   the top-level MAINTAINERS.yml, parsed with scripts/get_maintainer.py. The
   components are therefore the project's own maintained areas, not a
   hand-written list. The scope YAML only declares which areas are in-scope.
3. Build a global map: defined symbol -> component (via ``nm``).
4. For each object, read its relocations (``objdump -dr``) -- these are the
   calls and data references the linker will resolve -- and attribute each to
   an inter-component edge (from = the object's component, to = the component
   that defines the referenced symbol).
5. Check every cross-component edge against a declarative permission model and
   flag the ones that are not allowed.
6. Emit a Graphviz graph (green = allowed, red = violation), plus JSON and a
   human-readable text report.

Limitations (documented, not hidden)
------------------------------------
* Object-level relocations miss calls that the optimizer fully inlined away.
  ECLAIR works at source level and does not. This tool therefore UNDER-reports
  intra-translation-unit-inlined dependencies; cross-component API calls (the
  ones that matter for layering) are preserved because they cross a link
  boundary.
* ``ref`` edges conflate data reads and writes. Splitting them needs
  per-instruction load/store decoding (a planned second backend). The deck's
  integrity story (write-forbidden) needs that; layering (call/ref) does not.

Usage
-----
    python3 scripts/arch_analysis/arch_layers.py \
        --build build \
        --config scripts/arch_analysis/zephyr_scope.yaml \
        --out-dir build/arch_analysis
"""

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from collections import defaultdict

try:
    import yaml
except ImportError:
    sys.exit("error: PyYAML is required (pip install pyyaml)")

# Repository root and access to the in-tree MAINTAINERS parser.
_HERE = os.path.dirname(os.path.abspath(__file__))
ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE") or os.path.normpath(
    os.path.join(_HERE, "..", ".."))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts"))


# Relocation-type substrings that denote a transfer of control (a "call").
# Everything else that references a symbol is treated as a data reference.
_CALL_RE = re.compile(r'(CALL|PLT|JUMP|BRANCH|PC26|PLT32|_PC22|_WPLT|_JMP)')
# MOVT is the high half of a two-instruction ARM address load; skip it so a
# single address load is not counted twice (MOVW carries the same symbol).
_MOVT_RE = re.compile(r'MOVT')
_RELOC_LINE = re.compile(r'^\s*[0-9a-fA-F]+:\s+(R_\S+)\s+(\S+)')


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True, check=False).stdout


def detect_binutils(build):
    """Return (objdump, nm) paths from the build's CMake toolchain."""
    cache = os.path.join(build, "CMakeCache.txt")
    cc = None
    objdump = None
    if os.path.exists(cache):
        with open(cache) as fh:
            for line in fh:
                if line.startswith("CMAKE_C_COMPILER:"):
                    cc = line.split("=", 1)[1].strip()
                elif line.startswith("CMAKE_OBJDUMP:"):
                    objdump = line.split("=", 1)[1].strip()
    if objdump and os.path.exists(objdump):
        base = objdump
        nm = re.sub(r'objdump$', 'nm', base)
        if os.path.exists(nm):
            return objdump, nm
    if cc:
        # Derive the toolchain prefix from the C compiler path.
        for suf in ("gcc", "cc", "clang"):
            if cc.endswith(suf):
                objdump = cc[: -len(suf)] + "objdump"
                nm = cc[: -len(suf)] + "nm"
                if os.path.exists(objdump) and os.path.exists(nm):
                    return objdump, nm
    # Last resort: host binutils.
    return "objdump", "nm"


def object_to_source(obj_path, build_root=""):
    """Recover the source path an object was compiled from.

    The build tree mirrors the source tree, but a source may be bucketed into a
    library whose CMake object dir drops the leading sub-path (e.g. kernel/mutex.c
    -> .../kernel/CMakeFiles/kernel.dir/mutex.c.obj). We therefore recombine the
    directory *before* ``CMakeFiles`` (which mirrors the source subtree) with the
    ``.dir``-relative remainder, then make it relative to the build root so the
    result carries the component signal (``/kernel/``, ``/arch/`` ...) without
    host-path noise.
    """
    p = obj_path.replace(os.sep, "/")
    m = re.search(r'(.*)/CMakeFiles/[^/]+\.dir/(.*)\.(obj|o)$', p)
    if m:
        prefix_dir, rel = m.group(1), m.group(2)
        # CMake encodes '..' path segments (out-of-tree sources) as '__'.
        rel = "/".join(".." if s == "__" else s for s in rel.split("/"))
        combined = os.path.normpath(prefix_dir + "/" + rel)
    else:
        combined = os.path.normpath(re.sub(r'\.(obj|o)$', '', p))
    if build_root:
        br = os.path.normpath(build_root)
        if combined.startswith(br + os.sep):
            combined = combined[len(br) + 1:]
    return combined


def load_compdb(build):
    """Map absolute object path -> absolute source path from compile_commands."""
    path = os.path.join(build, "compile_commands.json")
    mapping = {}
    if not os.path.exists(path):
        return mapping
    for e in json.load(open(path)):
        src = e.get("file")
        directory = e.get("directory", "")
        obj = e.get("output")
        if not obj:
            args = e.get("arguments")
            if args is None and "command" in e:
                args = shlex.split(e["command"])
            for i, a in enumerate(args or []):
                if a == "-o" and i + 1 < len(args):
                    obj = args[i + 1]
                    break
                if a.startswith("-o") and len(a) > 2:
                    obj = a[2:]
                    break
        if obj and src:
            obj_abs = os.path.normpath(os.path.join(directory, obj))
            mapping[obj_abs] = os.path.abspath(src)
    return mapping


def _literal_prefix(glob):
    """The fixed part of a glob, before the first wildcard character."""
    return re.split(r'[*?\[]', glob, 1)[0]


class MaintainersClassifier:
    """Map a source file to its MAINTAINERS.yml area (the 'component').

    Components are not hand-maintained: they are the areas defined in the
    top-level MAINTAINERS.yml, parsed with the in-tree scripts/get_maintainer.py.
    A file that matches several areas is assigned to the most specific one (the
    area whose matching ``files`` pattern has the longest literal prefix). Files
    outside the Zephyr repository (e.g. HAL modules) are grouped as ``module:
    <name>``; in-repo files matching no area are ``UNMAINTAINED``.
    """

    def __init__(self, zephyr_base, maintainers_file=None):
        import get_maintainer  # in-tree, added to sys.path above
        self.base = os.path.abspath(zephyr_base)
        mfile = maintainers_file
        if mfile and not os.path.isabs(mfile):
            mfile = os.path.join(self.base, mfile)
        self.M = get_maintainer.Maintainers(mfile)
        raw = yaml.safe_load(open(self.M.filename))
        self.area_files = {name: (a.get("files") or [])
                           for name, a in raw.items()}
        self._cache = {}

    def _normalize(self, source):
        """Return (repo_relative_path, is_in_repo)."""
        s = source.replace(os.sep, "/")
        if os.path.isabs(source):
            ap = os.path.normpath(source)
            if ap == self.base or ap.startswith(self.base + os.sep):
                return os.path.relpath(ap, self.base).replace(os.sep, "/"), True
            return s, False
        if s.startswith("zephyr/"):      # build-relative reconstruction fallback
            return s[len("zephyr/"):], True
        return s, False

    @staticmethod
    def _module_name(s):
        parts = s.split("/")
        if "modules" in parts:
            i = parts.index("modules")
            return "module: " + "/".join(parts[i + 1:i + 3])
        return "EXTERNAL"

    def _specificity(self, area_name, path):
        best = 0
        for glob in self.area_files.get(area_name, []):
            lit = _literal_prefix(glob)
            if lit and path.startswith(lit):
                best = max(best, len(lit))
        return best

    def component(self, source):
        if source in self._cache:
            return self._cache[source]
        rel, in_repo = self._normalize(source)
        if not in_repo:
            comp = self._module_name(rel)
        else:
            matched = [a for a in self.M.areas.values() if a._contains(rel)]
            if not matched:
                comp = "UNMAINTAINED"
            elif len(matched) == 1:
                comp = matched[0].name
            else:
                matched.sort(key=lambda a: (-self._specificity(a.name, rel),
                                            a.name))
                comp = matched[0].name
        self._cache[source] = comp
        return comp

    def display(self, source):
        rel, _ = self._normalize(source)
        return rel


class ScopeModel:
    """Independence policy: which MAINTAINERS areas are in the qualification scope.

    Rule: in-scope code must not call or read out-of-scope code. Everything not
    listed in ``in_scope`` (including EXTERNAL / module / UNMAINTAINED) is out.
    """

    def __init__(self, cfg):
        self.in_scope = set(cfg.get("in_scope", []))

    def is_allowed(self, frm, to, action=None):
        return not (frm in self.in_scope and to not in self.in_scope)


def collect(build, classifier, objdump, nm, verbose=False):
    objects = []
    for root, _dirs, files in os.walk(build):
        for f in files:
            if f.endswith((".obj", ".o")):
                objects.append(os.path.join(root, f))
    compdb = load_compdb(build)
    if verbose:
        print(f"  {len(objects)} object files, "
              f"{len(compdb)} compile-db entries", file=sys.stderr)

    # source path (per object) and component for that object
    obj_component = {}
    obj_source = {}
    # global defined symbol -> component
    sym_component = {}

    for obj in objects:
        # Prefer the authoritative source from compile_commands.json; fall back
        # to reconstructing it from the object's build path.
        src = compdb.get(os.path.abspath(obj)) or object_to_source(obj, build)
        obj_source[obj] = classifier.display(src)
        comp = classifier.component(src)
        obj_component[obj] = comp
        for line in run([nm, "-f", "posix", "--defined-only", obj]).splitlines():
            parts = line.split()
            if len(parts) >= 2 and parts[1] in "TWDBRVGStwdbrvgs":
                # Global (uppercase) symbols are the only ones referable across
                # objects; still record locals in case of same-file resolution.
                sym_component.setdefault(parts[0], comp)

    # edge -> {action -> count}, and edge -> {action -> set(example symbols)}
    edges = defaultdict(lambda: defaultdict(int))
    examples = defaultdict(lambda: defaultdict(set))

    for obj in objects:
        frm = obj_component[obj]
        for line in run([objdump, "-dr", "--no-show-raw-insn", obj]).splitlines():
            m = _RELOC_LINE.match(line)
            if not m:
                continue
            rtype, target = m.group(1), m.group(2)
            if _MOVT_RE.search(rtype):
                continue
            target = target.split("+")[0].split("-")[0]
            if not target or target.startswith("."):
                continue  # section-relative reloc: no symbol identity
            to = sym_component.get(target)
            if to is None:
                to = "EXTERNAL"  # libgcc / compiler builtins / unresolved
            if to == frm:
                continue
            action = "call" if _CALL_RE.search(rtype) else "ref"
            edges[(frm, to)][action] += 1
            ex = examples[(frm, to)][action]
            if len(ex) < 8:
                ex.add(target)

    return edges, examples, obj_component, obj_source


def build_report(edges, examples, scope):
    rows = []
    for (frm, to), actions in edges.items():
        for action, count in actions.items():
            allowed = scope.is_allowed(frm, to, action)
            rows.append({
                "from": frm,
                "to": to,
                "action": action,
                "count": count,
                "allowed": allowed,
                "examples": sorted(examples[(frm, to)][action]),
            })
    rows.sort(key=lambda r: (r["allowed"], -r["count"]))
    return rows


def write_dot(rows, path, in_scope=frozenset()):
    lines = ["digraph zephyr_layers {",
             '  rankdir=LR;',
             '  node [shape=box style="rounded,filled" fillcolor="#cfe2f3" '
             'fontname="Helvetica"];',
             '  edge [fontname="Helvetica" fontsize=10];']
    nodes = set()
    for r in rows:
        nodes.add(r["from"])
        nodes.add(r["to"])
    for n in sorted(nodes):
        fill = "#fffacd" if n in in_scope else \
               "#f4cccc" if n == "EXTERNAL" else "#cfe2f3"
        lines.append(f'  "{n}" [label="{n}" fillcolor="{fill}"];')
    for r in rows:
        color = "#38761d" if r["allowed"] else "#cc0000"
        style = "solid" if r["allowed"] else "bold"
        label = f'{r["action"]} ({r["count"]})'
        lines.append(f'  "{r["from"]}" -> "{r["to"]}" '
                     f'[label="{label}" color="{color}" '
                     f'fontcolor="{color}" style={style}];')
    lines.append("}")
    with open(path, "w") as fh:
        fh.write("\n".join(lines) + "\n")


def write_text(rows, path):
    viol = [r for r in rows if not r["allowed"]]
    ok = [r for r in rows if r["allowed"]]
    with open(path, "w") as fh:
        fh.write("Zephyr architectural layering report\n")
        fh.write("=" * 60 + "\n\n")
        fh.write(f"edges total   : {len(rows)}\n")
        fh.write(f"allowed       : {len(ok)}\n")
        fh.write(f"VIOLATIONS    : {len(viol)}  "
                 f"({sum(r['count'] for r in viol)} references)\n\n")
        fh.write("VIOLATIONS (from -> to : action count)\n")
        fh.write("-" * 60 + "\n")
        for r in viol:
            fh.write(f"  {r['from']:>16} -> {r['to']:<16} : "
                     f"{r['action']:<4} {r['count']:>5}\n")
            if r["examples"]:
                fh.write(f"        e.g. {', '.join(r['examples'][:6])}\n")
        fh.write("\nALLOWED\n")
        fh.write("-" * 60 + "\n")
        for r in ok:
            fh.write(f"  {r['from']:>16} -> {r['to']:<16} : "
                     f"{r['action']:<4} {r['count']:>5}\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", required=True, help="Zephyr build directory")
    ap.add_argument("--config", required=True,
                    help="scope YAML (in_scope MAINTAINERS areas)")
    ap.add_argument("--out-dir", default=None,
                    help="output directory (default: <build>/arch_analysis)")
    ap.add_argument("--objdump", help="override objdump path")
    ap.add_argument("--nm", help="override nm path")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    out = args.out_dir or os.path.join(args.build, "arch_analysis")
    os.makedirs(out, exist_ok=True)

    cfg = yaml.safe_load(open(args.config))
    classifier = MaintainersClassifier(ZEPHYR_BASE, cfg.get("maintainers"))
    scope = ScopeModel(cfg)

    objdump, nm = (args.objdump, args.nm)
    if not (objdump and nm):
        d_objdump, d_nm = detect_binutils(args.build)
        objdump = objdump or d_objdump
        nm = nm or d_nm
    if args.verbose:
        print(f"objdump: {objdump}\nnm:      {nm}", file=sys.stderr)

    edges, examples, obj_comp, _ = collect(args.build, classifier, objdump, nm,
                                           args.verbose)
    rows = build_report(edges, examples, scope)

    json_path = os.path.join(out, "report.json")
    txt_path = os.path.join(out, "report.txt")
    dot_path = os.path.join(out, "graph.dot")
    with open(json_path, "w") as fh:
        json.dump(rows, fh, indent=2)
    write_text(rows, txt_path)
    write_dot(rows, dot_path, scope.in_scope)

    png_path = os.path.join(out, "graph.png")
    try:
        subprocess.run(["dot", "-Tpng", dot_path, "-o", png_path], check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        png_path = None

    n_viol = sum(1 for r in rows if not r["allowed"])
    print(f"components in build: "
          f"{len(set(obj_comp.values()))}")
    print(f"cross-component edges: {len(rows)}  violations: {n_viol}")
    print(f"reports: {txt_path}")
    print(f"         {json_path}")
    print(f"         {dot_path}" + (f"\n         {png_path}" if png_path else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
