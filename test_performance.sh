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
1 1 3 5
2 1 2 3 5
3 1 3 4 5
4 1 2 5
5 1 2 5" > membership

for i in `seq 1 5`; do
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
  ./da_proc $i membership 4000000 &
  da_proc_id[$i]=$!
done

sleep ${init_time}

echo "Evaluating application for ${evaluation_time} seconds."
for i in `seq 1 5`; do
  if [ -n "${da_proc_id[$i]}" ]; then
    kill -USR1 "${da_proc_id[$i]}"
  fi
done

sleep ${evaluation_time}

for i in `seq 1 5`; do
  if [ -n "${da_proc_id[$i]}" ]; then
	  kill -TERM "${da_proc_id[$i]}"
  fi
done

for i in `seq 1 5`; do
  wait "${da_proc_id[$i]}"
done

./check_lc.py
if [ ! $? -eq 0 ]; then
  echo "LCausal order is not respected."
  exit 1
fi

for i in `seq 1 5`; do
  msg=$( cat da_proc_$i.out | grep '^d' | wc -l )
  echo "Process $i delivered $msg."
done

echo "Performance test done."
