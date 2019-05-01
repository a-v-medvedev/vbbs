#!/bin/bash

. ~/bin/pool.inc

while [ "$POOL_N" -lt 8 ]; do
    run_task "jobrun.sh -n 16 -p 14 -o ./run.options.compute" jobrun_log 4
#    run_task "sleep 1" jobrun_log 4
done

wait_if_necessary 1
