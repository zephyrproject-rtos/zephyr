#! /usr/bin/env python3

import json
import logging
import re
import subprocess
import sys

# TODO: impact and problemtype.
# TODO: setup publication date.

# Setup Instructions
# ==================
#
# You will need to install the [Atlassian Command Line
# Interface](https://bobswift.atlassian.net/wiki/spaces/ACLI/overview)
# Set this variable to where you have installed this tool.
acli = '/home/<user>/ACLI/acli'

# To use this script, you will need to fill in your username, and you
# will need to generate an API token.  These can be generated at:
# https://id.atlassian.com/manage-profile/security/api-tokens
user = '<email>'
token = '<token>'

# You can verify this works with the following command:
# ```
# ./acli jira -u <email> -t <token> --action getProjectList \
#       --server https://zephyrprojectsec.atlassian.net
# ```
#
# Note that only members of the triage team will have access to issues
# that are under embargo.
#
# Usage
# =====
#
# Run as either `./tocve.py ZEPSEC-nn cve` to generate a CVE or as
# `./tocve.py ZEPSEC-nn rnote` to generate release notes.  The CVE
# command expects a cve-template.json in the current directory, which
# can be updated with values for the fields this script does not set.

# Set this to where you have installed ACLI.
acli_base = [acli, 'jira', '-u', user,
        '-t', token,
        '--quiet',
        '--outputType', 'json',
        '--server', 'https://zephyrprojectsec.atlassian.net' ]

url_base = 'https://zephyrprojectsec.atlassian.net/browse/'
vul_doc = "https://docs.zephyrproject.org/latest/security/vulnerabilities.html"
mitre_base = 'http://cve.mitre.org/cgi-bin/cvename.cgi?name='

def flatten_json(j):
    """Flatten a json input.  Given an array of single map entries,
    fold them all into a single object."""
    obj = {}
    for item in j:
        for k, v in item.items():
            if k in obj:
                raise Exception("Duplicate key: {}".format(k))
            obj[k] = v
    return obj

def query(issue, action, *args):
    cmd = acli_base.copy()
    cmd.extend(['--issue', issue, '--action', action])
    cmd.extend(args)
    lines = subprocess.check_output(cmd)
    # The API seems to return just a newline if there is nothing to
    # return, which is invalid json.  For these cases, we'll substitue
    # an empty array.
    if lines == b'\n':
        lines = b'[]\n'
    return json.loads(lines)

def query_ticket(issue):
    data = query(issue, "getIssue")
    return data

def query_weblinks(issue):
    data = query(issue, "getRemoteLinkList")
    data = [flatten_json(x) for x in data]
    return data

def query_links(issue):
    data = query(issue, "getLinkList")
    data = [flatten_json(x) for x in data]
    return data

def clean_version(verno):
    return re.sub(r' \(\d+\)', '', verno)

class Issue():
    def __init__(self, issue):
        self.issue = issue
        logging.info(f"Querying {issue}")
        self.ticket = query_ticket(issue)
        self.weblinks = query_weblinks(issue)
        self.links = query_links(issue)
        logging.debug(f"ticket: {self.ticket}")
        logging.debug(f"links: {self.links}")

    def cve(self):
        return self.ticket["CVE"]

    def description(self):
        return self.ticket["Description"]

    def summary(self):
        return self.ticket["Summary"]

    def get_links(self):
        links = [link["URL"] for link in self.weblinks]

        # Go through the subtasks, and get their links.
        for child in self.links:
            if child["Type Name"] != "Sub-Task":
                continue
            child_id = child["To Issue"]
            child_ticket = query_ticket(child_id)
            if child_ticket["Status"].startswith("Rejected"):
                continue
            child_links = query_weblinks(child_id)
            links.extend([link["URL"] for link in child_links])

        return links

    def get_affected_versions(self):
        vre = re.compile(r"(v\d+(\.\d+)+)")
        aff = [x[0] for x in vre.findall(self.ticket["Affects versions"])]

        # Textual sort isn't quite right, but probably ok for this.
        aff.sort()
        return aff


    def get_full_links(self):
        links = [{
            "ver": clean_version(self.ticket["Fix versions"]),
            "URL": link["URL"]
            } for link in self.weblinks]

        for child in self.links:
            if child["Type Name"] != "Sub-Task":
                continue
            child_id = child["To Issue"]
            child_ticket = query_ticket(child_id)
            if child_ticket["Status"].startswith("Rejected"):
                continue
            child_links = query_weblinks(child_id)
            ver = clean_version(child_ticket["Fix versions"])
            if len(ver) == 0:
                ver = "branch from " + clean_version(child_ticket["Affects versions"])
            links.extend([{
                "ver": ver,
                "URL": link["URL"],
                } for link in child_links])

        links.sort(key=lambda x: x["ver"])

        return links

    def get_fixed_versions(self):
        return [x["ver"] for x in self.get_full_links()]

def add_ref(refs, text):
    refs.append({
        "refsource": "CONFIRM",
        "url": text})

def get_pr(link):
    """Convert a link into a PR number"""
    return link.split('/')[-1]

if __name__ == '__main__':
    args = sys.argv[1:]
    if len(args) != 2:
        print("Usage: tocve.py ZEPSEC-nn {cve|rnote}")
        sys.exit(1)
    ticket = args[0]
    command = args[1]

    with open('scripts/security/cve-template.json') as fp:
        cve = json.load(fp)
    issue = Issue(ticket)
    cve_id = issue.cve()

    if command == 'cve':
        aff_ver = issue.get_affected_versions()
        valt = "This issue affects:\nzephyrproject-rtos zephyr"
        for ver in aff_ver:
            valt += "\nversion {} and later versions.".format(ver[1:])

        cve["CVE_data_meta"]["ID"] = cve_id
        cve["CVE_data_meta"]["TITLE"] = issue.summary()
        cve["description"]["description_data"] = [{
                "lang": "eng",
                "value": issue.description() },
                { "lang": "eng", "value": valt }
                ]
        cve["source"]["defect"] = [
                url_base + ticket ]

        refs = []
        add_ref(refs, url_base + ticket)
        add_ref(refs, vul_doc + "#" + cve_id.lower())
        for link in issue.get_links():
            add_ref(refs, link)

        cve["references"]["reference_data"] = refs

        versions = []
        for ver in aff_ver:
            versions.append({
                "version_affected": ">=",
                "version_value": ver[1:]})
        cve["affects"]["vendor"]["vendor_data"][0]["product"]["product_data"][0]["version"]["version_data"] = versions

        json.dump(cve, sys.stdout, indent=4)

    if command == 'rnote':
        print(cve_id)
        print('-' * len(cve_id))
        print()
        print(issue.summary())
        print()
        print(issue.description())
        print()
        aff = issue.get_fixed_versions()
        if len(aff) > 2:
            aff[-1] = "and " + aff[-1]
        print("This has been fixed in releases {}.".format(
            ", ".join(aff)))
        print()
        print("- `{} <{}{}>`_".format(cve_id, mitre_base, cve_id))
        print()
        print("- `Zephyr project bug tracker {}".format(ticket))
        print("  <{}{}>`_".format(url_base, ticket))
        print()
        for link in issue.get_full_links():
            print("- `PR{} fix for {}".format(get_pr(link["URL"]), link["ver"]))
            print("  <{}>`_".format(link["URL"]))
            print()
