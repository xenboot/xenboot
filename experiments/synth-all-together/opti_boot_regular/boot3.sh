#!/bin/bash
set -euo pipefail

if [ ! "$#" == "1" ]; then
	echo "Usage: $0 <num to boot>"
	exit
fi

KERNEL=mini-os.gz
MEMORY=32
TMPDIR="./.tmp3"
BASENAME=uk

START=376
STOP=500


# Clean tmp dir and output file
rm -rf $TMPDIR && mkdir -p $TMPDIR

for i in `seq $START $STOP`; do
        # Generate config file
        config_file=$TMPDIR/uk$i.cfg
        echo "kernel = \"$KERNEL\"" > $config_file
        echo "memory = $MEMORY" >> $config_file
        echo "on_crash = 'destroy'" >> $config_file
        echo "name = \"$BASENAME$i\"" >> $config_file
        echo "vcpus = 1" >> $config_file
        
        # Boot it
        xl create $config_file -q
	echo "  Booted $i / $NUM"
done

# cleanup tmp folder
rm -rf $TMPDIR
