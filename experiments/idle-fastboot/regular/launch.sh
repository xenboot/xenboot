#!/bin/bash
set -euo pipefail

RANGE="2 4 8 16 32 63 128 256 500"
#RANGE="2 4"
CHRONO="chrono/chronoquiet"

for i in $RANGE; do
	res=`$CHRONO ./boot_x_uks.sh $i`
	./destroy.sh $i
	echo "$i;$res"
done
