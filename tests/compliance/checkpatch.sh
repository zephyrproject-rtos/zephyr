#! /bin/bash -e
#
# @testcase static defaults=none
# @eval shcmd %(thisfile)s %(srcdir)s/../../
# @tags bat
#
# If checkpatch.pl is not on your path, you can:
#   1 - put it in your path (copy or symlink)
#   2 - export CPPPATH to the path where it is
#
# (1) is easier
#
set -eu -o pipefail
cd $1 || exit 127
# If no modifications to the tree, check the previous commit;
# otherwise, show the current modifications
if git diff-index HEAD --quiet; then
    git show --format=email HEAD || exit 127
else
    git diff HEAD || exit 127
fi  \
    | ${CPPATH:-scripts/}checkpatch.pl \
           ${CPOPTS:---no-tree --patch --show-types --mailback --terse --showfile --ignore FILE_PATH_CHANGES } \
           -
