#!/bin/bash
set -euo pipefail

./boot0.sh &
./boot1.sh &
./boot2.sh &
./boot3.sh &

wait

