#!/bin/bash
filename=$1
ret=0
if [ $# -eq 0 ]
  then
    echo "patch file is missing."
    exit 1
fi
if [ -s $1 ]
then
# run checkpatch over each patch file
while read p;
do
  perl scripts/checkpatch.pl $p
  ret=`expr $ret + $?`
done < "$filename"
else
echo "patch file is empty."
exit 1
fi
exit $ret
