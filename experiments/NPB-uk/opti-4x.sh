#!/bin/bash
set -euo pipefail

source Config.mk

killall xl &> /dev/null || true
xl server &

rm -rf $RESULT_FILE &> /dev/null || true
i=1
start=$SECONDS
while true; do
	let i_1="$i+1"
	let i_2="$i+2"
	let i_3="$i+3"
    config_file0=$TMPDIR/uk$i.cfg
    config_file1=$TMPDIR/uk$i_1.cfg
    config_file2=$TMPDIR/uk$i_2.cfg
    config_file3=$TMPDIR/uk$i_3.cfg

    xl client create $config_file0 $config_file1 $config_file2 $config_file3
    
    echo "$i_3" >> $RESULT_FILE
	
    duration=$(( SECONDS - start ))
    if [ $duration -gt $SECONDS_RUN ]; then
        exit
    fi

    vms=`./$GET_NUM_VMS`
    let needed="$CPUS_FOR_VMS-3"
    while [ ! $vms -lt $needed ]; do
        echo "system congested, sleeping"
        sleep $SLEEP_SECS
        vms=`./$GET_NUM_VMS`
    done

    let i="$i+4"
done
