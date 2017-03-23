#!/bin/sh
rm results.txt
rm -rf results
for conf in `ls -v *.conf`; do
	echo $conf
	outdir=results/$(basename $conf .conf)
	app=$(basename $conf .conf)
	make BOARD=arduino_101 CONF_FILE=$conf O=${outdir}
	${ZEPHYR_BASE}/scripts/sanitycheck -z ${outdir}/${app}.elf >> results.txt
done

