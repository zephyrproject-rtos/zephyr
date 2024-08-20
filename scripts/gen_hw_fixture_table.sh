#! /bin/bash
#
# Copyright (c) 2024 Linutronix GmbH
#
# SPDX-License-Identifier: Apache-2.0
#

if [ x"$1" == "x" ]
then
    echo "usage: $0 <hardware-fixture-table>"
    exit 1
fi

TOPDIR=$(git rev-parse --show-toplevel)

find $TOPDIR -name testcase.yaml -exec awk '/fixture:/{print $2}' {} \; | \
    sort -u | \
    awk -voutfile="$1" '
BEGIN {
	count = 1;
	max_desc = 11;
	max_fix = 0;
	hw_fixtures[0] = "Hardware fixture";
	hw_fixtures_desc[hw_fixtures[0]] = "Description";

	mode = 0
	PREAMBLE = "";
	FISH = "";
	while ((ret = getline < outfile) > 0) {
		switch (mode) {
		case 0:
			if (index($0, "+--")) {
				mode = 1;
				saveFS = FS;
				FS = "|";
			} else
				PREAMBLE = PREAMBLE $0 "\n";
			break;
		case 1:
			if (index($0, "+--"))
				break;
			else if (index($0, "|")) {
				gsub("\\s", "", $2);
				desc = substr($3, 2, length($3) - 2)
				hw_fixtures_desc[$2] = desc;
				max_desc = max_desc > length(desc) ? max_desc : length(desc);
			} else {
				mode = 2;
				FS = saveFS;
				FISH = $0 "\n";
			}
			break;
		default:
			FISH = FISH $0 "\n"
		}
	}
	if (ret < 0) {
		print("unexpected error:", ERRNO) > "/dev/stderr"
		exit
	}
	close(outfile);
}
{
	hw_fixtures[count++] = $1;
	max_fix = max_fix > length($1) ? max_fix : length($1);
}
END {
	lim_desc = ""
	for (i = 0; i < max_desc + 2; i++) lim_desc = lim_desc"-";

	lim_fix = ""
	for (i = 0; i < max_fix + 2; i++) lim_fix = lim_fix"-";

	printf ("%s", PREAMBLE) > outfile;

	print "+"lim_fix"+"lim_desc"+" > outfile;
	for (i = 0; i < count; i++) {
		fix = hw_fixtures[i];
		desc = hw_fixtures_desc[fix];

		printf("| %s", fix) > outfile;
		for (t = 0; t < max_fix - length(fix); t++)
			printf(" ") > outfile;
		printf (" | %s", desc)  > outfile;
		for (t = 0; t < max_desc - length(desc); t++)
			printf(" ") > outfile;
		print " |" > outfile;
		print "+"lim_fix"+"lim_desc"+" > outfile;
	}

	printf ("%s", FISH) > outfile;
}'
