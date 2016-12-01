#!/bin/bash
set -euo pipefail

source Config.mk

killall xl &> /dev/null || true
xl server &

rm -rf $RESULT_FILE &> /dev/null || true
i=1
start=$SECONDS
while true; do
    config_file=$TMPDIR/uk$i.cfg

    xl client create $config_file
    echo "$i" >> $RESULT_FILE
	
    duration=$(( SECONDS - start ))
    if [ $duration -gt $SECONDS_RUN ]; then
        exit
    fi

    vms=`./$GET_NUM_VMS`
    while [ ! $vms -lt $CPUS_FOR_VMS ]; do
        echo "system congested, sleeping"
        sleep $SLEEP_SECS
        vms=`./$GET_NUM_VMS`
    done

    let i="$i+1"
done
