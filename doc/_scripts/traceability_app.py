# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Interactive requirements-traceability web application for a Zephyr tree.

Builds on :mod:`gen_traceability_report` (the mlx.traceability JSON model) and
joins two additional evidence sources from a twister run:

* ``twister.json``     -- test execution results (pass/fail/skip per instance)
* ``test_matrix.json`` -- per-test line coverage (``by_test`` / ``by_line``)

together with the ``implemented_by`` linkage (``@satisfies`` on the kernel API)
to answer the *true traceability* question: does a test that verifies a
requirement actually exercise the code that implements that requirement?

For every requirement the app computes an **adequacy verdict** by resolving its
implementing symbols to function bodies in ``kernel/**/*.c`` (including the
``z_impl_`` syscall bodies) and intersecting those line ranges with the union
of the line coverage of the requirement's verifying tests:

* ``true``       -- every resolved implementing function is exercised by the
                    requirement's own verifying tests
* ``partial``    -- some implementing functions are exercised, others not
* ``broken``     -- the verifying tests never touch the implementing code
                    (the requirement *looks* verified but is not)
* ``unresolved`` -- implementation links exist but none map to a function body
                    (macros / static inlines)
* ``no-impl``    -- the requirement has no ``implemented_by`` link
* ``no-cov``     -- the verifying tests have no coverage data in this run

Everything is served from memory by a dependency-free stdlib HTTP server; the
132 MB coverage matrix is indexed once at startup and queried on demand.

Usage (from the zephyr tree root)::

    python doc/_scripts/traceability_app.py \\
        [--json doc/_build/html/traceability.json] \\
        [--twister twister.json] [--matrix test_matrix.json] \\
        [--root .] [--port 8765]
