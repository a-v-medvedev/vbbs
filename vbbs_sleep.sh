#!/bin/bash

tl=$(squeue -h -j "$SLURM_JOBID" -o '%l')
case "$tl" in
    UNLIMITED|NOT_SET)
        seconds=""
        ;;
    *-*)
        IFS='-:' read -r d h m s <<< "$tl"
        seconds=$((d*86400 + h*3600 + m*60 + s))
        ;;
    *:*:*)
        IFS=: read -r h m s <<< "$tl"
        seconds=$((h*3600 + m*60 + s))
        ;;
    *:*)
        IFS=: read -r m s <<< "$tl"
        seconds=$((m*60 + s))
        ;;
    *)
        seconds=$((t*60))
        ;;
esac

[ -z "$1" ] || seconds=$(("$1"*60))
[ -z "$seconds" ] && { echo "vbbs_sleep: can't guess my time limit."; exit 1; }

global_rank=${OMPI_COMM_WORLD_RANK:-${PMI_RANK:-${SLURM_PROCID:-}}}
if [ "$global_rank" == "0" ]; then
    set -x
    nodelist=$(squeue --Format="NodeList:2000" --noheader -j "${SLURM_JOBID}" | tail -n1 | awk '{print $1}')
    [ -v VBBS_PARAMS ] || export VBBS_PARAMS=$HOME/vbbs_hostfile
    export SLURM_NODELIST=$nodelist
    vbbs slurm_init || { echo "vbbs_sleep: error in slurm reservation handling: vbbs command failed."; exit 1; }
    cat $HOME/vbbs_hostfile
    set +x
fi

sleep $seconds

