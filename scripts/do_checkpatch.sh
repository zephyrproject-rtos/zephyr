#!/bin/sh

#
# Copyright (c) 2015 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

[ -f $1 ] && rm $1
[ -f $1_error.types ] && rm $1_error.types
[ -f $1_warning.types ] && rm $1_warning.types

dirs_to_check="arch drivers include kernel lib"
files=$(for d in ${dirs_to_check}; do find $d/ -type f -name '*.[ch]'; done)
for i in $files; do
	${ZEPHYR_BASE}/scripts/checkpatch.pl --mailback --no-tree -f --emacs --summary-file --show-types --ignore BRACES,PRINTK_WITHOUT_KERN_LEVEL,SPLIT_STRING --max-line-length=100 $i >> $1
done
grep ERROR: $1 |cut -d : -f 3,4 |sort -u > $1_error.types
grep WARNING: $1 |cut -d : -f 3,4 |sort -u > $1_warning.types
for i in `cat $1_error.types`; do
	echo -n $i ' '; grep $i $1 | wc -l
done
for i in `cat $1_warning.types`; do
	echo -n $i ' '; grep $i $1 | wc -l
done
