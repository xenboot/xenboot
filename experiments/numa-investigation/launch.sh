#!/bin/bash
set -euo pipefail

MEM="32 64 128 256 512 1024 2048 4096 8192 16384"
CHRONO="./tools/chrono/chronoquiet"
UKNAME="uk0"
TMP=".tmp.xl"

echo "node;memory size;create time; destroy time"

for memory in $MEM; do
	for i in `seq 0 7`; do
		cat node${i}.xl | sed "s/^memory.*$/memory = $memory/" > $TMP
		create=`$CHRONO xl create $TMP`
		destroy=`$CHRONO xl destroy $UKNAME`
		echo "$i;$memory;$create;$destroy"
	done
done
