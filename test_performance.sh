#!/bin/bash
#
# Tests the performance of the Uniform Reliable Broadcast application.
#
# usage: ./test_performance evaluation_time
#
# evaluation_time: Specifies the number of seconds the application
#                  should run.
#

sudo tc qdisc delete dev lo root netem 2>/dev/null

evaluation_time=$1
init_time=2

echo "20
1 127.0.0.1 11001
2 127.0.0.1 11002
3 127.0.0.1 11003
4 127.0.0.1 11004
5 127.0.0.1 11005
6 127.0.0.1 11006
7 127.0.0.1 11007
8 127.0.0.1 11008
9 127.0.0.1 11009
10 127.0.0.1 11010
11 127.0.0.1 11011
12 127.0.0.1 11012
13 127.0.0.1 11013
14 127.0.0.1 11014
15 127.0.0.1 11015
16 127.0.0.1 11016
17 127.0.0.1 11017
18 127.0.0.1 11018
19 127.0.0.1 11019
20 127.0.0.1 11020" > membership


#start 20 processes
for i in `seq 1 20`
do
    ./da_proc $i membership 1000 &
    da_proc_id[$i]=$!
    echo ${da_proc_id[$i]}
done

#leave some time for process initialization
sleep ${init_time}

#start broadcasting
echo "Evaluating application for ${evaluation_time} seconds."
for i in `seq 1 20`
do
  if [ -n "${da_proc_id[$i]}" ]; then
    kill -USR1 "${da_proc_id[$i]}"
  fi
done

#let the processes do the work for some time
sleep ${evaluation_time}

#stop all processes
for i in `seq 1 20`
do
    if [ -n "${da_proc_id[$i]}" ]; then
    echo ${da_proc_id[$i]}
	kill -TERM "${da_proc_id[$i]}"
    fi
done

#wait until all processes stop
for i in `seq 1 20`
do
    echo ${da_proc_id[$i]}
	wait "${da_proc_id[$i]}"
done

#count delivered messages in the logs
#... (not implemented here)

echo "Performance test done."
