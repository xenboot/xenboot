#!/bin/bash
set -euo pipefail

NUM=$1
PREFIX="uk"

for i in `seq  $NUM`; do
	xl destroy $PREFIX$i
done
