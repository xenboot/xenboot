#!/bin/bash
#set -euo pipefail

lines=`xl list | wc -l`
let vms="$lines-2"
echo "$vms"
