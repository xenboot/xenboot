#!/bin/bash
set -euo pipefail

xl client create config1.xl
sleep .5
xl client create config2.xl
sleep .5

xl client destroy sample1 sample2
