#!/bin/bash
set -euo pipefail

if [ ! "$#" == "1" ]; then
	echo "Usage: $0 <Num to destroy>"
	exit
fi

BASENAME=uk

NUM=$1

for i in `seq 1 $NUM`; do
        xl client destroy $BASENAME$i
done

