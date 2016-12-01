#!/bin/bash
set -euo pipefail

source Config.mk 

rm -rf $FILE_STOP $RESULT_FILE $TMPDIR &> /dev/null || true
mkdir $TMPDIR
i=1
start=$SECONDS
while [ ! -e $FILE_STOP ]; do
    let cpu="$i%$CPUS_FOR_VMS + $CPUS_FOR_DOM0"
    # Generate config file
    config_file=$TMPDIR/uk$i.cfg
    echo "kernel = \"$KERNEL\"" > $config_file
    echo "memory = $MEMORY" >> $config_file
    echo "on_crash = 'destroy'" >> $config_file
    echo "name = \"$BASENAME$i\"" >> $config_file
    echo "vcpus = 1" >> $config_file
    echo "cpus = \"$cpu\"" >> $config_file

    xl create $config_file -q
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
