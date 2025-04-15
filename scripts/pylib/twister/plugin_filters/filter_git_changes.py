import subprocess

from twisterlib.testplan import TestSuite
from plugin_filters.filter_framework import FilterFramework


class Filter(FilterFramework):
    def setup(self):
        self.generate_change_list()
        return super().setup()

    def filter(self, suite: TestSuite):
        return True

    def get_common_ancestor_commit(self):
        result = subprocess.check_output(
            ['git', 'merge-base', 'main', 'HEAD']
        ).decode().strip()
        return result

    def get_changed_files(self, common_ancestor_commit):
        diff_output = subprocess.check_output(
            ['git', 'diff', '--name-only', f'{common_ancestor_commit}..HEAD']
        ).decode().splitlines()
        return diff_output

    def generate_change_list(self):
        common_ancestor_commit = self.get_common_ancestor_commit()
        self.changed_files = self.get_changed_files(common_ancestor_commit)

        self.changed_c_files = [ file for file in self.changed_files if file.endswith('.c') or file.endswith('.h') ]
