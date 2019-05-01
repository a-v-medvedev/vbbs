set +x

SB=`which sbatch`
if [ $(basename $SB) != "sbatch" ]; then echo "No SLURM utilities in path!"; exit 1; fi

handle_out() {
	ID=$1
	echo "$N  --- $bench" $2
    if [ $NAME == "sync" ]; then
        cat results.$ID/out* | awk '{if (NF>8) { T=$9; print T*1000000; } }'
    elif [ $NAME == "pure" ]; then
        cat results.$ID/out* | awk '{if (NF>8) { T=$13; print T*1000000; } }'
    elif [ $NAME == "calc_and_progress" ]; then        
        cat results.$ID/out* | awk '{if (NF>8) { T=$13; print T*1000000; } }'
    elif [ $NAME == "calc" ]; then        
        cat results.$ID/out* | awk '{if (NF>8) { T=$13; print T*1000000; } }'
    elif [ $NAME == "mpich_progress" ]; then
        cat results.$ID/out* | awk '{if (NF>8) { T=$13; print T*1000000; } }'
        echo "$N  --- $bench" "--- progress_penalty"
    	cat results.$ID/out* | awk '{if (NF>8) { TC=$15; print TC*1000000 " "; } }'
    fi
}


run_and_handle_out() {
    AAA=`mktemp`
    vbbs start $N > $AAA
    if [ $? != 0 ]; then echo "VBBS failed on start"; return; fi
    ID=`cat $AAA | grep 'id:' | cut -d ' ' -f2`
    if [ "$ID" == "" ]; then echo "VBBS failed on start"; return; fi
    cat $AAA | grep 'node:' | cut -d ' ' -f2 > hostfile.$ID
    if [ `cat hostfile.$ID | wc -l` != $N ]; then echo "VBBS failed on start"; return; fi
    rm $AAA
    
    NP=`expr $N \* 14`
    KIND=async
    if [ $NAME == "mpich_progress" ]; then
        export MPICH_ASYNC_PROGRESS=1
        THREAD_LEVEL=multiple
        WORKLOAD=calc
    elif [ $NAME == "calc" ]; then
        export MPICH_ASYNC_PROGRESS=0
        THREAD_LEVEL=single
        WORKLOAD=calc
    elif [ $NAME == "calc_and_progress" ]; then
        export MPICH_ASYNC_PROGRESS=0
        THREAD_LEVEL=single
        WORKLOAD=calc_and_progress
    elif [ $NAME == "pure" ]; then
        export MPICH_ASYNC_PROGRESS=0
        THREAD_LEVEL=single
        WORKLOAD=none
    elif [ $NAME == "sync" ]; then
        export MPICH_ASYNC_PROGRESS=0
        THREAD_LEVEL=single
        WORKLOAD=none
        KIND=sync
    fi
    if [ "$calctime" == "" ]; then
        echo ">> calctime is empty for N=$N and bench=$bench"
        return
    fi
    BINARY_TO_RUN=IMB-ASYNC
    realbinarytorun=`which $BINARY_TO_RUN`
    tmpbinarytorun=$(mktemp --tmpdir=/tmp impi-bin-XXXXXX)
    cp "$realbinarytorun" $tmpbinarytorun
    chmod a+x $tmpbinarytorun
    for node in `cat hostfile.$ID`; do ssh $node "cp $realbinarytorun $tmpbinarytorun; chmod a+x $tmpbinarytorun"; done
    mpiexec.hydra -hostfile hostfile.$ID -n $NP -ppn 14 --errfile-pattern=err.$ID.%r --outfile-pattern=out.$ID.%r "$tmpbinarytorun" ${KIND}_$bench -cper10usec $CPER10USEC -workload $WORKLOAD -calctime $calctime -thread_level $THREAD_LEVEL >& mpirun.$ID.out
    for node in `cat hostfile.$ID`; do ssh $node "rm -f $tmpbinarytorun"; done
    mkdir results.$ID
    mv -f out.$ID.* err.$ID.* mpirun.$ID.* hostfile.$ID results.$ID >& /dev/null
    touch results.$ID/$NAME
    vbbs stop $ID
    if [ $? != 0 ]; then echo "VBBS failed on stop"; return; fi
	handle_out $ID " ---  $NAME"
}

function set_calc_time() {
    CPER10USEC=45
    if [ "$N" -eq "2" -a "$bench" == "pt2pt" ]; then calctime=10,10,100,1000,20000; fi
    if [ "$N" -eq "2" -a "$bench" == "allreduce" ]; then calctime=20,20,100,500,20000; fi
    if [ "$N" -eq "16" -a "$bench" == "pt2pt" ]; then calctime=10,10,200,5000,100000; fi
    if [ "$N" -eq "16" -a "$bench" == "allreduce" ]; then calctime=50,50,200,1000,20000; fi
    if [ "$N" -eq "64" -a "$bench" == "pt2pt" ]; then calctime=10,20,500,5000,100000; fi
    if [ "$N" -eq "64" -a "$bench" == "allreduce" ]; then calctime=100,100,200,1000,20000; fi
}

N=64
for NAME in sync calc calc_and_progress mpich_progress; do
    bench=pt2pt
    set_calc_time
    run_and_handle_out

    bench=allreduce
    set_calc_time
    run_and_handle_out
done


#N=16
#bench=pt2pt
#calctime=10,10,100,1000,20000
#run_and_handle_out
#
#bench=allreduce
#calctime=10,20,100,500,10000
#run_and_handle_out
#

#N=64
#bench=pt2pt
#calctime=10,20,500,5000,100000
#run_and_handle_out

#bench=allreduce
#calctime=50,50,100,1000,20000
#run_and_handle_out

#N=128
#bench=pt2pt
#calctime=10,20,500,5000,100000
#run_and_handle_out

#bench=allreduce
#calctime=50,50,100,1000,20000
#run_and_handle_out

#N=64
#bench=pt2pt
#...

#bench=allreduce
#...




