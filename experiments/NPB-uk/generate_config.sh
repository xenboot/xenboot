#!/bin/bash
set -euo pipefail

source Config.mk

rm -rf $TMPDIR &> /dev/null || true
mkdir $TMPDIR


for i in `seq 1 $1`; do
    let cpu="$i%$CPUS_FOR_VMS + $CPUS_FOR_DOM0"                                                       
    # Generate config file 
    config_file=$TMPDIR/uk$i.cfg                                                                      
    echo "kernel = \"$KERNEL\"" > $config_file                                                        
    echo "memory = $MEMORY" >> $config_file                                                           
    echo "on_crash = 'destroy'" >> $config_file                                                       
    echo "name = \"$BASENAME$i\"" >> $config_file                                                     
    echo "vcpus = 1" >> $config_file                                                                  
    echo "cpus = \"$cpu\"" >> $config_file                                                            
done
