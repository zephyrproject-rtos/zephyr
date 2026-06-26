# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Turn an ``mlx.traceability`` JSON export into a self-contained, interactive HTML
traceability report.

The input is the ``traceability_json_export_path`` file produced by the docs
build (by default ``doc/_build/html/traceability.json``). The output is a single
HTML file (no external assets, works offline) with:

* a dashboard of coverage metrics,
* per-component verification/design coverage,
* a filterable, sortable, drill-down requirement explorer,
* gap and issue finders (untested / undesigned requirements, dangling links,
  non-test verifiers, childless system requirements),
* an interactive relationship graph (requirements <-> tests / design / system
  requirements).

It is completely independent of the docs build: it only reads the JSON.

Usage::

    python doc/_scripts/gen_traceability_report.py \\
        [--json doc/_build/html/traceability.json] \\
        [--out doc/_build/traceability_report.html]
"""

import argparse
import json
import re
import sys
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

SRS_RE = re.compile(r"^ZEP-SRS-\d+-\d+$")
SYRS_RE = re.compile(r"^ZEP-SYRS-\d+$")


def classify(item_id):
    if SRS_RE.match(item_id):
        return "srs"
    if SYRS_RE.match(item_id):
        return "syrs"
    if item_id.startswith("DESIGN-"):
        return "design"
    if item_id.startswith("test_"):
        return "test"
    return "other"


def srs_sortkey(uid):
    return tuple(int(x) for x in re.findall(r"\d+", uid))


def build_model(items):
    """Reduce the raw mlx export into a compact model the report consumes."""
    by_id = {it["id"]: it for it in items}

    def targets(it, key):
        return list((it.get("targets") or {}).get(key) or [])

    def attr(it, key):
        return (it.get("attributes") or {}).get(key) or ""

    # All ids referenced by any relationship, to detect dangling links.
    referenced = set()
    for it in items:
        for lst in (it.get("targets") or {}).values():
            referenced.update(lst or [])
    dangling = sorted(referenced - set(by_id))

    reqs, syrs, design, tests = {}, {}, {}, {}
    for it in items:
        kind = classify(it["id"])
        if kind == "srs":
            verifiers = targets(it, "validated_by")
            reqs[it["id"]] = {
                "id": it["id"],
                "title": it.get("caption") or "",
                "component": attr(it, "component") or "(none)",
                "status": attr(it, "status") or "(none)",
                "rtype": attr(it, "rtype") or "",
                "tests": sorted(verifiers),
                "design": sorted(targets(it, "fulfilled_by")),
                "parents": sorted(targets(it, "trace")),
                "document": it.get("document") or "",
            }
        elif kind == "syrs":
            syrs[it["id"]] = {
                "id": it["id"],
                "title": it.get("caption") or "",
                "component": attr(it, "component") or "(none)",
                "status": attr(it, "status") or "(none)",
                "children": sorted(targets(it, "backtrace"), key=lambda u: srs_sortkey(u)),
            }
        elif kind == "design":
            design[it["id"]] = {
                "id": it["id"],
                "title": it.get("caption") or "",
                "fulfills": sorted(targets(it, "fulfills"), key=srs_sortkey),
                "document": it.get("document") or "",
            }
        else:  # test or other
            tests[it["id"]] = {
                "id": it["id"],
                "caption": it.get("caption") or "",
                "validates": sorted(targets(it, "validates"), key=srs_sortkey),
                "implements": sorted(targets(it, "implements"), key=srs_sortkey),
                "is_test": kind == "test",
                "document": it.get("document") or "",
            }

    # Per-component aggregates.
    comp = defaultdict(lambda: {"total": 0, "tested": 0, "designed": 0, "both": 0, "neither": 0})
    for r in reqs.values():
        c = comp[r["component"]]
        c["total"] += 1
        t = bool(r["tests"])
        d = bool(r["design"])
        c["tested"] += t
        c["designed"] += d
        c["both"] += t and d
        c["neither"] += not t and not d

    status_counts = defaultdict(int)
    for r in reqs.values():
        status_counts[r["status"]] += 1

    # Issues / gaps.
    untested = sorted((r["id"] for r in reqs.values() if not r["tests"]), key=srs_sortkey)
    undesigned = sorted((r["id"] for r in reqs.values() if not r["design"]), key=srs_sortkey)
    neither = sorted(
        (r["id"] for r in reqs.values() if not r["tests"] and not r["design"]),
        key=srs_sortkey,
    )
    no_parent = sorted((r["id"] for r in reqs.values() if not r["parents"]), key=srs_sortkey)
    non_test_verifiers = sorted(
        t["id"] for t in tests.values() if not t["is_test"] and (t["validates"] or t["implements"])
    )
    childless_syrs = sorted(s["id"] for s in syrs.values() if not s["children"])

    total = len(reqs)
    tested = sum(1 for r in reqs.values() if r["tests"])
    designed = sum(1 for r in reqs.values() if r["design"])

    summary = {
        "requirements": total,
        "system_requirements": len(syrs),
        "tests": sum(1 for t in tests.values() if t["is_test"]),
        "design": len(design),
        "tested": tested,
        "designed": designed,
        "tested_pct": round(100 * tested / total, 1) if total else 0,
        "designed_pct": round(100 * designed / total, 1) if total else 0,
        "untested": len(untested),
        "undesigned": len(undesigned),
        "neither": len(neither),
        "dangling": len(dangling),
        "non_test_verifiers": len(non_test_verifiers),
        "generated": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
    }

    components = []
    for name, c in sorted(comp.items(), key=lambda kv: (kv[1]["tested"] / kv[1]["total"], -kv[1]["total"]) if kv[1]["total"] else (0, 0)):
        components.append({"name": name, **c,
                           "tested_pct": round(100 * c["tested"] / c["total"]) if c["total"] else 0,
                           "designed_pct": round(100 * c["designed"] / c["total"]) if c["total"] else 0})

    return {
        "summary": summary,
        "components": components,
        "status_counts": dict(sorted(status_counts.items())),
        "reqs": reqs,
        "syrs": syrs,
        "design": design,
        "tests": tests,
        "issues": {
            "untested": untested,
            "undesigned": undesigned,
            "neither": neither,
            "no_parent": no_parent,
            "non_test_verifiers": non_test_verifiers,
            "childless_syrs": childless_syrs,
            "dangling": dangling,
        },
    }


def print_summary(model):
    s = model["summary"]
    print(f"Requirements:        {s['requirements']}")
    print(f"  verified by tests: {s['tested']} ({s['tested_pct']}%)")
    print(f"  linked to design:  {s['designed']} ({s['designed_pct']}%)")
    print(f"  untested:          {s['untested']}")
    print(f"  no design link:    {s['undesigned']}")
    print(f"  neither:           {s['neither']}")
    print(f"System requirements: {s['system_requirements']}")
    print(f"Tests:               {s['tests']}    Design items: {s['design']}")
    if s["dangling"]:
        print(f"!! dangling links:   {s['dangling']}  -> {', '.join(model['issues']['dangling'][:8])}...")
    if s["non_test_verifiers"]:
        print(f"!! non-test verifiers: {s['non_test_verifiers']}  -> {', '.join(model['issues']['non_test_verifiers'])}")
    print("\nWorst-covered components:")
    for c in model["components"][:8]:
        print(f"  {c['name']:32} {c['tested']:3}/{c['total']:<3} tested ({c['tested_pct']:3}%)  "
              f"{c['designed']:3}/{c['total']:<3} designed")


def render_html(model):
    data_json = json.dumps(model, separators=(",", ":")).replace("</", "<\\/")
    return _HTML_TEMPLATE.replace("/*__DATA__*/", data_json)


# --- The HTML/CSS/JS report (self-contained, no external assets) -------------
_HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Zephyr Requirements Traceability Report</title>
<style>
:root{
  --bg:#0f1419; --panel:#1a212b; --panel2:#222c39; --line:#2d3a4a; --fg:#e6edf3;
  --muted:#8b98a5; --accent:#4493f8; --ok:#3fb950; --warn:#d29922; --bad:#f85149;
  --srs:#4493f8; --syrs:#a371f7; --test:#3fb950; --design:#d29922; --other:#f85149;
}
*{box-sizing:border-box}
body{margin:0;font:14px/1.5 -apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;
  background:var(--bg);color:var(--fg)}
header{padding:14px 20px;border-bottom:1px solid var(--line);display:flex;
  align-items:baseline;gap:16px;flex-wrap:wrap;background:var(--panel)}
header h1{font-size:18px;margin:0}
header .meta{color:var(--muted);font-size:12px}
nav{display:flex;gap:4px;padding:0 12px;background:var(--panel);border-bottom:1px solid var(--line);
  position:sticky;top:0;z-index:10;flex-wrap:wrap}
nav button{background:none;border:none;color:var(--muted);padding:10px 14px;cursor:pointer;
  font-size:13px;border-bottom:2px solid transparent}
nav button:hover{color:var(--fg)}
nav button.active{color:var(--fg);border-bottom-color:var(--accent)}
main{padding:18px 20px;max-width:1500px;margin:0 auto}
.tab{display:none}.tab.active{display:block}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin-bottom:18px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:14px}
.card .n{font-size:26px;font-weight:600}
.card .l{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.04em}
.card.ok .n{color:var(--ok)} .card.warn .n{color:var(--warn)} .card.bad .n{color:var(--bad)}
.card.click{cursor:pointer} .card.click:hover{border-color:var(--accent)}
h2{font-size:15px;border-bottom:1px solid var(--line);padding-bottom:6px;margin:22px 0 12px}
table{width:100%;border-collapse:collapse;font-size:13px}
th,td{text-align:left;padding:6px 8px;border-bottom:1px solid var(--line);vertical-align:top}
th{color:var(--muted);font-weight:600;cursor:pointer;user-select:none;position:sticky;top:43px;background:var(--bg)}
th:hover{color:var(--fg)}
tbody tr:hover{background:var(--panel)}
.bar{height:9px;border-radius:5px;background:var(--panel2);overflow:hidden;min-width:90px;position:relative}
.bar>span{display:block;height:100%;background:var(--ok)}
.bar.low>span{background:var(--bad)} .bar.mid>span{background:var(--warn)}
.pill{display:inline-block;padding:1px 7px;border-radius:10px;font-size:11px;border:1px solid var(--line)}
.pill.yes{color:var(--ok);border-color:var(--ok)} .pill.no{color:var(--bad);border-color:var(--bad)}
.tag{display:inline-block;font-size:11px;padding:0 6px;border-radius:4px;background:var(--panel2);
  margin:1px 2px;color:var(--muted)}
.tag.t{color:var(--test)} .tag.d{color:var(--design)} .tag.s{color:var(--syrs)}
.controls{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-bottom:12px}
input[type=search],select{background:var(--panel);border:1px solid var(--line);color:var(--fg);
  padding:7px 10px;border-radius:6px;font-size:13px}
input[type=search]{min-width:240px}
.muted{color:var(--muted)}
.expand{cursor:pointer;color:var(--accent)}
.detail{background:var(--panel);font-size:12px}
.detail td{padding:10px 14px}
.detail h4{margin:8px 0 4px;font-size:12px;color:var(--muted);text-transform:uppercase}
code{background:var(--panel2);padding:1px 5px;border-radius:4px;font-size:12px}
.legend{display:flex;gap:14px;flex-wrap:wrap;margin:8px 0;font-size:12px;color:var(--muted)}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:5px;vertical-align:middle}
#gcanvas{width:100%;height:640px;background:var(--panel);border:1px solid var(--line);border-radius:8px;
  display:block;cursor:grab}
.gtip{position:fixed;pointer-events:none;background:#000d;border:1px solid var(--line);padding:6px 9px;
  border-radius:6px;font-size:12px;max-width:320px;display:none;z-index:50}
.copybtn{background:var(--panel2);border:1px solid var(--line);color:var(--fg);border-radius:6px;
  padding:4px 9px;font-size:12px;cursor:pointer}
.copybtn:hover{border-color:var(--accent)}
details{margin:6px 0}summary{cursor:pointer;color:var(--accent)}
a{color:var(--accent);text-decoration:none}a:hover{text-decoration:underline}
</style>
</head>
<body>
<header>
  <h1>Zephyr Requirements Traceability</h1>
  <span class="meta" id="meta"></span>
</header>
<nav id="nav"></nav>
<main>
  <div class="tab active" id="tab-overview"></div>
  <div class="tab" id="tab-components"></div>
  <div class="tab" id="tab-requirements"></div>
  <div class="tab" id="tab-gaps"></div>
  <div class="tab" id="tab-graph"></div>
</main>
<div class="gtip" id="gtip"></div>
<script id="data" type="application/json">/*__DATA__*/</script>
<script>
const M = JSON.parse(document.getElementById('data').textContent);
const REQ = M.reqs, SYRS = M.syrs, DESIGN = M.design, TESTS = M.tests;
const el = (h)=>{const t=document.createElement('template');t.innerHTML=h.trim();return t.content.firstChild;};
const esc = (s)=>(s||'').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const pct = (n,d)=>d?Math.round(100*n/d):0;
const barClass = (p)=>p>=80?'':p>=40?'mid':'low';

document.getElementById('meta').textContent =
  `${M.summary.requirements} requirements · ${M.summary.tests} tests · ${M.summary.design} design items · generated ${M.summary.generated}`;

// ---- tabs ----
const TABS=[['overview','Overview'],['components','Coverage by component'],
  ['requirements','Requirement explorer'],['gaps','Gaps & issues'],['graph','Relationship graph']];
const nav=document.getElementById('nav');
TABS.forEach(([id,label],i)=>{
  const b=el(`<button data-t="${id}">${label}</button>`);
  if(i===0)b.classList.add('active');
  b.onclick=()=>{document.querySelectorAll('nav button').forEach(x=>x.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
    b.classList.add('active');document.getElementById('tab-'+id).classList.add('active');
    if(id==='graph')Graph.ensure();};
  nav.appendChild(b);
});
function goReq(uid){ // jump to requirement explorer filtered to one uid
  document.querySelector('nav button[data-t=requirements]').click();
  const s=document.getElementById('reqsearch'); s.value=uid; s.dispatchEvent(new Event('input'));
}

// ---- overview ----
(function(){
  const s=M.summary;
  const card=(n,l,cls,onclick)=>`<div class="card ${cls||''} ${onclick?'click':''}" ${onclick?`onclick="${onclick}"`:''}><div class="n">${n}</div><div class="l">${l}</div></div>`;
  let h=`<div class="cards">
    ${card(s.requirements,'Requirements')}
    ${card(s.tested_pct+'%','Verified by tests',s.tested_pct>=80?'ok':s.tested_pct>=40?'warn':'bad')}
    ${card(s.designed_pct+'%','Linked to design',s.designed_pct>=80?'ok':s.designed_pct>=40?'warn':'bad')}
    ${card(s.untested,'Untested',s.untested?'bad':'ok',"document.querySelector('nav button[data-t=gaps]').click()")}
    ${card(s.neither,'No test & no design',s.neither?'bad':'ok',"document.querySelector('nav button[data-t=gaps]').click()")}
    ${card(s.dangling,'Dangling links',s.dangling?'bad':'ok',"document.querySelector('nav button[data-t=gaps]').click()")}
  </div>`;
  // status breakdown
  h+=`<h2>Requirement status</h2><div class="cards">`;
  for(const [k,v] of Object.entries(M.status_counts)) h+=card(v,esc(k));
  h+=`</div>`;
  // verification progress bar
  h+=`<h2>Verification & design coverage</h2>`;
  const pbar=(label,done,total)=>{const p=pct(done,total);return `<div style="margin:8px 0">
    <div style="display:flex;justify-content:space-between"><span>${label}</span><span class="muted">${done}/${total} (${p}%)</span></div>
    <div class="bar ${barClass(p)}" style="height:14px"><span style="width:${p}%"></span></div></div>`;};
  h+=pbar('Requirements verified by at least one test',s.tested,s.requirements);
  h+=pbar('Requirements linked to a design element',s.designed,s.requirements);
  document.getElementById('tab-overview').innerHTML=h;
})();

// ---- components ----
(function(){
  let h=`<p class="muted">Sorted by verification coverage (weakest first). Click a row to explore that component.</p>
  <table><thead><tr><th>Component</th><th>Reqs</th><th>Verified</th><th></th><th>Designed</th><th></th><th>Neither</th></tr></thead><tbody>`;
  for(const c of M.components){
    h+=`<tr style="cursor:pointer" onclick="goComp('${esc(c.name)}')">
      <td>${esc(c.name)}</td><td>${c.total}</td>
      <td>${c.tested} <span class="muted">(${c.tested_pct}%)</span></td>
      <td><div class="bar ${barClass(c.tested_pct)}"><span style="width:${c.tested_pct}%"></span></div></td>
      <td>${c.designed} <span class="muted">(${c.designed_pct}%)</span></td>
      <td><div class="bar ${barClass(c.designed_pct)}"><span style="width:${c.designed_pct}%"></span></div></td>
      <td>${c.neither?`<span style="color:var(--bad)">${c.neither}</span>`:'0'}</td></tr>`;
  }
  h+=`</tbody></table>`;
  document.getElementById('tab-components').innerHTML=h;
})();
function goComp(name){
  document.querySelector('nav button[data-t=requirements]').click();
  document.getElementById('reqcomp').value=name;
  document.getElementById('reqsearch').value='';
  renderReqs();
}

// ---- requirement explorer ----
const reqList=Object.values(REQ).sort((a,b)=>a.id.localeCompare(b.id,undefined,{numeric:true}));
let sortKey='id',sortDir=1;
(function(){
  const comps=[...new Set(reqList.map(r=>r.component))].sort();
  const stats=[...new Set(reqList.map(r=>r.status))].sort();
  document.getElementById('tab-requirements').innerHTML=`
  <div class="controls">
    <input type="search" id="reqsearch" placeholder="Search UID, title, test, design…">
    <select id="reqcomp"><option value="">All components</option>${comps.map(c=>`<option>${esc(c)}</option>`).join('')}</select>
    <select id="reqstatus"><option value="">All statuses</option>${stats.map(c=>`<option>${esc(c)}</option>`).join('')}</select>
    <select id="reqcov">
      <option value="">Any coverage</option>
      <option value="untested">Untested</option>
      <option value="undesigned">No design</option>
      <option value="neither">No test &amp; no design</option>
      <option value="full">Tested &amp; designed</option>
    </select>
    <span class="muted" id="reqcount"></span>
  </div>
  <table id="reqtable"><thead><tr>
    <th data-k="id">UID</th><th data-k="title">Title</th><th data-k="component">Component</th>
    <th data-k="status">Status</th><th data-k="ntests"># tests</th><th data-k="ndesign"># design</th>
    <th data-k="verified">Verified</th></tr></thead><tbody id="reqbody"></tbody></table>`;
  document.getElementById('reqsearch').addEventListener('input',renderReqs);
  document.getElementById('reqcomp').addEventListener('change',renderReqs);
  document.getElementById('reqstatus').addEventListener('change',renderReqs);
  document.getElementById('reqcov').addEventListener('change',renderReqs);
  document.querySelectorAll('#reqtable th').forEach(th=>th.onclick=()=>{
    const k=th.dataset.k; if(sortKey===k)sortDir*=-1;else{sortKey=k;sortDir=1;} renderReqs();});
  renderReqs();
})();
function reqMatches(r,q){
  if(!q)return true; q=q.toLowerCase();
  return r.id.toLowerCase().includes(q)||r.title.toLowerCase().includes(q)
    ||r.tests.some(t=>t.toLowerCase().includes(q))||r.design.some(d=>d.toLowerCase().includes(q));
}
function renderReqs(){
  const q=document.getElementById('reqsearch').value;
  const c=document.getElementById('reqcomp').value;
  const st=document.getElementById('reqstatus').value;
  const cov=document.getElementById('reqcov').value;
  let rows=reqList.filter(r=>reqMatches(r,q)
    &&(!c||r.component===c)&&(!st||r.status===st)
    &&(cov!=='untested'||!r.tests.length)
    &&(cov!=='undesigned'||!r.design.length)
    &&(cov!=='neither'||(!r.tests.length&&!r.design.length))
    &&(cov!=='full'||(r.tests.length&&r.design.length)));
  const val=(r)=>({id:r.id,title:r.title,component:r.component,status:r.status,
    ntests:r.tests.length,ndesign:r.design.length,verified:r.tests.length?1:0}[sortKey]);
  rows.sort((a,b)=>{const x=val(a),y=val(b);return (x<y?-1:x>y?1:0)*sortDir||a.id.localeCompare(b.id,undefined,{numeric:true});});
  document.getElementById('reqcount').textContent=`${rows.length} of ${reqList.length}`;
  const body=document.getElementById('reqbody'); body.innerHTML='';
  for(const r of rows){
    const tr=el(`<tr>
      <td><span class="expand">${r.id}</span></td><td>${esc(r.title)}</td>
      <td>${esc(r.component)}</td><td>${esc(r.status)}</td>
      <td style="color:${r.tests.length?'var(--ok)':'var(--bad)'}">${r.tests.length}</td>
      <td style="color:${r.design.length?'var(--ok)':'var(--muted)'}">${r.design.length}</td>
      <td>${r.tests.length?'<span class="pill yes">yes</span>':'<span class="pill no">no</span>'}</td></tr>`);
    tr.querySelector('.expand').onclick=()=>toggleDetail(tr,r);
    body.appendChild(tr);
  }
}
function toggleDetail(tr,r){
  if(tr.nextSibling&&tr.nextSibling.classList&&tr.nextSibling.classList.contains('detail')){
    tr.nextSibling.remove();return;}
  const tests=r.tests.map(t=>`<span class="tag t" title="${esc((TESTS[t]||{}).caption||'')}">${t}${TESTS[t]&&!TESTS[t].is_test?' ⚠':''}</span>`).join('')||'<span class="muted">none — not verified</span>';
  const des=r.design.map(d=>`<span class="tag d" title="${esc((DESIGN[d]||{}).title||'')}">${d}</span>`).join('')||'<span class="muted">none</span>';
  const par=r.parents.map(p=>`<span class="tag s" title="${esc((SYRS[p]||{}).title||'')}">${p}</span>`).join('')||'<span class="muted">none</span>';
  const d=el(`<tr class="detail"><td colspan="7">
    <h4>Verifying tests (${r.tests.length})</h4>${tests}
    <h4>Design / architecture (${r.design.length})</h4>${des}
    <h4>Parent system requirements</h4>${par}
    <div class="muted" style="margin-top:8px">Component: ${esc(r.component)} · type: ${esc(r.rtype)} · status: ${esc(r.status)}
     · <a href="#" onclick="Graph.focus('${r.id}');return false">show in graph</a></div>
  </td></tr>`);
  tr.after(d);
}

// ---- gaps & issues ----
(function(){
  const I=M.issues;
  const section=(title,desc,ids,linkify)=>{
    if(!ids.length)return `<details><summary>${title} — <span style="color:var(--ok)">0</span> ✓</summary><p class="muted">${desc}</p></details>`;
    const list=ids.map(linkify).join(' ');
    return `<details ${ids.length?'open':''}><summary>${title} — <span style="color:var(--bad)">${ids.length}</span></summary>
      <p class="muted">${desc} <button class="copybtn" onclick='navigator.clipboard.writeText(${JSON.stringify(ids.join("\n"))})'>copy ${ids.length} ids</button></p>
      <div style="line-height:2.1">${list}</div></details>`;
  };
  const rlink=(u)=>`<span class="tag" style="cursor:pointer;color:var(--srs)" onclick="goReq('${u}')">${u}</span>`;
  const plain=(u)=>`<span class="tag">${esc(u)}</span>`;
  let h=`<p class="muted">Each section lists items that likely need attention. Click a requirement to open it; copy a list to triage.</p>`;
  h+=section('Requirements with no verifying test','These requirements have no <code>@verifies</code> link from any test.',I.untested,rlink);
  h+=section('Requirements with no test AND no design','Highest risk: neither verified nor linked to a design element.',I.neither,rlink);
  h+=section('Requirements with no design link','No design/architecture element fulfils these (add a <code>design</code> directive).',I.undesigned,rlink);
  h+=section('Requirements with no parent system requirement','Not traced up to a ZEP-SYRS system requirement.',I.no_parent,rlink);
  h+=section('Non-test verifiers','Symbols that carry <code>@verifies</code> but are not <code>test_*</code> functions (often macros picked up by accident).',I.non_test_verifiers,plain);
  h+=section('System requirements with no children','ZEP-SYRS items that no software requirement traces to.',I.childless_syrs,plain);
  h+=section('Dangling links','Relationship targets that do not resolve to any defined item (broken references).',I.dangling,plain);
  document.getElementById('tab-gaps').innerHTML=h;
})();

// ---- relationship graph (self-contained canvas force layout) ----
const Graph=(function(){
  const cv=el('<canvas id="gcanvas"></canvas>');
  let built=false,nodes=[],links=[],byId={},raf=0,alpha=0,
      tx=0,ty=0,scale=1,drag=null,pan=null,hover=null,sel=null,dpr=1;
  const COLOR={srs:'#4493f8',syrs:'#a371f7',test:'#3fb950',design:'#d29922',other:'#f85149'};
  function host(){return document.getElementById('tab-graph');}
  function ui(){
    const comps=[...new Set(Object.values(REQ).map(r=>r.component))].sort();
    host().innerHTML=`<div class="controls">
      <select id="gscope"><option value="__hier">System → software requirement hierarchy</option>
        ${comps.map(c=>`<option value="c:${esc(c)}">Component: ${esc(c)}</option>`).join('')}</select>
      <input type="search" id="gfind" placeholder="Focus a requirement (e.g. ZEP-SRS-1-3)…">
      <span class="muted">drag nodes · scroll to zoom · click to highlight neighbours</span>
    </div>
    <div class="legend">
      <span><span class="dot" style="background:${COLOR.srs}"></span>requirement</span>
      <span><span class="dot" style="background:${COLOR.syrs}"></span>system req</span>
      <span><span class="dot" style="background:${COLOR.test}"></span>test</span>
      <span><span class="dot" style="background:${COLOR.design}"></span>design</span>
      <span><span class="dot" style="background:${COLOR.other}"></span>other verifier</span>
    </div>`;
    host().appendChild(cv);
    document.getElementById('gscope').onchange=e=>build(e.target.value);
    document.getElementById('gfind').addEventListener('change',e=>{const v=e.target.value.trim();if(v)focus(v);});
    bindCanvas();
  }
  function addNode(id,kind,label){if(!byId[id]){const n={id,kind,label,
      x:(Math.random()-.5)*600,y:(Math.random()-.5)*400,vx:0,vy:0,deg:0};byId[id]=n;nodes.push(n);}return byId[id];}
  function addLink(a,b){if(byId[a]&&byId[b]){links.push({s:byId[a],t:byId[b]});byId[a].deg++;byId[b].deg++;}}
  function build(scope){
    nodes=[];links=[];byId={};sel=null;
    if(scope==='__hier'){
      for(const s of Object.values(SYRS)){addNode(s.id,'syrs',s.id);}
      for(const r of Object.values(REQ)){
        if(!r.parents.length)continue;
        addNode(r.id,'srs',r.id);
        r.parents.forEach(p=>{addNode(p, p.startsWith('ZEP-SYRS')?'syrs':'srs', p);addLink(r.id,p);});
      }
    } else { // component
      const comp=scope.slice(2);
      for(const r of Object.values(REQ)){
        if(r.component!==comp)continue;
        addNode(r.id,'srs',r.id);
        r.tests.forEach(t=>{addNode(t,(TESTS[t]&&!TESTS[t].is_test)?'other':'test',t);addLink(r.id,t);});
        r.design.forEach(dd=>{addNode(dd,'design',dd);addLink(r.id,dd);});
        r.parents.forEach(p=>{addNode(p,'syrs',p);addLink(r.id,p);});
      }
    }
    // initial radial-ish seed
    nodes.forEach((n,i)=>{const a=i/nodes.length*6.283;n.x=Math.cos(a)*250;n.y=Math.sin(a)*250;});
    fit();alpha=1;tick();
  }
  function fit(){
    const r=cv.getBoundingClientRect();dpr=window.devicePixelRatio||1;
    cv.width=r.width*dpr;cv.height=r.height*dpr;
    scale=1;tx=cv.width/(2*dpr);ty=cv.height/(2*dpr);
  }
  function step(){
    const k=120, rep=2200;
    for(let i=0;i<nodes.length;i++){const a=nodes[i];
      for(let j=i+1;j<nodes.length;j++){const b=nodes[j];
        let dx=a.x-b.x,dy=a.y-b.y,d2=dx*dx+dy*dy+0.01,d=Math.sqrt(d2);
        const f=rep/d2; const fx=dx/d*f,fy=dy/d*f;
        a.vx+=fx;a.vy+=fy;b.vx-=fx;b.vy-=fy;}}
    for(const l of links){let dx=l.t.x-l.s.x,dy=l.t.y-l.s.y,d=Math.sqrt(dx*dx+dy*dy)+.01;
      const f=(d-k)*0.02;const fx=dx/d*f,fy=dy/d*f;
      l.s.vx+=fx;l.s.vy+=fy;l.t.vx-=fx;l.t.vy-=fy;}
    for(const n of nodes){if(n===drag)continue;
      n.vx-=n.x*0.0012;n.vy-=n.y*0.0012; // gentle centering
      n.x+=n.vx*alpha;n.y+=n.vy*alpha;n.vx*=0.86;n.vy*=0.86;}
    alpha*=0.985;
  }
  function draw(){
    const ctx=cv.getContext('2d');ctx.setTransform(dpr,0,0,dpr,0,0);
    ctx.clearRect(0,0,cv.width,cv.height);
    ctx.save();ctx.translate(tx,ty);ctx.scale(scale,scale);
    const nbr=new Set();
    if(sel){nbr.add(sel.id);links.forEach(l=>{if(l.s===sel)nbr.add(l.t.id);if(l.t===sel)nbr.add(l.s.id);});}
    for(const l of links){
      const on=!sel||(nbr.has(l.s.id)&&nbr.has(l.t.id)&&(l.s===sel||l.t===sel));
      ctx.strokeStyle=on?'#5a6b7d':'#2d3a4a55';ctx.lineWidth=on?1.2/scale:0.6/scale;
      ctx.beginPath();ctx.moveTo(l.s.x,l.s.y);ctx.lineTo(l.t.x,l.t.y);ctx.stroke();}
    for(const n of nodes){
      const dim=sel&&!nbr.has(n.id);
      const r=Math.min(11,4+Math.sqrt(n.deg))/1;
      ctx.globalAlpha=dim?0.18:1;
      ctx.fillStyle=COLOR[n.kind]||'#888';
      ctx.beginPath();ctx.arc(n.x,n.y,r,0,6.283);ctx.fill();
      if(n===hover||n===sel){ctx.strokeStyle='#fff';ctx.lineWidth=1.5/scale;ctx.stroke();}
      if(scale>1.25||n===hover||n===sel||n.kind==='syrs'){
        ctx.globalAlpha=dim?0.25:0.95;ctx.fillStyle='#cdd9e5';
        ctx.font=`${10/scale}px sans-serif`;ctx.fillText(n.label,n.x+r+2/scale,n.y+3/scale);}
    }
    ctx.restore();ctx.globalAlpha=1;
  }
  function tick(){draw();if(alpha>0.02){step();raf=requestAnimationFrame(tick);}else{draw();}}
  function toWorld(ev){const r=cv.getBoundingClientRect();
    return {x:(ev.clientX-r.left-tx)/scale,y:(ev.clientY-r.top-ty)/scale};}
  function pick(p){let best=null,bd=1e9;for(const n of nodes){const dx=n.x-p.x,dy=n.y-p.y,d=dx*dx+dy*dy;
    if(d<bd&&d<400/scale){bd=d;best=n;}}return best;}
  function bindCanvas(){
    const tip=document.getElementById('gtip');
    cv.onmousedown=e=>{const p=toWorld(e);const n=pick(p);
      if(n){drag=n;}else{pan={x:e.clientX,y:e.clientY,tx,ty};cv.style.cursor='grabbing';}};
    window.addEventListener('mousemove',e=>{
      if(drag){const p=toWorld(e);drag.x=p.x;drag.y=p.y;drag.vx=drag.vy=0;alpha=Math.max(alpha,.3);if(alpha&&!raf)tick();}
      else if(pan){tx=pan.tx+(e.clientX-pan.x);ty=pan.ty+(e.clientY-pan.y);draw();}
      else{const r=cv.getBoundingClientRect();
        if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom){hover=null;tip.style.display='none';return;}
        const n=pick(toWorld(e));hover=n;draw();
        if(n){tip.style.display='block';tip.style.left=(e.clientX+12)+'px';tip.style.top=(e.clientY+12)+'px';
          tip.innerHTML=label(n);}else tip.style.display='none';}});
    window.addEventListener('mouseup',()=>{if(drag){drag=null;}if(pan){pan=null;cv.style.cursor='grab';}});
    cv.onclick=e=>{const n=pick(toWorld(e));sel=(n===sel)?null:n;draw();};
    cv.onwheel=e=>{e.preventDefault();const r=cv.getBoundingClientRect();
      const mx=e.clientX-r.left,my=e.clientY-r.top;const f=e.deltaY<0?1.12:0.89;
      tx=mx-(mx-tx)*f;ty=my-(my-ty)*f;scale*=f;draw();};
  }
  function label(n){
    if(n.kind==='srs'){const r=REQ[n.id];return `<b>${n.id}</b><br>${esc(r?r.title:'')}<br><span class="muted">${esc(r?r.component:'')} · ${r&&r.tests.length?r.tests.length+' tests':'untested'}</span>`;}
    if(n.kind==='syrs'){const s=SYRS[n.id];return `<b>${n.id}</b><br>${esc(s?s.title:'')}`;}
    if(n.kind==='design'){const d=DESIGN[n.id];return `<b>${n.id}</b><br>${esc(d?d.title:'')}`;}
    const t=TESTS[n.id];return `<b>${n.id}</b><br>${esc(t?t.caption:'')}${t&&!t.is_test?'<br>⚠ not a test_* function':''}`;}
  function focus(uid){
    document.querySelector('nav button[data-t=graph]').click();ensure();
    const r=REQ[uid]; if(r){document.getElementById('gscope').value='c:'+r.component;build('c:'+r.component);}
    else build(document.getElementById('gscope').value);
    sel=byId[uid]||null; if(sel){tx=cv.width/(2*dpr)-sel.x*scale;ty=cv.height/(2*dpr)-sel.y*scale;} draw();
  }
  function ensure(){if(!built){built=true;ui();build('__hier');window.addEventListener('resize',()=>{if(host().classList.contains('active')){fit();draw();}});}}
  return {ensure,focus};
})();
</script>
</body>
</html>"""


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--json", default="doc/_build/html/traceability.json",
                   help="Path to the mlx traceability JSON export.")
    p.add_argument("--out", default="doc/_build/traceability_report.html",
                   help="Path of the HTML report to write.")
    p.add_argument("--summary-only", action="store_true",
                   help="Only print the text summary; do not write HTML.")
    args = p.parse_args(argv)

    src = Path(args.json)
    if not src.is_file():
        p.error(f"traceability JSON not found: {src}\n"
                "Build the docs first (it is written to <build>/html/traceability.json).")
    items = json.loads(src.read_text())
    model = build_model(items)
    print_summary(model)

    if not args.summary_only:
        out = Path(args.out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(render_html(model))
        print(f"\nWrote {out}  ({out.stat().st_size // 1024} KiB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
