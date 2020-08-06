#! /usr/bin/env python3

from github import Github
from git import Git
import netrc
import pickle
import pprint
import requests
import re

jira_host = 'zephyrprojectsec.atlassian.net'
baseurl = f"https://{jira_host}/rest/api/2/"

# Custom field names.
cve_field = 'customfield_10035'
embargo_field = 'customfield_10051'

# Get authentication information.
def get_auth(host):
    auth = netrc.netrc().authenticators(host)
    if auth is None:
        raise Exception("Expecting a single authenticator for host")
    return (auth[0], auth[2])
auth = get_auth(jira_host)

pr_re = re.compile(r'^https://github.com/zephyrproject-rtos/zephyr/pull/(\d+)$')

gh_token = get_auth('github.com')[1]
gh = Github(gh_token)
repo = gh.get_repo("zephyrproject-rtos/zephyr")
gitwork = Git(".")

with open('token') as fd:
    token = fd.readline().rstrip()

def query(text, field, params={}):
    result = []
    start = 1

    while True:
        params["startAt"] = start
        r = requests.get(baseurl + text, auth=auth, params=params)
        if r.status_code != 200:
            print(r)
            raise Exception("Failure in query")
        j = r.json()
        # print(len(j[field]), j["maxResults"])

        # The Jira API is inconsistent.  If the results returned are
        # directly a list, just use that.
        if isinstance(j, list):
            return j
        # for k in j.keys():
        #     print(k)

        result.extend(j[field])

        if len(j[field]) < j["maxResults"]:
            break

        start += j["maxResults"]

    return result

def get_remote_links(key):
    return query("issue/" + key + "/remotelink", 'unknown')

class Issue(object):
    def __init__(self, js):
        self.key = js["key"]
        fields = js["fields"]
        self.fixversion = fields["fixVersions"]
        self._status = fields["status"]
        self._issuetype = fields["issuetype"]
        self.cve = fields[cve_field]
        self.embargo = fields[embargo_field]
        self.subtasks = fields["subtasks"]

        self.fields = fields

        self.remotes = None
        # Only query the remotes lazily, because it is slow.
        # self.remotes = get_remote_links(self.key)

    def status(self):
        return self._status["name"]

    def issuetype(self):
        return self._issuetype["name"]

    def getlinks(self):
        if self.remotes is None:
            self.remotes = get_remote_links(self.key)
        return [x["object"]["url"] for x in self.remotes]

def main():
    p = { 'jql': 'project="ZEPSEC"' }
    j = query("search", "issues", params=p)

    pp = pprint.PrettyPrinter()
    # j = r.json()

    # pp.pprint(j['issues'][1])
    issues = []
    for jissue in j:
        # print(jissue['key'])
        issue = Issue(jissue)
        issues.append(issue)

    # Filter out the issues that are "Public".
    issues = [x for x in issues if x.status() != "Public"]
    issues = [x for x in issues if x.status() != "Rejected"]

    # Select open primary issues (with CVE).
    if False:
        issues = [x for x in issues if x.issuetype() != "Backport"]
        issues = [x for x in issues if x.cve is not None]

    # Select unresolved backports
    if True:
        issues = [x for x in issues if x.issuetype() == "Backport"]

    for issue in issues:
        # print(issue.key, issue.cve)
        print(issue.key, issue.status())
        for link in issue.getlinks():
            print("    ", link)
            m = pr_re.search(link)
            if m is not None:
                pr = int(m.group(1))
                gpr = repo.get_pull(pr)
                print("        ", pr, gpr.state, gpr.title)
                for c in gpr.get_commits():
                    print("        ", pr, c)
                    # pp.pprint(c.raw_data)
                    print("            ", gitwork.describe(c.sha))

if __name__ == '__main__':
    main()
