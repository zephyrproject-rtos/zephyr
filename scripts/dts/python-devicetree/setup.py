# Copyright (c) 2021, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import setuptools

long_description = '''
Placeholder
===========

This is just a placeholder for moving Zephyr's devicetree libraries
to PyPI.
'''

version = '0.0.2'

setuptools.setup(
    # TBD, just use these for now.
    author='Zephyr Project',
    author_email='devel@lists.zephyrproject.org',
    name='devicetree',
    version=version,
    description='Python libraries for devicetree',
    long_description=long_description,
    # http://docutils.sourceforge.net/FAQ.html#what-s-the-official-mime-type-for-restructuredtext-data
    long_description_content_type="text/x-rst",
    url='https://github.com/zephyrproject-rtos/python-devicetree',
    packages=setuptools.find_packages(where='src'),
    package_dir={'': 'src'},
    classifiers=[
        'Programming Language :: Python :: 3 :: Only',
        'License :: OSI Approved :: Apache Software License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
    ],
    install_requires=[
        'PyYAML>=6.0',
    ],
    python_requires='>=3.6',
)
