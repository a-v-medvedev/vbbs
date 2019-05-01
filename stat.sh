#!/bin/bash
set -e 

NP=$1
BENCH=$2
MODE=$3
NAME=${NP}_${BENCH}_${MODE}

cat script7_log.* | awk "/^$NP  --- $BENCH --- $MODE/ { S=1; next; } /---/ { S=0; next; } S > 0 { A[S] = A[S] \$1 \",\"; S++; } END { for (i=1; i<=5; i++) print A[i] }" > $NAME
echo "$NP  ---  $BENCH  ---  $MODE"
for i in 1 2 3 4 5; do
    head -n $i $NAME | tail -n 1 | sed 's/,/\n/g' | grep -v '^[ ]*$' | sort -n > ${NAME}_sorted
    N=`cat ${NAME}_sorted | wc -l`
    NM=`expr $N / 2`
    MED=`cat ${NAME}_sorted | awk "NR == $NM { print }"`
    MIN=`head -n 1 ${NAME}_sorted | tail -n 1`
    MAX=`tail -n 1 ${NAME}_sorted | head -n 1`
    echo $MIN $MAX $MED
done

