#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 The Zephyr Project Contributors
#
"""Local interactive web app for Zephyr architectural layering analysis.

Serves an interactive dependency/layering explorer on top of a real Zephyr
build. The graph, components and violations are recomputed live from the build's
object files (via scripts/arch_analysis/arch_layers.py) and the component model
YAML, so you can edit the policy and hit "Reload" to see the effect immediately.

Depends only on the Python standard library plus PyYAML (already required by the
analyzer). No web framework, no external JavaScript / CDN.

Usage
-----
    west build -p always -b frdm_mcxn947/mcxn947/cpu0 samples/hello_world -d build
    python3 scripts/arch_analysis/serve.py \
        --build build \
        --config scripts/arch_analysis/zephyr_scope.yaml \
        --port 8080
    # then open http://localhost:8080
"""

import argparse
import json
import os
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import arch_layers as al  # noqa: E402

try:
    import yaml
except ImportError:
    sys.exit("error: PyYAML is required (pip install pyyaml)")

WEBAPP_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "webapp")


def analyze(build, config):
    """Extract the RAW dependency graph for the web frontend.

    Only the (expensive) structural extraction happens here -- the allowed vs
    violation decision is left to the client so that toggling a component's
    scope in the UI recomputes instantly without re-reading object files.
    """
    cfg = yaml.safe_load(open(config))
    classifier = al.MaintainersClassifier(al.ZEPHYR_BASE, cfg.get("maintainers"))
    objdump, nm = al.detect_binutils(build)
    edges, examples, obj_component, obj_source = al.collect(
        build, classifier, objdump, nm)

    rows = []
    for (frm, to), actions in edges.items():
        for action, count in actions.items():
            rows.append({
                "from": frm, "to": to, "action": action, "count": count,
                "examples": sorted(examples[(frm, to)][action]),
            })

    files_by_comp = {}
    for obj, comp in obj_component.items():
        files_by_comp.setdefault(comp, set()).add(obj_source[obj])

    referenced = set()
    for r in rows:
        referenced.add(r["from"])
        referenced.add(r["to"])

    def layer_root(path):
        parts = path.split("/")
        tops = ("kernel", "arch", "subsys", "drivers", "lib", "soc",
                "include", "modules")
        for i, p in enumerate(parts):
            if p in tops:
                return "/".join(parts[i:i + 2])
        return parts[0]

    nodes = []
    for name in sorted(referenced):
        files = sorted(files_by_comp.get(name, []))
        # Meaningful layer directories the component's files live under.
        roots = sorted({layer_root(f) for f in files})
        nodes.append({
            "id": name,
            "files": files,
            "file_count": len(files),
            "roots": roots,
            "external": name == "EXTERNAL",
        })

    initial_in_scope = list(cfg.get("in_scope", ["Kernel"]))

    meta = {
        "build": os.path.abspath(build),
        "config": os.path.abspath(config),
        "objdump": objdump,
        "objects": len(obj_component),
        "edges": len(rows),
    }
    return {"nodes": nodes, "edges": rows,
            "initial_in_scope": initial_in_scope, "meta": meta}


class Handler(BaseHTTPRequestHandler):
    build = None
    config = None

    def log_message(self, *args):  # quieter console
        pass

    def _send(self, code, body, ctype):
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?", 1)[0]
        if path in ("/", "/index.html"):
            with open(os.path.join(WEBAPP_DIR, "index.html")) as fh:
                self._send(200, fh.read(), "text/html; charset=utf-8")
        elif path == "/api/data":
            try:
                data = analyze(self.build, self.config)
                self._send(200, json.dumps(data), "application/json")
            except Exception as exc:  # surface analysis errors in the UI
                self._send(500, json.dumps({"error": str(exc)}),
                           "application/json")
        else:
            self._send(404, "not found", "text/plain")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", required=True, help="Zephyr build directory")
    ap.add_argument("--config", required=True, help="component model YAML")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--host", default="127.0.0.1")
    args = ap.parse_args()

    Handler.build = args.build
    Handler.config = args.config
    srv = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"arch_analysis web app on http://{args.host}:{args.port}")
    print(f"  build:  {os.path.abspath(args.build)}")
    print(f"  config: {os.path.abspath(args.config)}  (edit + Reload to re-check)")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nbye")


if __name__ == "__main__":
    main()
