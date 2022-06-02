import os
import subprocess

class Device:
        def __init__(self, coverage=False):
                self.proc = None
                self.coverage = coverage
                if self.coverage:
                        cmd = 'west build -b native_posix -- -DCONFIG_COVERAGE=y >log/build.log 2>&1'
                else:
                        cmd = 'west build -b native_posix>log/build.log 2>&1'
                subprocess.run(cmd, shell=True)

        def run(self):
                self.proc = subprocess.Popen(
                        'west build -b native_posix -t run >log/app.log 2>&1',
                        shell=True, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)

        def stop(self):
                if self.proc:
                        self.proc.terminate()
                if self.coverage:
                        subprocess.run('lcov --capture --directory ./ --output-file lcov.info -q --rc lcov_branch_coverage=1', shell=True)
                        subprocess.run('genhtml lcov.info --output-directory lcov_html -q --ignore-errors source --branch-coverage --highlight --legend', shell=True)
