N=0

function wait_if_necessary {
    amount=$1
    do_wait=1
    while [ $do_wait -eq 1 ]; do
            count=`jobs -p | wc -l`
            do_wait=$(($count >= $amount))
            echo "Now $count jobs are running, wait..."
            sleep 10
    done
}

function run_task {
        cmd=$1
        output=$2
        concurency=$3
        count=`jobs -p | wc -l`
        N=$(expr $N \+ 1)
        echo "New active task #$count:  $cmd > ${output}.$N"
        $cmd > ${output}.$N && touch ${output}.${N}.done &
        wait_if_necessary $concurency
}

while [ "$N" -lt 8 ]; do
    run_task "jobrun.sh -n 16 -p 14 -o ./run.options.compute" jobrun_log 4
done

wait_if_necessary 1