"""

import argparse
import glob
import json
import re
import sys
import webbrowser
from collections import defaultdict
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

sys.path.insert(0, str(Path(__file__).parent))
from gen_traceability_report import build_model, srs_sortkey  # noqa: E402

# Test-result rollup precedence (worst wins for reporting a test's state).
_FAIL = {"failed", "error"}
_SKIP = {"skipped", "blocked", "not run", "filtered"}


def slugify(name):
    return re.sub(r"[^a-zA-Z0-9]", "_", name)


# --- twister.json ------------------------------------------------------------

def load_results(twister_path):
    """Index twister results by suite-qualified ZTEST function name.

    Doxygen documents ZTEST functions as <ztest suite>__<fn> (ZTEST PREDEFINED
    in zephyr.doxyfile.in). Twister identifiers are
    <scenario>.<ztest suite>.<fn minus test_>, so the qualified name is
    rebuilt from the last two segments. Qualifying by suite also keeps
    same-named tests from different suites apart, which the previous bare
    test_<fn> keying silently merged.

    Returns (env, results, qual_map) where results[fn] = {"instances": [...],
    rollup...} and qual_map[scenario][test_<fn>] = qualified name, used to
    join the coverage matrix (whose keys only carry the scenario slug).
    """
    data = json.loads(Path(twister_path).read_text())
    env = data.get("environment", {})
    results = defaultdict(lambda: {"instances": []})
    qual_map = defaultdict(dict)
    for suite in data.get("testsuites", []):
        for case in suite.get("testcases", []):
            parts = case["identifier"].split(".")
            bare = "test_" + parts[-1]
            fn = parts[-2] + "__" + bare if len(parts) >= 2 else bare
            qual_map[suite.get("name", "")][bare] = fn
            results[fn]["instances"].append({
                "suite": suite.get("name", ""),
                "platform": suite.get("platform", ""),
                "status": (case.get("status") or "").lower(),
                "time": case.get("execution_time") or "",
            })
    for fn, r in results.items():
        st = [i["status"] for i in r["instances"]]
        r["npass"] = sum(s == "passed" for s in st)
        r["nfail"] = sum(s in _FAIL for s in st)
        r["nskip"] = sum(s in _SKIP for s in st)
        r["rollup"] = ("failing" if r["nfail"] else
                       "passing" if r["npass"] else
                       "skipped" if r["nskip"] else "no-run")
    return env, dict(results), dict(qual_map)


# --- test_matrix.json ---------------------------------------------------------

def load_matrix(matrix_path, qual_map):
    """Index the coverage matrix.

    Matrix keys are <scenario slug>_<test fn>; the scenario slug does not
    carry the ztest suite name, so the suite-qualified test name is recovered
    via ``qual_map`` (scenario -> bare test_ name -> qualified) built from the
    twister results. A bare name that is unambiguous across all scenarios is
    resolved through the fallback map; otherwise the bare name is kept (it
    simply won't join with the traceability model).

    Returns (cov_by_fn, by_line) where

    * cov_by_fn[fn] = {file: sorted list of covered lines}   (tree files only)
    * by_line[file] = {line(int): [instance keys]}           (as-is, tree files)
    """
    data = json.loads(Path(matrix_path).read_text())
    slugs = sorted(((slugify(s), fns) for s, fns in qual_map.items()),
                   key=lambda kv: -len(kv[0]))  # longest slug wins

    # bare test_ name -> qualified name, when unique across all scenarios
    unique = {}
    for fns in qual_map.values():
        for bare, fn in fns.items():
            unique[bare] = fn if unique.get(bare, fn) == fn else None

    def split_key(key):
        for slug, fns in slugs:
            if key.startswith(slug + "_") and key[len(slug) + 1:].startswith("test_"):
                bare = key[len(slug) + 1:]
                return fns.get(bare) or unique.get(bare) or bare
        m = re.search(r"(test_[A-Za-z0-9_]+)$", key)
        return (unique.get(m.group(1)) or m.group(1)) if m else None

    keep = lambda f: f.startswith(("kernel/", "include/", "lib/", "tests/"))

    cov_by_fn = {}
    for key, files in data.get("by_test", {}).items():
        fn = split_key(key)
        if not fn:
            continue
        dst = cov_by_fn.setdefault(fn, {})
        for f, lines in files.items():
            if keep(f):
                dst.setdefault(f, set()).update(int(x) for x in lines)
    for fn, files in cov_by_fn.items():
        for f in files:
            files[f] = sorted(files[f])

    by_line = {}
    for f, lines in data.get("by_line", {}).items():
        if keep(f):
            by_line[f] = {int(ln): [split_key(k) or k for k in tests]
                          for ln, tests in lines.items()}
    return cov_by_fn, by_line


# --- implementation symbol resolution -----------------------------------------

def resolve_impl_symbols(root, symbols):
    """Best-effort mapping of implementing symbols to function bodies in
    kernel/**/*.c (name or z_impl_<name> definition at column 0, body ending at
    a column-0 closing brace). Returns {sym: {"file", "a", "b"}}."""
    sources = {}
    for f in glob.glob(str(root / "kernel/**/*.c"), recursive=True):
        rel = str(Path(f).relative_to(root))
        sources[rel] = Path(f).read_text(errors="ignore").split("\n")

    resolved = {}
    for sym in symbols:
        pats = [re.compile(r"^[A-Za-z_][\w \t\*]*\b" + re.escape(n) + r"\s*\(")
                for n in (f"z_impl_{sym}", sym)]
        hit = None
        for f, lines in sources.items():
            for i, ln in enumerate(lines):
                if ln.rstrip().endswith(";"):
                    continue
                if any(p.match(ln) for p in pats):
                    end = next((j + 1 for j in range(i + 1, min(i + 500, len(lines)))
                                if lines[j] == "}"), None)
                    if end:
                        hit = {"file": f, "a": i + 1, "b": end}
                        break
            if hit:
                break
        if hit:
            resolved[sym] = hit
    return resolved


# --- the extended model --------------------------------------------------------

class App:
    """Loads and joins everything; owns the request-time lookups."""

    def __init__(self, args):
        self.root = Path(args.root).resolve()

        items = json.loads(Path(args.json).read_text())
        self.model = build_model(items)

        # Split impl symbols (carry `implements`) out of the tests dict.
        self.impls, self.tests = {}, {}
        for tid, t in self.model["tests"].items():
            if t["implements"] and not t["validates"]:
                self.impls[tid] = t
            else:
                self.tests[tid] = t

        # Reverse map: requirement -> implementing symbols.
        self.req_impls = defaultdict(list)
        for sym, t in self.impls.items():
            for uid in t["implements"]:
                self.req_impls[uid].append(sym)

        self.env, self.results, qual_map = load_results(args.twister)
        self.cov, self.by_line = load_matrix(args.matrix, qual_map)
        self.impl_loc = resolve_impl_symbols(self.root, self.impls)

        self.compute()

    # -- verdicts --------------------------------------------------------------
    def evidence(self, req):
        fns = [t for t in req["tests"]]
        if not fns:
            return "untested"
        roll = [self.results[f]["rollup"] for f in fns if f in self.results]
        if not roll:
            return "no-run"
        if "failing" in roll:
            return "failing"
        if "passing" in roll:
            return "passing"
        return "skipped"

    def adequacy(self, uid, req):
        syms = self.req_impls.get(uid, [])
        if not syms:
            return {"verdict": "no-impl", "impls": []}
        detail, resolved, own_hits = [], 0, 0
        tests_with_cov = [f for f in req["tests"] if f in self.cov]
        for sym in sorted(syms):
            loc = self.impl_loc.get(sym)
            if not loc:
                detail.append({"sym": sym, "loc": None, "own": 0, "any": 0, "exec": 0})
                continue
            resolved += 1
            f, a, b = loc["file"], loc["a"], loc["b"]
            fl = self.by_line.get(f, {})
            exec_lines = [ln for ln in fl if a <= ln <= b]
            own = set()
            for t in tests_with_cov:
                own.update(ln for ln in self.cov[t].get(f, ()) if a <= ln <= b)
            if own:
                own_hits += 1
            detail.append({"sym": sym, "loc": f"{f}:{a}-{b}", "file": f, "a": a, "b": b,
                           "own": len(own), "any": len(exec_lines), "exec": len(exec_lines)})
        if not resolved:
            verdict = "unresolved"
        elif not req["tests"]:
            verdict = "no-impl"          # unreachable; kept for clarity
        elif not tests_with_cov:
            verdict = "no-cov"
        elif own_hits == resolved:
            verdict = "true"
        elif own_hits:
            verdict = "partial"
        else:
            verdict = "broken"
        return {"verdict": verdict, "impls": detail}

    def compute(self):
        m = self.model
        for uid, r in m["reqs"].items():
            r["impls"] = sorted(self.req_impls.get(uid, []))
            r["evidence"] = self.evidence(r)
            adq = self.adequacy(uid, r)
            r["adequacy"] = adq["verdict"]
            r["adq_detail"] = adq["impls"]

        # summary extensions
        ev = defaultdict(int)
        adq = defaultdict(int)
        for r in m["reqs"].values():
            ev[r["evidence"]] += 1
            adq[r["adequacy"]] += 1
        ncase = sum(len(r["instances"]) for r in self.results.values())
        m["summary"].update({
            "evidence": dict(ev), "adequacy": dict(adq),
            "run": {
                "platform": ", ".join(sorted({i["platform"] for r in self.results.values()
                                              for i in r["instances"]})),
                "zephyr": self.env.get("zephyr_version", ""),
                "date": self.env.get("run_date", ""),
                "tests": len(self.results), "cases": ncase,
                "pass": sum(r["npass"] for r in self.results.values()),
                "fail": sum(r["nfail"] for r in self.results.values()),
                "skip": sum(r["nskip"] for r in self.results.values()),
            },
            "cov": {
                "tests": len(self.cov),
                "files": len(self.by_line),
                "lines": sum(len(v) for v in self.by_line.values()),
            },
            "impl_symbols": len(self.impls),
            "impl_resolved": len(self.impl_loc),
        })

        # per-test compact info for the tests tab
        m["testinfo"] = {}
        for fn in set(self.tests) | set(self.results) | set(self.cov):
            res = self.results.get(fn)
            covd = self.cov.get(fn)
            m["testinfo"][fn] = {
                "validates": self.tests.get(fn, {}).get("validates", []),
                "caption": self.tests.get(fn, {}).get("caption", ""),
                "linked": fn in self.tests,
                "rollup": res["rollup"] if res else "no-run",
                "ninst": len(res["instances"]) if res else 0,
                "nfail": res["nfail"] if res else 0,
                "nskip": res["nskip"] if res else 0,
                "kfiles": len(covd) if covd else 0,
                "klines": sum(len(v) for v in covd.values()) if covd else 0,
            }

        # impl symbols for the model
        m["implinfo"] = {}
        for sym, t in self.impls.items():
            loc = self.impl_loc.get(sym)
            m["implinfo"][sym] = {
                "implements": t["implements"], "caption": t["caption"],
                "loc": f"{loc['file']}:{loc['a']}-{loc['b']}" if loc else None,
            }

        m["tests"] = self.tests  # without impl symbols

        # gaps, extended
        reqs = m["reqs"]
        key = srs_sortkey
        m["issues"]["broken_adequacy"] = sorted(
            (u for u, r in reqs.items() if r["adequacy"] == "broken"), key=key)
        m["issues"]["failing_evidence"] = sorted(
            (u for u, r in reqs.items() if r["evidence"] == "failing"), key=key)
        m["issues"]["skipped_only"] = sorted(
            (u for u, r in reqs.items() if r["evidence"] == "skipped"), key=key)
        m["issues"]["no_run"] = sorted(
            (u for u, r in reqs.items() if r["evidence"] == "no-run"), key=key)
        m["issues"]["no_impl"] = sorted(
            (u for u, r in reqs.items() if r["adequacy"] == "no-impl"), key=key)
        m["issues"]["unresolved_impl"] = sorted(
            s for s in self.impls if s not in self.impl_loc)
        m["issues"]["unlinked_executed"] = sorted(
            fn for fn, ti in m["testinfo"].items()
            if not ti["linked"] and ti["ninst"])
        # recompute: only symbols carrying @verifies that aren't tests are odd
        m["issues"]["non_test_verifiers"] = sorted(
            t for t, v in self.tests.items()
            if not v["is_test"] and v["validates"])

        # coverage file list
        m["covfiles"] = sorted(
            ({"file": f, "lines": len(lines)} for f, lines in self.by_line.items()),
            key=lambda x: -x["lines"])

    # -- request-time details ----------------------------------------------------
    def req_detail(self, uid):
        r = self.model["reqs"].get(uid)
        if not r:
            return None
        tests = []
        for fn in r["tests"]:
            res = self.results.get(fn)
            covd = self.cov.get(fn, {})
            tests.append({
                "fn": fn, "caption": self.tests.get(fn, {}).get("caption", ""),
                "rollup": res["rollup"] if res else "no-run",
                "instances": res["instances"] if res else [],
                "kfiles": {f: len(v) for f, v in covd.items()
                           if f.startswith(("kernel/", "include/"))},
            })
        return {"req": r, "tests": tests}

    def test_detail(self, fn):
        res = self.results.get(fn)
        covd = self.cov.get(fn, {})
        return {
            "fn": fn,
            "caption": self.tests.get(fn, {}).get("caption", ""),
            "validates": self.tests.get(fn, {}).get("validates", []),
            "instances": res["instances"] if res else [],
            "files": {f: len(v) for f, v in sorted(covd.items())},
        }

    def file_detail(self, relpath):
        # only files we have coverage or impl info for, and inside the tree
        known = set(self.by_line) | {loc["file"] for loc in self.impl_loc.values()}
        if relpath not in known:
            return None
        p = (self.root / relpath).resolve()
        if not str(p).startswith(str(self.root)) or not p.is_file():
            return None
        text = p.read_text(errors="replace").split("\n")
        fl = self.by_line.get(relpath, {})
        # line -> requirement uids (via validating tests)
        reqline = {}
        covline = {}
        for ln, fns in fl.items():
            covline[ln] = sorted(set(fns))[:40]
            uids = set()
            for f in fns:
                uids.update(self.tests.get(f, {}).get("validates", []))
            if uids:
                reqline[ln] = sorted(uids, key=srs_sortkey)[:40]
        fns_here = [{"sym": s, "a": loc["a"], "b": loc["b"]}
                    for s, loc in self.impl_loc.items() if loc["file"] == relpath]
        return {"file": relpath, "text": text, "cov": covline,
                "reqs": reqline, "impl_fns": sorted(fns_here, key=lambda x: x["a"])}


# --- HTTP server ----------------------------------------------------------------

def make_handler(app, html):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, fmt, *a):  # quiet
            pass

        def _send(self, code, body, ctype="application/json; charset=utf-8"):
            data = body if isinstance(body, bytes) else json.dumps(body).encode()
            self.send_response(code)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self):
            u = urlparse(self.path)
            q = parse_qs(u.query)
            try:
                if u.path == "/":
                    self._send(200, html.encode(), "text/html; charset=utf-8")
                elif u.path == "/api/model":
                    self._send(200, app.model)
                elif u.path.startswith("/api/req/"):
                    d = app.req_detail(u.path.rsplit("/", 1)[1])
                    self._send(200 if d else 404, d or {"error": "unknown requirement"})
                elif u.path.startswith("/api/test/"):
                    self._send(200, app.test_detail(u.path.rsplit("/", 1)[1]))
                elif u.path == "/api/file":
                    d = app.file_detail((q.get("path") or [""])[0])
                    self._send(200 if d else 404, d or {"error": "unknown file"})
                else:
                    self._send(404, {"error": "not found"})
            except BrokenPipeError:
                pass
            except Exception as e:  # pragma: no cover - defensive
                self._send(500, {"error": str(e)})
    return Handler


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--json", default="doc/_build/html/traceability.json")
    p.add_argument("--twister", default="twister.json")
    p.add_argument("--matrix", default="test_matrix.json")
    p.add_argument("--root", default=".")
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=8765)
    p.add_argument("--no-browser", action="store_true")
    args = p.parse_args(argv)

    for f in (args.json, args.twister, args.matrix):
        if not Path(f).is_file():
            p.error(f"input not found: {f}")

    print("loading traceability, results and coverage …")
    app = App(args)
    s = app.model["summary"]
    print(f"  {s['requirements']} requirements · {s['tests']} linked tests · "
          f"{s['impl_symbols']} impl symbols ({s['impl_resolved']} resolved)")
    print(f"  run: {s['run']['tests']} tests / {s['run']['cases']} cases on "
          f"{s['run']['platform']} · coverage: {s['cov']['lines']} lines in "
          f"{s['cov']['files']} files")
    print(f"  adequacy: {s['adequacy']}")

    html = _HTML.replace("__GENERATED__",
                         datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC"))
    httpd = ThreadingHTTPServer((args.host, args.port), make_handler(app, html))
    url = f"http://{args.host}:{args.port}/"
    print(f"serving on {url}   (Ctrl-C to stop)")
    if not args.no_browser:
        webbrowser.open(url)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nbye")
    return 0


_HTML = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Zephyr True Traceability</title>
<style>
:root{
  --bg:#0f1419; --panel:#1a212b; --panel2:#222c39; --line:#2d3a4a; --fg:#e6edf3;
  --muted:#8b98a5; --accent:#3b82f6;
  /* status palette validated for dark surface (OKLCH band + CVD + contrast) */
  --ok:#2ea043; --warn:#bf8700; --bad:#f85149;
  --srs:#3b82f6; --syrs:#a371f7; --test:#2ea043; --design:#bf8700; --impl:#e5534b;
}
*{box-sizing:border-box}
body{margin:0;font:14px/1.5 -apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;
  background:var(--bg);color:var(--fg)}
header{padding:12px 20px;border-bottom:1px solid var(--line);display:flex;
  align-items:baseline;gap:14px;flex-wrap:wrap;background:var(--panel)}
header h1{font-size:17px;margin:0}
header .meta{color:var(--muted);font-size:12px}
nav{display:flex;gap:2px;padding:0 12px;background:var(--panel);border-bottom:1px solid var(--line);
  position:sticky;top:0;z-index:10;flex-wrap:wrap}
nav button{background:none;border:none;color:var(--muted);padding:10px 13px;cursor:pointer;
  font-size:13px;border-bottom:2px solid transparent}
nav button:hover{color:var(--fg)} nav button.active{color:var(--fg);border-bottom-color:var(--accent)}
main{padding:16px 20px;max-width:1550px;margin:0 auto}
.tab{display:none}.tab.active{display:block}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(155px,1fr));gap:10px;margin-bottom:16px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:12px 14px}
.card .n{font-size:24px;font-weight:600}
.card .l{color:var(--muted);font-size:11px;text-transform:uppercase;letter-spacing:.04em}
.card .s{color:var(--muted);font-size:11px;margin-top:2px}
.card.ok .n{color:var(--ok)} .card.warn .n{color:var(--warn)} .card.bad .n{color:var(--bad)}
.card.click{cursor:pointer} .card.click:hover{border-color:var(--accent)}
h2{font-size:15px;border-bottom:1px solid var(--line);padding-bottom:6px;margin:20px 0 10px}
p.note{color:var(--muted);font-size:13px;max-width:900px}
table{width:100%;border-collapse:collapse;font-size:13px}
th,td{text-align:left;padding:6px 8px;border-bottom:1px solid var(--line);vertical-align:top}
th{color:var(--muted);font-weight:600;cursor:pointer;user-select:none;position:sticky;top:41px;background:var(--bg);z-index:5}
tbody tr:hover{background:var(--panel)}
.stack{display:flex;gap:2px;height:22px;border-radius:5px;overflow:hidden;margin:6px 0}
.stack>div{min-width:2px;display:flex;align-items:center;justify-content:center;
  font-size:11px;color:#fff;white-space:nowrap;overflow:hidden}
.legend{display:flex;gap:14px;flex-wrap:wrap;margin:4px 0 14px;font-size:12px;color:var(--muted)}
.dot{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:5px;vertical-align:middle}
.st{display:inline-block;padding:0 7px;border-radius:9px;font-size:11px;font-weight:600;border:1px solid}
.st-passing{color:var(--ok);border-color:var(--ok)} .st-failing{color:var(--bad);border-color:var(--bad)}
.st-skipped{color:var(--warn);border-color:var(--warn)}
.st-no-run,.st-untested{color:var(--muted);border-color:var(--line)}
.st-true{color:var(--ok);border-color:var(--ok)} .st-partial{color:var(--warn);border-color:var(--warn)}
.st-broken{color:var(--bad);border-color:var(--bad);background:#f8514922}
.st-unresolved,.st-no-impl,.st-no-cov{color:var(--muted);border-color:var(--line)}
.tag{display:inline-block;font-size:11px;padding:0 6px;border-radius:4px;background:var(--panel2);
  margin:1px 2px;color:var(--muted);cursor:default}
.tag.t{color:var(--test)} .tag.d{color:var(--design)} .tag.s{color:var(--syrs)} .tag.i{color:var(--impl)}
.tag.lnk{cursor:pointer} .tag.lnk:hover{outline:1px solid var(--accent)}
.controls{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-bottom:10px}
input[type=search],select{background:var(--panel);border:1px solid var(--line);color:var(--fg);
  padding:7px 10px;border-radius:6px;font-size:13px}
input[type=search]{min-width:230px}
.muted{color:var(--muted)} .expand{cursor:pointer;color:var(--accent)}
.detail{background:var(--panel);font-size:12px} .detail td{padding:10px 14px}
.detail h4{margin:8px 0 4px;font-size:12px;color:var(--muted);text-transform:uppercase}
code,.mono{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:12px}
code{background:var(--panel2);padding:1px 5px;border-radius:4px}
a{color:var(--accent);text-decoration:none}a:hover{text-decoration:underline}
details{margin:6px 0}summary{cursor:pointer;color:var(--accent)}
.copybtn{background:var(--panel2);border:1px solid var(--line);color:var(--fg);border-radius:6px;
  padding:3px 9px;font-size:12px;cursor:pointer}
.copybtn:hover{border-color:var(--accent)}
/* coverage viewer */
.covwrap{display:grid;grid-template-columns:330px 1fr;gap:14px;align-items:start}
.filelist{max-height:75vh;overflow:auto;background:var(--panel);border:1px solid var(--line);border-radius:8px}
.filelist div{padding:6px 10px;font-size:12px;cursor:pointer;border-bottom:1px solid var(--line);
  display:flex;justify-content:space-between;gap:8px}
.filelist div:hover,.filelist div.sel{background:var(--panel2)}
.src{background:var(--panel);border:1px solid var(--line);border-radius:8px;max-height:75vh;overflow:auto}
.src .hdr{position:sticky;top:0;background:var(--panel2);padding:8px 12px;font-size:13px;z-index:2;
  display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap}
.srcline{display:flex;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:12px;line-height:1.45}
.srcline:hover{background:#ffffff0a}
.srcline .no{width:56px;text-align:right;padding-right:10px;color:var(--muted);user-select:none;flex:none}
.srcline .gut{width:8px;flex:none}
.srcline.cov .gut{background:var(--ok)}
.srcline.cov{background:#2ea04314;cursor:pointer}
.srcline.hl{outline:1.5px solid var(--accent);outline-offset:-1.5px}
.srcline pre{margin:0;white-space:pre;overflow-x:visible;padding-left:8px}
.fnmark{border-left:3px solid var(--impl)}
.linepop{position:fixed;background:#0b0f14f2;border:1px solid var(--line);border-radius:8px;
  padding:10px 12px;font-size:12px;max-width:520px;z-index:60;max-height:340px;overflow:auto}
#gcanvas{width:100%;height:640px;background:var(--panel);border:1px solid var(--line);border-radius:8px;
  display:block;cursor:grab}
.gtip{position:fixed;pointer-events:none;background:#000d;border:1px solid var(--line);padding:6px 9px;
  border-radius:6px;font-size:12px;max-width:340px;display:none;z-index:50}
.bar{height:9px;border-radius:5px;background:var(--panel2);overflow:hidden;min-width:80px}
.bar>span{display:block;height:100%;background:var(--ok)}
.bar.low>span{background:var(--bad)} .bar.mid>span{background:var(--warn)}
</style>
</head>
<body>
<header>
  <h1>Zephyr True Traceability</h1>
  <span class="meta" id="meta"></span>
</header>
<nav id="nav"></nav>
<main>
  <div class="tab active" id="tab-overview"></div>
  <div class="tab" id="tab-adequacy"></div>
  <div class="tab" id="tab-requirements"></div>
  <div class="tab" id="tab-tests"></div>
  <div class="tab" id="tab-coverage"></div>
  <div class="tab" id="tab-gaps"></div>
  <div class="tab" id="tab-graph"></div>
</main>
<div class="gtip" id="gtip"></div>
<script>
let M=null;
const el=h=>{const t=document.createElement('template');t.innerHTML=h.trim();return t.content.firstChild;};
const esc=s=>(s||'').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const pct=(n,d)=>d?Math.round(100*n/d):0;
const api=async p=>{const r=await fetch(p);if(!r.ok)throw new Error(p);return r.json();};
const chip=(v)=>`<span class="st st-${esc(v)}">${esc(v)}</span>`;
const EV_COLORS={passing:'var(--ok)',failing:'var(--bad)',skipped:'var(--warn)','no-run':'#5a6b7d',untested:'#39434f'};
const ADQ_COLORS={true:'var(--ok)',partial:'var(--warn)',broken:'var(--bad)',unresolved:'#5a6b7d','no-impl':'#39434f','no-cov':'#4a5764'};
const ADQ_HELP={
 'true':'every resolved implementing function is exercised by the requirement’s own verifying tests',
 partial:'some implementing functions are exercised by the verifying tests, others are not',
 broken:'the verifying tests never execute the implementing code — verification does not reach the implementation',
 unresolved:'implementation links exist but map to macros/inlines, not function bodies',
 'no-impl':'the requirement has no implemented_by link (@satisfies)',
 'no-cov':'the verifying tests have no coverage data in this run'};

const TABS=[['overview','Overview'],['adequacy','True traceability'],
 ['requirements','Requirements'],['tests','Tests'],['coverage','Coverage browser'],
 ['gaps','Gaps & issues'],['graph','Graph']];

function tab(id){document.querySelectorAll('nav button').forEach(x=>x.classList.toggle('active',x.dataset.t===id));
 document.querySelectorAll('.tab').forEach(x=>x.classList.toggle('active',x.id==='tab-'+id));
 if(id==='graph')Graph.ensure();}

async function boot(){
 M=await api('/api/model');
 document.getElementById('meta').textContent=
  `${M.summary.requirements} requirements · ${M.summary.tests} tests · run: ${M.summary.run.platform} `+
  `(${M.summary.run.zephyr}) · generated __GENERATED__`;
 const nav=document.getElementById('nav');
 TABS.forEach(([id,label],i)=>{const b=el(`<button data-t="${id}">${label}</button>`);
  if(i===0)b.classList.add('active');b.onclick=()=>tab(id);nav.appendChild(b);});
 Overview();Adequacy();Requirements();Tests();Coverage();Gaps();
}
boot();

// ---------- shared renderers ----------
function stackBar(counts,colors,order,onclick){
 const total=order.reduce((a,k)=>a+(counts[k]||0),0)||1;
 let seg='',leg='';
 for(const k of order){const v=counts[k]||0;if(!v)continue;
  const w=Math.max(2,100*v/total);
  seg+=`<div style="background:${colors[k]};flex:0 0 ${w}%" title="${k}: ${v}"
    ${onclick?`onclick="${onclick}('${k}')"`:''} role="img" aria-label="${k}: ${v}">${w>7?`${v}`:''}</div>`;
  leg+=`<span><span class="dot" style="background:${colors[k]}"></span>${k}: <b>${v}</b></span>`;}
 return `<div class="stack" ${onclick?'style="cursor:pointer"':''}>${seg}</div><div class="legend">${leg}</div>`;
}
function goto(tabId,cb){tab(tabId);if(cb)cb();}

// ---------- overview ----------
function Overview(){
 const s=M.summary,r=s.run,c=s.cov;
 const adqTrue=(s.adequacy['true']||0),adqBroken=(s.adequacy.broken||0);
 const card=(n,l,sub,cls,click)=>`<div class="card ${cls||''} ${click?'click':''}" ${click?`onclick="${click}"`:''}>
   <div class="n">${n}</div><div class="l">${l}</div>${sub?`<div class="s">${sub}</div>`:''}</div>`;
 let h=`<div class="cards">
  ${card(s.requirements,'Requirements',`${s.system_requirements} system`)}
  ${card(s.tested_pct+'%','Verified by tests',`${s.tested}/${s.requirements}`,s.tested_pct>=80?'ok':s.tested_pct>=40?'warn':'bad')}
  ${card(adqTrue,'True traceability','impl exercised by own tests',adqTrue?'ok':'','tab(\'adequacy\')')}
  ${card(adqBroken,'Broken traceability','tests never reach impl',adqBroken?'bad':'ok','tab(\'adequacy\')')}
  ${card(r.pass,'Cases passed',`${r.fail} failed · ${r.skip} skipped`,r.fail?'warn':'ok','tab(\'tests\')')}
  ${card(c.lines.toLocaleString(),'Covered lines',`${c.files} files · ${c.tests} tests`,'','tab(\'coverage\')')}
 </div>`;
 h+=`<h2>Verification evidence <span class="muted" style="font-weight:400">— requirement status from the twister run</span></h2>`;
 h+=stackBar(s.evidence,EV_COLORS,['passing','failing','skipped','no-run','untested'],'evFilter');
 h+=`<h2>True traceability <span class="muted" style="font-weight:400">— do verifying tests exercise the implementing code?</span></h2>`;
 h+=stackBar(s.adequacy,ADQ_COLORS,['true','partial','broken','unresolved','no-cov','no-impl'],'adqFilter');
 h+=`<p class="note">Implementation linkage: <b>${s.impl_symbols}</b> symbols carry <code>@satisfies</code>;
   <b>${s.impl_resolved}</b> resolve to function bodies in <code>kernel/**/*.c</code>
   (the rest are macros or inlines). A requirement has <i>true traceability</i> when the union of its
   verifying tests’ line coverage intersects every resolved implementing function.</p>`;
 h+=`<h2>Test run</h2><p class="note">${esc(r.zephyr)} on <b>${esc(r.platform)}</b>, ${esc(r.date)} —
   ${r.tests} test functions, ${r.cases} case instances.</p>`;
 document.getElementById('tab-overview').innerHTML=h;
}
function evFilter(v){goto('requirements',()=>{document.getElementById('reqev').value=v;renderReqs();});}
function adqFilter(v){goto('adequacy',()=>{document.getElementById('adqsel').value=v;renderAdq();});}

// ---------- true traceability ----------
let adqSort={k:'verdict',d:1};
function Adequacy(){
 const host=document.getElementById('tab-adequacy');
 host.innerHTML=`
 <p class="note"><b>True traceability:</b> a requirement is only genuinely verified when the tests that
  <code>@verifies</code> it actually execute the code that <code>@satisfies</code> it. Below, each requirement’s
  implementing functions are resolved to <code>kernel/**/*.c</code> line ranges and intersected with the line
  coverage of its verifying tests from this twister run.</p>
 <div class="controls">
  <input type="search" id="adqsearch" placeholder="Search UID, title, symbol…">
  <select id="adqsel"><option value="">All verdicts</option>
   ${Object.keys(ADQ_HELP).map(k=>`<option value="${k}">${k} — ${ADQ_HELP[k].slice(0,40)}…</option>`).join('')}</select>
  <span class="muted" id="adqcount"></span>
 </div>
 <table><thead><tr><th data-k="id">UID</th><th data-k="title">Title</th><th data-k="component">Component</th>
  <th data-k="evidence">Evidence</th><th data-k="nimpl">Impl fns</th><th data-k="hit">Exercised by own tests</th>
  <th data-k="verdict">Verdict</th></tr></thead><tbody id="adqbody"></tbody></table>`;
 document.getElementById('adqsearch').addEventListener('input',renderAdq);
 document.getElementById('adqsel').addEventListener('change',renderAdq);
 host.querySelectorAll('th').forEach(th=>th.onclick=()=>{const k=th.dataset.k;
  if(adqSort.k===k)adqSort.d*=-1;else adqSort={k,d:1};renderAdq();});
 renderAdq();
}
const ADQ_RANK={broken:0,partial:1,'true':2,'no-cov':3,unresolved:4,'no-impl':5};
function renderAdq(){
 const q=(document.getElementById('adqsearch').value||'').toLowerCase();
 const sel=document.getElementById('adqsel').value;
 let rows=Object.values(M.reqs).filter(r=>r.impls.length||sel===''||sel==='no-impl');
 rows=rows.filter(r=>!sel||r.adequacy===sel);
 if(!sel)rows=rows.filter(r=>r.adequacy!=='no-impl'); // default view: only reqs with impl links
 if(q)rows=rows.filter(r=>r.id.toLowerCase().includes(q)||r.title.toLowerCase().includes(q)
   ||r.impls.some(s=>s.toLowerCase().includes(q)));
 const hit=r=>r.adq_detail.filter(d=>d.own>0).length;
 const val=r=>({id:r.id,title:r.title,component:r.component,evidence:r.evidence,
   nimpl:r.impls.length,hit:hit(r),verdict:ADQ_RANK[r.adequacy]??9}[adqSort.k]);
 rows.sort((a,b)=>{const x=val(a),y=val(b);return (x<y?-1:x>y?1:0)*adqSort.d
   ||a.id.localeCompare(b.id,undefined,{numeric:true});});
 document.getElementById('adqcount').textContent=`${rows.length} requirements`;
 const body=document.getElementById('adqbody');body.innerHTML='';
 for(const r of rows){
  const res=r.adq_detail.filter(d=>d.loc);
  const nhit=res.filter(d=>d.own>0).length;
  const tr=el(`<tr><td><span class="expand">${r.id}</span></td><td>${esc(r.title)}</td>
   <td>${esc(r.component)}</td><td>${chip(r.evidence)}</td>
   <td>${r.impls.length}${res.length<r.impls.length?` <span class="muted">(${r.impls.length-res.length} unresolved)</span>`:''}</td>
   <td>${res.length?`${nhit}/${res.length}`:'<span class="muted">—</span>'}</td>
   <td title="${esc(ADQ_HELP[r.adequacy]||'')}">${chip(r.adequacy)}</td></tr>`);
  tr.querySelector('.expand').onclick=()=>toggleAdqDetail(tr,r);
  body.appendChild(tr);
 }
}
async function toggleAdqDetail(tr,r){
 if(tr.nextSibling&&tr.nextSibling.classList&&tr.nextSibling.classList.contains('detail')){tr.nextSibling.remove();return;}
 const d=await api('/api/req/'+r.id);
 let impls='';
 for(const im of r.adq_detail){
  impls+=`<tr><td class="mono">${im.sym}</td>
   <td>${im.loc?`<a href="#" onclick="openSource('${im.file}',${im.a},${im.b});return false" class="mono">${im.loc}</a>`
       :'<span class="muted">unresolved (macro/inline)</span>'}</td>
   <td>${im.loc?`${im.own} <span class="muted">of ${im.exec} covered-in-run</span>`:'—'}</td>
   <td>${im.loc?(im.own>0?chip('true'):chip('broken')):chip('unresolved')}</td></tr>`;
 }
 let tests='';
 for(const t of d.tests){
  const files=Object.entries(t.kfiles).map(([f,n])=>`<span class="tag lnk mono" onclick="openSource('${f}')">${f} <b>${n}</b></span>`).join('')||'<span class="muted">no kernel coverage</span>';
  tests+=`<tr><td class="mono">${t.fn}</td><td>${chip(t.rollup)}</td>
   <td>${t.instances.length}</td><td>${files}</td></tr>`;
 }
 const det=el(`<tr class="detail"><td colspan="7">
  <h4>Implementing functions (${r.impls.length}) — lines covered by this requirement's own tests</h4>
  <table><thead><tr><th>Symbol</th><th>Location</th><th>Own-test coverage</th><th>Verdict</th></tr></thead>
  <tbody>${impls}</tbody></table>
  <h4>Verifying tests (${d.tests.length})</h4>
  <table><thead><tr><th>Test</th><th>Result</th><th>Instances</th><th>Kernel files covered (lines)</th></tr></thead>
  <tbody>${tests}</tbody></table></td></tr>`);
 tr.after(det);
}

// ---------- requirements explorer ----------
let reqSort={k:'id',d:1};
function Requirements(){
 const reqs=Object.values(M.reqs);
 const comps=[...new Set(reqs.map(r=>r.component))].sort();
 document.getElementById('tab-requirements').innerHTML=`
 <div class="controls">
  <input type="search" id="reqsearch" placeholder="Search UID, title, test, symbol…">
  <select id="reqcomp"><option value="">All components</option>${comps.map(c=>`<option>${esc(c)}</option>`).join('')}</select>
  <select id="reqev"><option value="">Any evidence</option>
   <option>passing</option><option>failing</option><option>skipped</option><option>no-run</option><option>untested</option></select>
  <select id="reqadq"><option value="">Any traceability</option>
   ${Object.keys(ADQ_HELP).map(k=>`<option>${k}</option>`).join('')}</select>
  <span class="muted" id="reqcount"></span>
 </div>
 <table id="reqtable"><thead><tr>
  <th data-k="id">UID</th><th data-k="title">Title</th><th data-k="component">Component</th>
  <th data-k="ntests">Tests</th><th data-k="evidence">Evidence</th>
  <th data-k="nimpl">Impl</th><th data-k="adequacy">Traceability</th><th data-k="ndesign">Design</th>
 </tr></thead><tbody id="reqbody"></tbody></table>`;
 ['reqsearch','reqcomp','reqev','reqadq'].forEach(id=>{
  const e=document.getElementById(id);
  e.addEventListener(id==='reqsearch'?'input':'change',renderReqs);});
 document.querySelectorAll('#reqtable th').forEach(th=>th.onclick=()=>{const k=th.dataset.k;
  if(reqSort.k===k)reqSort.d*=-1;else reqSort={k,d:1};renderReqs();});
 renderReqs();
}
function renderReqs(){
 const q=(document.getElementById('reqsearch').value||'').toLowerCase();
 const c=document.getElementById('reqcomp').value, ev=document.getElementById('reqev').value,
       adq=document.getElementById('reqadq').value;
 let rows=Object.values(M.reqs).filter(r=>
  (!c||r.component===c)&&(!ev||r.evidence===ev)&&(!adq||r.adequacy===adq)&&
  (!q||r.id.toLowerCase().includes(q)||r.title.toLowerCase().includes(q)
    ||r.tests.some(t=>t.toLowerCase().includes(q))||r.impls.some(s=>s.toLowerCase().includes(q))));
 const val=r=>({id:r.id,title:r.title,component:r.component,ntests:r.tests.length,
  evidence:r.evidence,nimpl:r.impls.length,adequacy:ADQ_RANK[r.adequacy]??9,ndesign:r.design.length}[reqSort.k]);
 rows.sort((a,b)=>{const x=val(a),y=val(b);return (x<y?-1:x>y?1:0)*reqSort.d
  ||a.id.localeCompare(b.id,undefined,{numeric:true});});
 document.getElementById('reqcount').textContent=`${rows.length} of ${Object.keys(M.reqs).length}`;
 const body=document.getElementById('reqbody');body.innerHTML='';
 for(const r of rows){
  const tr=el(`<tr><td><span class="expand">${r.id}</span></td><td>${esc(r.title)}</td>
   <td>${esc(r.component)}</td>
   <td style="color:${r.tests.length?'var(--ok)':'var(--bad)'}">${r.tests.length}</td>
   <td>${chip(r.evidence)}</td>
   <td style="color:${r.impls.length?'var(--fg)':'var(--muted)'}">${r.impls.length}</td>
   <td title="${esc(ADQ_HELP[r.adequacy]||'')}">${chip(r.adequacy)}</td>
   <td style="color:${r.design.length?'var(--fg)':'var(--muted)'}">${r.design.length}</td></tr>`);
  tr.querySelector('.expand').onclick=()=>toggleReqDetail(tr,r);
  body.appendChild(tr);
 }
}
async function toggleReqDetail(tr,r){
 if(tr.nextSibling&&tr.nextSibling.classList&&tr.nextSibling.classList.contains('detail')){tr.nextSibling.remove();return;}
 const d=await api('/api/req/'+r.id);
 let tests='';
 for(const t of d.tests){
  const inst=t.instances.map(i=>`<span class="tag" title="${esc(i.suite)}">${esc(i.platform)}: <b style="color:${i.status==='passed'?'var(--ok)':(i.status==='failed'||i.status==='error')?'var(--bad)':'var(--warn)'}">${esc(i.status)}</b></span>`).join('')||'<span class="muted">not executed in this run</span>';
  tests+=`<div style="margin:4px 0"><span class="tag t mono">${t.fn}</span> ${chip(t.rollup)} ${inst}</div>`;
 }
 const impls=r.impls.map(s=>{const ii=M.implinfo[s]||{};
  return `<span class="tag i mono lnk" title="${esc(ii.caption||'')}" ${ii.loc?`onclick="openSource('${ii.loc.split(':')[0]}',${ii.loc.split(':')[1].split('-')[0]},${ii.loc.split(':')[1].split('-')[1]})"`:''}>${s}${ii.loc?'':' (macro)'}</span>`;}).join('')||'<span class="muted">none</span>';
 const des=r.design.map(x=>`<span class="tag d">${x}</span>`).join('')||'<span class="muted">none</span>';
 const par=r.parents.map(x=>`<span class="tag s">${x}</span>`).join('')||'<span class="muted">none</span>';
 const det=el(`<tr class="detail"><td colspan="8">
  <div class="muted">${esc(r.title)} — ${esc(r.component)} · ${esc(r.status)} · traceability: ${chip(r.adequacy)}</div>
  <h4>Verifying tests (${d.tests.length})</h4>${tests||'<span class="muted">none</span>'}
  <h4>Implementing symbols (${r.impls.length})</h4>${impls}
  <h4>Design</h4>${des} <h4>Parents</h4>${par}</td></tr>`);
 tr.after(det);
}

// ---------- tests ----------
let tSort={k:'rollup',d:1};
const ROLL_RANK={failing:0,skipped:1,'no-run':2,passing:3};
function Tests(){
 document.getElementById('tab-tests').innerHTML=`
 <div class="controls">
  <input type="search" id="tsearch" placeholder="Search test name / requirement…">
  <select id="troll"><option value="">Any result</option>
   <option>passing</option><option>failing</option><option>skipped</option><option>no-run</option></select>
  <select id="tlink"><option value="">Linked & unlinked</option>
   <option value="linked">Linked to requirements</option><option value="unlinked">No requirement link</option></select>
  <span class="muted" id="tcount"></span>
 </div>
 <table id="ttable"><thead><tr>
  <th data-k="fn">Test</th><th data-k="nval">Verifies</th><th data-k="rollup">Result</th>
  <th data-k="ninst">Instances</th><th data-k="klines">Kernel lines covered</th>
 </tr></thead><tbody id="tbody"></tbody></table>`;
 ['tsearch','troll','tlink'].forEach(id=>{const e=document.getElementById(id);
  e.addEventListener(id==='tsearch'?'input':'change',renderTests);});
 document.querySelectorAll('#ttable th').forEach(th=>th.onclick=()=>{const k=th.dataset.k;
  if(tSort.k===k)tSort.d*=-1;else tSort={k,d:1};renderTests();});
 renderTests();
}
function renderTests(){
 const q=(document.getElementById('tsearch').value||'').toLowerCase();
 const roll=document.getElementById('troll').value, lnk=document.getElementById('tlink').value;
 let rows=Object.entries(M.testinfo).map(([fn,t])=>({fn,...t,nval:t.validates.length}));
 rows=rows.filter(t=>(!roll||t.rollup===roll)
  &&(lnk!=='linked'||t.linked)&&(lnk!=='unlinked'||!t.linked)
  &&(!q||t.fn.toLowerCase().includes(q)||t.validates.some(u=>u.toLowerCase().includes(q))));
 const val=t=>({fn:t.fn,nval:t.nval,rollup:ROLL_RANK[t.rollup]??9,ninst:t.ninst,klines:t.klines}[tSort.k]);
 rows.sort((a,b)=>{const x=val(a),y=val(b);return (x<y?-1:x>y?1:0)*tSort.d||a.fn.localeCompare(b.fn);});
 document.getElementById('tcount').textContent=`${rows.length} tests`;
 const body=document.getElementById('tbody');body.innerHTML='';
 const frag=document.createDocumentFragment();
 for(const t of rows.slice(0,800)){
  const tr=el(`<tr><td><span class="expand mono">${t.fn}</span>${t.linked?'':' <span class="tag">unlinked</span>'}</td>
   <td>${t.validates.map(u=>`<span class="tag lnk" onclick="focusReq('${u}')">${u}</span>`).join('')||'<span class="muted">—</span>'}</td>
   <td>${chip(t.rollup)}${t.nfail?` <span class="muted">${t.nfail}f</span>`:''}${t.nskip?` <span class="muted">${t.nskip}s</span>`:''}</td>
   <td>${t.ninst}</td><td>${t.klines?`${t.klines} <span class="muted">in ${t.kfiles} files</span>`:'<span class="muted">—</span>'}</td></tr>`);
  tr.querySelector('.expand').onclick=()=>toggleTestDetail(tr,t.fn);
  frag.appendChild(tr);
 }
 body.appendChild(frag);
 if(rows.length>800)body.appendChild(el(`<tr><td colspan="5" class="muted">…showing first 800; refine the search.</td></tr>`));
}
async function toggleTestDetail(tr,fn){
 if(tr.nextSibling&&tr.nextSibling.classList&&tr.nextSibling.classList.contains('detail')){tr.nextSibling.remove();return;}
 const d=await api('/api/test/'+fn);
 const inst=d.instances.map(i=>`<tr><td class="mono">${esc(i.suite)}</td><td>${esc(i.platform)}</td>
   <td><b style="color:${i.status==='passed'?'var(--ok)':(i.status==='failed'||i.status==='error')?'var(--bad)':'var(--warn)'}">${esc(i.status)}</b></td>
   <td>${esc(i.time)}s</td></tr>`).join('')||'<tr><td colspan="4" class="muted">not executed</td></tr>';
 const files=Object.entries(d.files).filter(([f])=>f.startsWith('kernel/')||f.startsWith('include/'))
  .map(([f,n])=>`<span class="tag lnk mono" onclick="openSource('${f}')">${f} <b>${n}</b></span>`).join('')||'<span class="muted">none</span>';
 const det=el(`<tr class="detail"><td colspan="5">
  <div class="muted">${esc(d.caption)}</div>
  <h4>Instances</h4><table><thead><tr><th>Suite</th><th>Platform</th><th>Status</th><th>Time</th></tr></thead><tbody>${inst}</tbody></table>
  <h4>Kernel / include coverage</h4>${files}</td></tr>`);
 tr.after(det);
}
function focusReq(uid){goto('requirements',()=>{const s=document.getElementById('reqsearch');
 s.value=uid;document.getElementById('reqev').value='';document.getElementById('reqadq').value='';
 document.getElementById('reqcomp').value='';renderReqs();});}

// ---------- coverage browser ----------
let curFile=null;
function Coverage(){
 const list=M.covfiles.map(f=>`<div data-f="${f.file}" onclick="openSource('${f.file}')">
   <span class="mono">${f.file}</span><span class="muted">${f.lines}</span></div>`).join('');
 document.getElementById('tab-coverage').innerHTML=`
 <p class="note">Files with line coverage from this run. Green lines were executed by at least one test —
  click one to see which tests ran it and which requirements those tests verify.
  Red left borders mark resolved implementing functions (<code>@satisfies</code>).</p>
 <div class="covwrap"><div class="filelist" id="filelist">${list}</div>
 <div class="src" id="srcpane"><div class="hdr"><span class="muted">select a file…</span></div></div></div>`;
}
async function openSource(path,a,b){
 goto('coverage');
 document.querySelectorAll('.filelist div').forEach(x=>x.classList.toggle('sel',x.dataset.f===path));
 const pane=document.getElementById('srcpane');
 pane.innerHTML=`<div class="hdr"><span>loading ${esc(path)}…</span></div>`;
 let d;try{d=await api('/api/file?path='+encodeURIComponent(path));}
 catch(e){pane.innerHTML=`<div class="hdr"><span style="color:var(--bad)">no data for ${esc(path)}</span></div>`;return;}
 curFile=d;
 const fnAt=n=>d.impl_fns.find(f=>f.a<=n&&n<=f.b);
 let rows='';
 for(let i=0;i<d.text.length;i++){
  const n=i+1,cov=d.cov[n],fn=fnAt(n),hl=a&&b&&n>=a&&n<=b;
  rows+=`<div class="srcline${cov?' cov':''}${hl?' hl':''}${fn?' fnmark':''}" data-n="${n}" ${cov?`onclick="linePop(event,${n})"`:''}>
   <span class="no">${n}</span><span class="gut"></span><pre>${esc(d.text[i])||' '}</pre></div>`;
 }
 const nfn=d.impl_fns.length,ncov=Object.keys(d.cov).length;
 pane.innerHTML=`<div class="hdr"><span class="mono">${esc(path)}</span>
  <span class="muted">${ncov} covered lines · ${nfn} implementing fn${nfn===1?'':'s'}${
   d.impl_fns.map(f=>` · <a href="#" onclick="openSource('${path}',${f.a},${f.b});return false" class="mono">${f.sym}</a>`).join('')}</span></div>${rows}`;
 if(a){const t=pane.querySelector(`[data-n="${a}"]`);if(t)t.scrollIntoView({block:'center'});}
}
function linePop(ev,n){
 document.querySelectorAll('.linepop').forEach(x=>x.remove());
 const tests=(curFile.cov[n]||[]);
 const reqs=(curFile.reqs[n]||[]);
 const pop=el(`<div class="linepop">
  <div><b>${esc(curFile.file)}:${n}</b> <button class="copybtn" style="float:right" onclick="this.closest('.linepop').remove()">×</button></div>
  <h4 style="margin:8px 0 3px;color:var(--muted);font-size:11px">EXECUTED BY ${tests.length} TEST${tests.length===1?'':'S'}</h4>
  ${tests.map(t=>`<span class="tag t mono">${esc(t)}</span>`).join('')||'<span class="muted">—</span>'}
  <h4 style="margin:8px 0 3px;color:var(--muted);font-size:11px">VERIFIES REQUIREMENTS</h4>
  ${reqs.map(u=>`<span class="tag lnk" onclick="focusReq('${u}')">${u}</span>`).join('')||'<span class="muted">these tests carry no @verifies</span>'}
 </div>`);
 document.body.appendChild(pop);
 const y=Math.min(ev.clientY+10,window.innerHeight-Math.min(340,pop.offsetHeight)-10);
 pop.style.left=Math.min(ev.clientX+10,window.innerWidth-540)+'px';pop.style.top=y+'px';
 setTimeout(()=>document.addEventListener('click',function h(e){
  if(!pop.contains(e.target)){pop.remove();document.removeEventListener('click',h);}},{once:false}),0);
}

// ---------- gaps ----------
function Gaps(){
 const I=M.issues;
 const rlink=u=>`<span class="tag lnk" style="color:var(--srs)" onclick="focusReq('${u}')">${u}</span>`;
 const plain=u=>`<span class="tag mono">${esc(u)}</span>`;
 const section=(title,desc,ids,linkify)=>{
  if(!ids||!ids.length)return `<details><summary>${title} — <span style="color:var(--ok)">0</span> ✓</summary><p class="note">${desc}</p></details>`;
  return `<details open><summary>${title} — <span style="color:var(--bad)">${ids.length}</span></summary>
   <p class="note">${desc} <button class="copybtn" onclick='navigator.clipboard.writeText(${JSON.stringify(ids.join("\n"))})'>copy ${ids.length}</button></p>
   <div style="line-height:2.1">${ids.map(linkify).join(' ')}</div></details>`;};
 let h=`<p class="note">Ranked by risk: the first sections are requirements whose verification is an illusion.</p>`;
 h+=section('Broken true traceability','Verifying tests exist and ran with coverage, but <b>never execute the implementing code</b>.',I.broken_adequacy,rlink);
 h+=section('Verified but failing','At least one verifying test failed in this run.',I.failing_evidence,rlink);
 h+=section('Verified but skipped everywhere','All verifying tests were skipped/blocked in this run — no execution evidence.',I.skipped_only,rlink);
 h+=section('Verified but never executed','Linked tests were not part of this twister run.',I.no_run,rlink);
 h+=section('No verifying test',"No <code>@verifies</code> links.",I.untested,rlink);
 h+=section('No implementation link','No <code>@satisfies</code> on any symbol (or only on macros, which Doxygen exports as defines).',I.no_impl,rlink);
 h+=section('Implementation symbols not resolved','Carry <code>@satisfies</code> but map to macros/inlines, not kernel/*.c function bodies.',I.unresolved_impl,plain);
 h+=section('Executed but unlinked tests','Ran in this twister run but verify no requirement (top candidates for new @verifies).',(I.unlinked_executed||[]).slice(0,120),plain);
 h+=section('No design link','No design directive fulfils these.',I.undesigned,rlink);
 h+=section('Dangling links','Relationship targets that resolve to nothing.',I.dangling,plain);
 document.getElementById('tab-gaps').innerHTML=h;
}

// ---------- graph ----------
const Graph=(function(){
 const cv=el('<canvas id="gcanvas"></canvas>');
 let built=false,nodes=[],links=[],byId={},raf=0,alpha=0,tx=0,ty=0,scale=1,drag=null,pan=null,hover=null,sel=null,dpr=1;
 const COLOR={srs:'#3b82f6',syrs:'#a371f7',test:'#2ea043',design:'#bf8700',impl:'#e5534b'};
 function host(){return document.getElementById('tab-graph');}
 function ui(){
  const comps=[...new Set(Object.values(M.reqs).map(r=>r.component))].sort();
  host().innerHTML=`<div class="controls">
   <select id="gscope">${comps.map(c=>`<option value="${esc(c)}">Component: ${esc(c)}</option>`).join('')}</select>
   <span class="muted">drag · scroll to zoom · click to highlight neighbours</span></div>
  <div class="legend">
   <span><span class="dot" style="background:${COLOR.srs}"></span>requirement</span>
   <span><span class="dot" style="background:${COLOR.syrs}"></span>system req</span>
   <span><span class="dot" style="background:${COLOR.test}"></span>test</span>
   <span><span class="dot" style="background:${COLOR.design}"></span>design</span>
   <span><span class="dot" style="background:${COLOR.impl}"></span>implementation</span>
  </div>`;
  host().appendChild(cv);
  document.getElementById('gscope').onchange=e=>build(e.target.value);
  bind();
 }
 function add(id,kind,label){if(!byId[id]){const n={id,kind,label,x:(Math.random()-.5)*500,y:(Math.random()-.5)*350,vx:0,vy:0,deg:0};byId[id]=n;nodes.push(n);}return byId[id];}
 function link(a,b){if(byId[a]&&byId[b]){links.push({s:byId[a],t:byId[b]});byId[a].deg++;byId[b].deg++;}}
 function build(comp){
  nodes=[];links=[];byId={};sel=null;
  for(const r of Object.values(M.reqs)){
   if(r.component!==comp)continue;
   add(r.id,'srs',r.id);
   r.tests.forEach(t=>{add(t,'test',t);link(r.id,t);});
   r.impls.forEach(s=>{add(s,'impl',s);link(r.id,s);});
   r.design.forEach(x=>{add(x,'design',x);link(r.id,x);});
   r.parents.forEach(p=>{add(p,'syrs',p);link(r.id,p);});
  }
  nodes.forEach((n,i)=>{const a=i/nodes.length*6.283;n.x=Math.cos(a)*240;n.y=Math.sin(a)*240;});
  fit();alpha=1;tick();
 }
 function fit(){const r=cv.getBoundingClientRect();dpr=window.devicePixelRatio||1;
  cv.width=r.width*dpr;cv.height=r.height*dpr;scale=1;tx=cv.width/(2*dpr);ty=cv.height/(2*dpr);}
 function step(){const k=110,rep=2100;
  for(let i=0;i<nodes.length;i++){const a=nodes[i];
   for(let j=i+1;j<nodes.length;j++){const b=nodes[j];
    let dx=a.x-b.x,dy=a.y-b.y,d2=dx*dx+dy*dy+.01,d=Math.sqrt(d2),f=rep/d2,fx=dx/d*f,fy=dy/d*f;
    a.vx+=fx;a.vy+=fy;b.vx-=fx;b.vy-=fy;}}
  for(const l of links){let dx=l.t.x-l.s.x,dy=l.t.y-l.s.y,d=Math.sqrt(dx*dx+dy*dy)+.01,f=(d-k)*.02,fx=dx/d*f,fy=dy/d*f;
   l.s.vx+=fx;l.s.vy+=fy;l.t.vx-=fx;l.t.vy-=fy;}
  for(const n of nodes){if(n===drag)continue;n.vx-=n.x*.0012;n.vy-=n.y*.0012;
   n.x+=n.vx*alpha;n.y+=n.vy*alpha;n.vx*=.86;n.vy*=.86;}
  alpha*=.985;}
 function draw(){const ctx=cv.getContext('2d');ctx.setTransform(dpr,0,0,dpr,0,0);
  ctx.clearRect(0,0,cv.width,cv.height);ctx.save();ctx.translate(tx,ty);ctx.scale(scale,scale);
  const nbr=new Set();
  if(sel){nbr.add(sel.id);links.forEach(l=>{if(l.s===sel)nbr.add(l.t.id);if(l.t===sel)nbr.add(l.s.id);});}
  for(const l of links){const on=!sel||(nbr.has(l.s.id)&&nbr.has(l.t.id)&&(l.s===sel||l.t===sel));
   ctx.strokeStyle=on?'#5a6b7d':'#2d3a4a55';ctx.lineWidth=(on?1.2:.6)/scale;
   ctx.beginPath();ctx.moveTo(l.s.x,l.s.y);ctx.lineTo(l.t.x,l.t.y);ctx.stroke();}
  for(const n of nodes){const dim=sel&&!nbr.has(n.id);const r=Math.min(11,4+Math.sqrt(n.deg));
   ctx.globalAlpha=dim?.18:1;ctx.fillStyle=COLOR[n.kind]||'#888';
   ctx.beginPath();ctx.arc(n.x,n.y,r,0,6.283);ctx.fill();
   if(n===hover||n===sel){ctx.strokeStyle='#fff';ctx.lineWidth=1.5/scale;ctx.stroke();}
   if(scale>1.2||n===hover||n===sel||n.kind==='syrs'){ctx.globalAlpha=dim?.25:.95;ctx.fillStyle='#cdd9e5';
    ctx.font=`${10/scale}px sans-serif`;ctx.fillText(n.label,n.x+r+2/scale,n.y+3/scale);}}
  ctx.restore();ctx.globalAlpha=1;}
 function tick(){draw();if(alpha>.02){step();raf=requestAnimationFrame(tick);}else draw();}
 function toWorld(e){const r=cv.getBoundingClientRect();return{x:(e.clientX-r.left-tx)/scale,y:(e.clientY-r.top-ty)/scale};}
 function pick(p){let best=null,bd=1e9;for(const n of nodes){const dx=n.x-p.x,dy=n.y-p.y,d=dx*dx+dy*dy;
  if(d<bd&&d<400/scale){bd=d;best=n;}}return best;}
 function bind(){const tip=document.getElementById('gtip');
  cv.onmousedown=e=>{const n=pick(toWorld(e));if(n)drag=n;else{pan={x:e.clientX,y:e.clientY,tx,ty};cv.style.cursor='grabbing';}};
  window.addEventListener('mousemove',e=>{
   if(drag){const p=toWorld(e);drag.x=p.x;drag.y=p.y;drag.vx=drag.vy=0;alpha=Math.max(alpha,.3);if(!raf)tick();}
   else if(pan){tx=pan.tx+(e.clientX-pan.x);ty=pan.ty+(e.clientY-pan.y);draw();}
   else{const r=cv.getBoundingClientRect();
    if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom){hover=null;tip.style.display='none';return;}
    const n=pick(toWorld(e));hover=n;draw();
    if(n){tip.style.display='block';tip.style.left=(e.clientX+12)+'px';tip.style.top=(e.clientY+12)+'px';tip.innerHTML=label(n);}
    else tip.style.display='none';}});
  window.addEventListener('mouseup',()=>{drag=null;if(pan){pan=null;cv.style.cursor='grab';}});
  cv.onclick=e=>{const n=pick(toWorld(e));sel=(n===sel)?null:n;draw();};
  cv.onwheel=e=>{e.preventDefault();const r=cv.getBoundingClientRect();
   const mx=e.clientX-r.left,my=e.clientY-r.top,f=e.deltaY<0?1.12:.89;
   tx=mx-(mx-tx)*f;ty=my-(my-ty)*f;scale*=f;draw();};}
 function label(n){
  if(n.kind==='srs'){const r=M.reqs[n.id];return `<b>${n.id}</b><br>${esc(r.title)}<br><span class="muted">evidence: ${r.evidence} · traceability: ${r.adequacy}</span>`;}
  if(n.kind==='test'){const t=M.testinfo[n.id]||{};return `<b>${n.id}</b><br><span class="muted">result: ${t.rollup||'?'} · ${t.klines||0} kernel lines</span>`;}
  if(n.kind==='impl'){const i=M.implinfo[n.id]||{};return `<b>${n.id}</b><br><span class="muted">${i.loc||'macro/inline'}</span>`;}
  return `<b>${n.id}</b>`;}
 function ensure(){if(!built){built=true;ui();
  build(document.getElementById('gscope').value);
  window.addEventListener('resize',()=>{if(host().classList.contains('active')){fit();draw();}});}}
 return {ensure};
})();
</script>
</body>
</html>"""

if __name__ == "__main__":
    sys.exit(main())
