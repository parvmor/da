#!/bin/bash

if [ "$1" = "" ] || [ "$2" = "" ]; then
  echo "Validates the submission (i.e., valid input for a simple test-case) for the given excercise"
  echo -e "usage: \n$ $0 <exercice_name (\"FIFO\" or \"LCausal\")> <language (\"C\" or \"JAVA\")>"
  exit 1
fi

# Time to wait for correct processes to broadcast all messages (in seconds).
# Should be adapted to the number of messages to send.
time_to_finish=40
init_time=2

# Configure lossy network simulation.
sudo tc qdisc add dev lo root netem 2>/dev/null
sudo tc qdisc change dev lo root netem delay 50ms 200ms loss 10% 25% reorder 25% 50%

# Create default Makefile if not existing.
if [ ! -f Makefile ]; then
  echo "WARNING: Makefile not found! using default Makefile"
  if [ "$2" = "C" ]; then
    cp Makefile_c_example Makefile
  else
    cp Makefile_java_example Makefile
  fi
fi

# Compile (should output: da_proc or Da_proc.class).
make clean
make

# prepare input
if [ "$1" = "FIFO" ]; then
  echo "writing FIFO input..."
  echo "5
1 127.0.0.1 12001
2 127.0.0.1 12002
3 127.0.0.1 12003
4 127.0.0.1 12004
5 127.0.0.1 12005" > membership
else
  echo "writing LCausal input..."
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
fi

# Start 5 processes, each broadcasting 100 messages.
for i in `seq 1 5`; do
  if [ "$2" = "C" ]; then
    ./da_proc $i membership 100 &
  else
    java Da_proc $i membership 100 &
  fi
  da_proc_id[$i]=$!
done

# Leave some time for process initialization.
sleep $init_time
# Do some nasty stuff like process crashes and delays.
kill -STOP "${da_proc_id[3]}"
sleep 1
kill -TERM "${da_proc_id[2]}"
da_proc_id[2]=""
kill -CONT "${da_proc_id[3]}"

# Start broadcasting.
for i in `seq 1 5`; do
  if [ -n "${da_proc_id[$i]}" ]; then
    kill -USR2 "${da_proc_id[$i]}"
  fi
done

# Do some more nasty stuff.
kill -TERM "${da_proc_id[4]}"
da_proc_id[4]=""
kill -STOP "${da_proc_id[1]}"
sleep 0.5
kill -CONT "${da_proc_id[1]}"

# Leave some time for the correct processes to broadcast all messages.
sleep $time_to_finish
# Stop all processes.
for i in `seq 1 5`; do
  if [ -n "${da_proc_id[$i]}" ]; then
    kill -TERM "${da_proc_id[$i]}"
  fi
done

# Wait until all processes stop.
for i in `seq 1 5`; do
  if [ -n "${da_proc_id[$i]}" ]; then
    wait "${da_proc_id[$i]}"
  fi
done

if [ "$1" = "FIFO" ]; then
  # Check logs for correctness.
  ./check_output.sh 1 3 5
else
  ./check_output_lc.sh
fi

echo "Correctness test done."
