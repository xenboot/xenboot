#!/bin/bash
set -euo pipefail

NUM=$1
KERNEL="mini-os-idle.gz"
UK_MEMORY=32
TMP_FOLDER=".tmp/"
BASENAME="uk"

#1. Generate config files
rm -rf $TMP_FOLDER && mkdir $TMP_FOLDER
for i in `seq 1 $NUM`; do
	config_file=$TMP_FOLDER/uk$i.cfg
	echo "kernel = \"$KERNEL\"" > $config_file
	echo "memory = $UK_MEMORY" >> $config_file
	echo "on_crash = 'destroy'" >> $config_file
	echo "name = \"$BASENAME$i\"" >> $config_file
	echo "vcpus = 1" >> $config_file
done


#2. Boot everything in a parallel loop
for i in `seq 1 $NUM`; do
	xl create $TMP_FOLDER/uk$i.cfg &
done
wait
