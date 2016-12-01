#!/bin/bash
set -euo pipefail

if [ ! "$#" == "1" ]; then
	echo "Usage: $0 <Num to boot>"
	exit
fi

KERNEL=mini-os.gz
MEMORY=32
TMPDIR="./.tmp"
BASENAME=uk

NUM=$1

killall xl &> /dev/null || true

xl server &
sleep 1

# Clean tmp dir and output file
rm -rf $TMPDIR && mkdir -p $TMPDIR

for i in `seq 1 $NUM`; do
        # Generate config file
        config_file=$TMPDIR/uk$i.cfg
        let cpu="$i%44 + 4"
        echo "kernel = \"$KERNEL\"" > $config_file
        echo "memory = $MEMORY" >> $config_file
        echo "on_crash = 'destroy'" >> $config_file
        echo "name = \"$BASENAME$i\"" >> $config_file
        echo "vcpus = 1" >> $config_file
        echo "cpus = \"$cpu\"" >> $config_file
        
        # Boot it
        xl client create $config_file
#	echo "  Booted $i / $NUM"
done

# cleanup tmp folder
# rm -rf $TMPDIR
