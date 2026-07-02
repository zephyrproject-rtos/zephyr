#!/usr/bin/env python3
#
# Copyright (c) 2025 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Generate a self-contained HTML dashboard from a Twister per-test coverage
matrix.

Twister's ``--coverage-per-test`` mode writes ``test_matrix.json`` (with
``by_line`` and ``by_test`` views). This script turns that JSON into a single,
dependency-free HTML page that shows, per test, how many files and lines it
covers and how many lines it covers uniquely, and lets you drill into a test's
files or look up which tests cover a given file and line.

Example:

    scripts/gen_test_matrix_dashboard.py -i twister-out/coverage/test_matrix.json
"""

import argparse
import json
import os
import sys

# Self-contained dashboard. The matrix is inlined at __MATRIX_DATA__ and all
# derived metrics are computed in the browser.
_TEMPLATE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Per-test coverage matrix</title>
<style>
 body{font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;
   margin:0;color:#1a1a1a;background:#f6f7f9}
 header{background:#2c3e50;color:#fff;padding:14px 24px}
 header h1{margin:0;font-size:18px}
 .stats{display:flex;flex-wrap:wrap;gap:22px;margin-top:8px;font-size:13px;color:#cfd8e3}
 .stats b{color:#fff;font-size:15px}
 main{padding:18px 24px;display:grid;grid-template-columns:1fr 1fr;gap:18px}
 .card{background:#fff;border:1px solid #e2e6ea;border-radius:6px;padding:14px;min-width:0}
 .card h2{margin:0 0 10px;font-size:14px;color:#2c3e50}
 .full{grid-column:1/3}
 table{border-collapse:collapse;width:100%;font-size:13px}
 th,td{text-align:left;padding:5px 8px;border-bottom:1px solid #eef1f3;
   white-space:nowrap;overflow:hidden;text-overflow:ellipsis;max-width:52ch}
 th{cursor:pointer;user-select:none;background:#f0f2f5;position:sticky;top:0}
 th.num,td.num{text-align:right;font-variant-numeric:tabular-nums;white-space:nowrap}
 th.sorted::after{content:" \25be";color:#2c3e50}
 th.asc.sorted::after{content:" \25b4"}
 tbody tr{cursor:pointer}
 tbody tr:hover{background:#eef6ff}
 tr.sel{background:#dcecff}
 input{width:100%;padding:6px 8px;border:1px solid #cbd2d9;border-radius:4px;
   margin-bottom:10px;box-sizing:border-box;font-size:13px}
 .scroll{max-height:62vh;overflow:auto}
 .uniq{color:#c0392b;font-weight:600}
 code{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:12px}
 .muted{color:#7a8794;font-size:12px}
</style>
</head>
<body>
<header>
 <h1>Per-test coverage matrix</h1>
 <div class="stats" id="stats"></div>
</header>
<main>
 <section class="card full">
   <h2>Coverage per test</h2>
   <input type="search" id="testFilter" placeholder="Filter tests&#8230;">
   <div class="scroll"><table id="testTable">
     <thead><tr>
       <th data-k="test">Test</th>
       <th data-k="files" class="num">Files</th>
       <th data-k="lines" class="num">Lines</th>
       <th data-k="unique" class="num">Unique</th>
       <th data-k="uniquePct" class="num">Unique %</th>
     </tr></thead>
     <tbody></tbody>
   </table></div>
 </section>
 <section class="card">
   <h2 id="detailTitle">Test details</h2>
   <div id="detail" class="muted">Select a test to see the files it covers.</div>
 </section>
 <section class="card">
   <h2>File &#8594; line &#8594; tests</h2>
   <input type="text" id="fileInput" list="fileList" placeholder="Type a source file&#8230;">
   <datalist id="fileList"></datalist>
   <div id="fileView" class="muted">Choose a file to see which tests cover each line.</div>
 </section>
</main>
<script id="data" type="application/json">__MATRIX_DATA__</script>
<script>
const DATA = JSON.parse(document.getElementById('data').textContent);
const byLine = DATA.by_line || {}, byTest = DATA.by_test || {};

const uniqByTest = {};
let coveredLines = 0, uniqueLines = 0;
for (const f in byLine) {
  for (const ln in byLine[f]) {
    const ts = byLine[f][ln];
    coveredLines++;
    if (ts.length === 1) { uniqueLines++; uniqByTest[ts[0]] = (uniqByTest[ts[0]] || 0) + 1; }
  }
}
const rows = Object.keys(byTest).map(t => {
  const files = Object.keys(byTest[t]).length;
  const lines = Object.values(byTest[t]).reduce((a, l) => a + l.length, 0);
  const unique = uniqByTest[t] || 0;
  return { test: t, files, lines, unique, uniquePct: lines ? unique / lines * 100 : 0 };
});
document.getElementById('stats').innerHTML =
  `<div><b>${rows.length}</b> tests</div>` +
  `<div><b>${Object.keys(byLine).length}</b> files</div>` +
  `<div><b>${coveredLines}</b> covered lines</div>` +
  `<div><b>${uniqueLines}</b> covered by exactly one test</div>`;

const tbody = document.querySelector('#testTable tbody');
let sortKey = 'unique', sortDir = -1;
function renderTable() {
  const q = document.getElementById('testFilter').value.toLowerCase();
  const rs = rows.filter(r => r.test.toLowerCase().includes(q)).sort((a, b) => {
    const x = a[sortKey], y = b[sortKey];
    return (x < y ? -1 : x > y ? 1 : 0) * sortDir;
  });
  tbody.innerHTML = rs.map(r =>
    `<tr data-t="${enc(r.test)}"><td title="${enc(r.test)}"><code>${enc(r.test)}</code></td>` +
    `<td class="num">${r.files}</td><td class="num">${r.lines}</td>` +
    `<td class="num uniq">${r.unique}</td><td class="num">${r.uniquePct.toFixed(1)}</td></tr>`
  ).join('');
  document.querySelectorAll('#testTable th').forEach(th => {
    th.classList.toggle('sorted', th.dataset.k === sortKey);
    th.classList.toggle('asc', th.dataset.k === sortKey && sortDir === 1);
  });
}
function showTest(t) {
  const files = byTest[t] || {};
  const list = Object.keys(files).map(f => {
    let u = 0;
    for (const ln of files[f]) { if ((byLine[f] && byLine[f][ln] || []).length === 1) u++; }
    return { f, n: files[f].length, u };
  }).sort((a, b) => b.n - a.n);
  document.getElementById('detailTitle').textContent = 'Files covered by ' + t;
  const total = list.reduce((a, x) => a + x.n, 0);
  document.getElementById('detail').innerHTML =
    `<div class="muted">${list.length} files, ${total} lines</div>` +
    `<div class="scroll"><table><thead><tr><th>File</th><th class="num">Lines</th>` +
    `<th class="num">Unique</th></tr></thead><tbody>` +
    list.map(x => `<tr><td title="${enc(x.f)}"><code>${enc(x.f)}</code></td>` +
      `<td class="num">${x.n}</td><td class="num uniq">${x.u}</td></tr>`).join('') +
    `</tbody></table></div>`;
}
function showFile(f) {
  const el = document.getElementById('fileView');
  const lines = byLine[f];
  if (!lines) { el.className = 'muted'; el.textContent = 'No coverage recorded for that file.'; return; }
  el.className = 'scroll';
  const lns = Object.keys(lines).map(Number).sort((a, b) => a - b);
  el.innerHTML = `<table><thead><tr><th class="num">Line</th><th>Covered by</th></tr></thead><tbody>` +
    lns.map(ln => {
      const ts = lines[ln];
      const cell = ts.length === 1
        ? `<span class="uniq">${enc(ts[0])}</span>`
        : ts.map(enc).join(', ');
      return `<tr><td class="num">${ln}</td><td>${cell}</td></tr>`;
    }).join('') + `</tbody></table>`;
}
function enc(s) {
  return String(s).replace(/[&<>"]/g,
    c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
}

document.querySelectorAll('#testTable th').forEach(th => th.onclick = () => {
  const k = th.dataset.k;
  if (sortKey === k) { sortDir *= -1; } else { sortKey = k; sortDir = k === 'test' ? 1 : -1; }
  renderTable();
});
document.getElementById('testFilter').oninput = renderTable;
tbody.onclick = e => {
  const tr = e.target.closest('tr');
  if (!tr || !tr.dataset.t) return;
  document.querySelectorAll('#testTable tr.sel').forEach(x => x.classList.remove('sel'));
  tr.classList.add('sel');
  showTest(tr.dataset.t);
};
document.getElementById('fileList').innerHTML =
  Object.keys(byLine).sort().map(f => `<option value="${enc(f)}">`).join('');
document.getElementById('fileInput').oninput = e => showFile(e.target.value);
renderTable();
</script>
</body>
</html>
"""


def render_dashboard(matrix):
    """Return the dashboard HTML for a parsed coverage matrix."""
    payload = json.dumps(matrix, sort_keys=True).replace("</", "<\\/")
    return _TEMPLATE.replace("__MATRIX_DATA__", payload)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-i", "--input", default="test_matrix.json",
                        help="Path to the test_matrix.json produced by Twister "
                             "(default: %(default)s).")
    parser.add_argument("-o", "--output", default=None,
                        help="Output HTML file (default: input with .html suffix).")
    args = parser.parse_args()

    try:
        with open(args.input) as fp:
            matrix = json.load(fp)
    except OSError as e:
        sys.exit(f"Unable to read coverage matrix '{args.input}': {e}")
    except json.JSONDecodeError as e:
        sys.exit(f"'{args.input}' is not a valid coverage matrix: {e}")

    output = args.output or (os.path.splitext(args.input)[0] + ".html")
    with open(output, "w") as fp:
        fp.write(render_dashboard(matrix))
    print(f"Per-test coverage dashboard written to {output}")


if __name__ == "__main__":
    main()
