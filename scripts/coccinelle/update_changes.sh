#!/bin/sh

if [ -z "$ZEPHYR_BASE" ]
then
    echo '$ZEPHYR_BASE not set' >&2
    exit 1
fi

list_of_changes="$1"

if [ -z "$list_of_changes" ]
then
    echo 'List of changed function names not given' >&2
    exit 1
fi

find "$ZEPHYR_BASE/$2" -type f -name '*.[hcS]' \( \
     -path '*/scripts/*' -prune -o \
     -path '*/outdir/*' -prune -o \
     -path '*/build/*' -prune -o \
     -path '*/sanity-out/*' -prune -o \
     -print \) |
    while read filename
    do
	echo "Checking $filename..."

	cat "$list_of_changes" |
	    while read from to rest
	    do
		if [ -z "$to" ]
		then
		    continue
		fi

		if egrep "(^|\\W)$from($|\\W)" "$filename" > /dev/null
		then
		    echo "  updating $from to $to"
		    cat "$filename" |
			sed -E "s/(^|\\W)${from}($|\\W)/\1${to}\2/g" \
			    > "${filename}.tmp" && mv "${filename}.tmp" "$filename"
		fi
	    done
    done
