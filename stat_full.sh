N=$1 
bench=$2
./stat.sh $N $bench sync >> ${N}_${bench}_full
./stat.sh $N $bench calc >> ${N}_${bench}_full
./stat.sh $N $bench calc_and_progress >> ${N}_${bench}_full
./stat.sh $N $bench mpich_progress >> ${N}_${bench}_full
./stat.sh $N $bench progress_penalty >> ${N}_${bench}_full

