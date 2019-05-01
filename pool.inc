POOL_N=0
POOL_LOCKED=0
POOL_CNT=0

function wait_if_necessary {
    amount=$1
    while 
            count=`jobs -rp | wc -l`
            do_wait1=$(($count >= $amount))
            if [ -f lock ]; then do_wait2=1; POOL_LOCKED=`expr $POOL_LOCKED \+ 1`; else do_wait2=0; POOL_LOCKED=0; fi
            if [ "$do_wait1" -eq "1" -a "$count" -gt "$POOL_CNT" ]; then
                echo "Now $count jobs are running, wait..."
            fi
            if [ "$do_wait2" -eq "1" -a "$POOL_LOCKED" -eq "1" ]; then
                echo "Lock file found, wait..."
            fi
            POOL_CNT=$count
            sleep 1
            [ "$do_wait1" -eq 1 -o "$do_wait2" -eq 1 ]
    do
        continue;
    done
}

function run_task {
        cmd=$1
        output=$2
        concurency=$3
        wait_if_necessary $concurency
        count=`jobs -rp | wc -l`
        count=`expr $count \+ 1`
        POOL_N=$(expr ${POOL_N} \+ 1)
        echo "New active task #$count:  $cmd > ${output}.${POOL_N}"
        $cmd > ${output}.${POOL_N} && touch ${output}.${POOL_N}.done &
}

