#!/bin/bash
#
#  chk_sanity
#
#	 first paramter is a file that has been captured by:
#		  sanity_chk -A x86 -T gcc > sanity.out.txt 2>&1
#		  or equivalent
#	 second parameter contains the name of the data file.
#		  The data file has the items to check.
#		  Each line of the file is a different check.
#		  The format is:
#			   regular expression|count
#		  for example:
#		  ^VXMICRO EXECUTION PROJECT SUCCESSFUL$|114
#
#		  The expectation is that many different pattern
#		  files will be created for different
#		  configurations.
#
#	 usage example:
#		  sanity_chk -A x86 -T gcc > sanity.out.txt
#		  ./chk_sanity.sh sanity.out.txt patterns_sanity_x86_gcc.txt
#
#	 return code
#		  0 = no failures
#		  non-zero = number of failures
#
usage()
{
	 echo Usage: chk_sanity captured_output_file pattern_file
	 echo \tpattern file contains:  regular expression\|count
}

if [ ! -e "$1" ] ; then
	 echo "ERROR input specifies an output file that does not exist"
	 usage
	 exit 1
fi

if [ ! -e "$2" ] ; then
	 echo "ERROR pattern file does not exist"
	 usage
	 exit 1
fi

RTNCODE=0
while read f; do
	 RE_STR=`echo $f | cut -d "|" -f 1`
	 COUNT=`echo $f | cut -d "|" -f 2`
	 FOUND=`grep "${RE_STR}" $1 | wc -l` 
	 if [ $FOUND -ne $COUNT ] ; then
		  echo FAIL for expression $RE_STR expected $COUNT instances but found $FOUND
		  let "RTNCODE+=1"
	 else
		  echo PASS for expression $RE_STR
	 fi
done < $2
exit $RTNCODE
