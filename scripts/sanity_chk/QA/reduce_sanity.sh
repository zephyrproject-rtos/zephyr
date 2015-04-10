#!/bin/bash
#
#	 reduce_sanity.sh
#
#	 There are lots of lines that are output
#	 by sanity_chk that are not needed if the
#	 reader is just skimming for errors.
#	 This script removes lots of the most
#	 obvious ones.
#
#	 Parameter one:  input filename
#	 Output to stdout
#
if [ ! -e "${1}" ] ; then
	 echo ERROR filename with content required as parameter
	 exit 1
fi
grep -v "^\[gcc\]" $1 | \
grep -v "^\[Compiling\]" | \
grep -v "^\[Linking\]" | \
grep -v "^make\[.\]: Entering" | \
grep -v "^make\[.\]: Leaving" | \
grep -v "^Build complete\.$" | \
grep -v "^Using [0-9]* threads" | \
grep -v "\*-=\*-="
