#!/bin/sh

#
# Copyright (c) 2015 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1) Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2) Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3) Neither the name of Intel Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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
