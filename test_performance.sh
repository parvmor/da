#!/bin/bash
#
# Tests the performance of the Uniform Reliable Broadcast application.
#
# usage: ./test_performance evaluation_time
#
# evaluation_time: Specifies the number of seconds the application
#                  should run.
#

evaluation_time=$1
init_time=2

echo "5
1 127.0.0.1 12001
2 127.0.0.1 12002
3 127.0.0.1 12003
4 127.0.0.1 12004
5 127.0.0.1 12005
1 4 5
2 1
3 1 2
4
5 3 4" > membership

#start 5 processes
for i in `seq 1 5`
do
    # valgrind \
    #   --leak-check=full \
    #   --show-leak-kinds=all \
    #   --track-origins=yes \
    #   --verbose \
    #   --log-file=valgrind-logs/logs-$i.txt \
    #   ./da_proc $i membership 3000 &
    # valgrind --tool=helgrind \
    #   --log-file=helgrind-logs/logs-$i.txt \
    #   ./da_proc $i membership 3000 &
    ./da_proc $i membership 10000 &
    da_proc_id[$i]=$!
done

#leave some time for process initialization
sleep ${init_time}

#start broadcasting
echo "Evaluating application for ${evaluation_time} seconds."
for i in `seq 1 5`
do
  if [ -n "${da_proc_id[$i]}" ]; then
    kill -USR1 "${da_proc_id[$i]}"
  fi
done

#let the processes do the work for some time
sleep ${evaluation_time}

#stop all processes
for i in `seq 1 5`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -TERM "${da_proc_id[$i]}"
    fi
done

#wait until all processes stop
for i in `seq 1 5`
do
	wait "${da_proc_id[$i]}"
done

./check_lc.py
if [ ! $? -eq 0 ]; then
  echo "LCausal order is not respected."
fi

#count delivered messages in the logs
#... (not implemented here)
for i in `seq 1 5`; do
  msg=$( cat da_proc_$i.out | grep '^d' | wc -l )
  echo "Process $i delivered $msg."
done

echo "Performance test done."
