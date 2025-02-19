import subprocess

from twisterlib.testplan import TestSuite
from plugin_filters.filter_framework import FilterFramework


class Filter(FilterFramework):
    def setup(self):
        main()
        return super().setup()
    def filter(self, suite: TestSuite):
        return True


# Get the closest common ancestor commit between main and the current branch
def get_common_ancestor_commit():
    result = subprocess.check_output(
        ['git', 'merge-base', 'main', 'HEAD']
    ).decode().strip()
    return result

# Get the list of changed files between the common ancestor commit and the current branch
def get_changed_files(common_ancestor_commit):
    diff_output = subprocess.check_output(
        ['git', 'diff', '--name-only', f'{common_ancestor_commit}..HEAD']
    ).decode().splitlines()
    return diff_output

# Run the tests for impacted Python files
def run_tests(modules_to_test):
    for module in modules_to_test:
        print(f"Running tests for: {module}")
        #subprocess.run(['pytest', module])

# Map Python tests to corresponding C modules
def map_tests_to_c_modules(changed_python_files):
    dependency_map = {
        "test_hello_1.py": ["module1.c", "module2.c"],  # Example mapping
        "test_hello_2.py": ["module3.c"],
    }
    impacted_c_modules = set()
    for test in changed_python_files:
        if test in dependency_map:
            impacted_c_modules.update(dependency_map[test])
    return impacted_c_modules

# Main function
def main():
    # Get the common ancestor commit
    common_ancestor_commit = get_common_ancestor_commit()

    # Get the list of changed files compared to the common ancestor commit
    changed_files = get_changed_files(common_ancestor_commit)

    # Separate Python files and C files
    changed_python_files = [f for f in changed_files if f.endswith('.py')]
    changed_c_files = [f for f in changed_files if f.endswith('.c')]

    # Map changed Python files to their dependent C modules
    #impacted_c_modules = map_tests_to_c_modules(changed_python_files)

    # Identify the tests to run based on impacted C files
    modules_to_test = []
    for c_file in changed_c_files:
        # You need to find the corresponding Python test module for the C file
        # You can use a naming convention or logic to match the C file to the test
        python_test = f"test_{c_file.replace('.c', '.py')}"
        modules_to_test.append(python_test)

    # Run tests on impacted modules
    run_tests(modules_to_test)
