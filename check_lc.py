#!/usr/bin/python3 -u

import sys
from collections import defaultdict

i = 1
l = 1
nextMessage = defaultdict(lambda : 1)

num_processes = 0;

dependencies = {}
with open("membership") as f:
    linecounter = 0
    for line in f:
        if linecounter == 0:
            num_processes = int(line)
        if linecounter > num_processes:
            tokens = line.split()
            dependencies[int(tokens[0])] = list(map(int,tokens))
        linecounter += 1

dictionaries = {}


for j in range(num_processes):
    i = 1
    l = 1
    vc = [0] * num_processes
    dic = {}
    with open("da_proc_" + str(j+1) + ".out") as f:
        for line in f:
            tokens = line.split()
            
            # Check broadcast
            if tokens[0] == 'b':
                msg = int(tokens[1])
                if msg != i:
                    print("Line {0}: Messages broadcast out of order. Expected message {1} but broadcast message {2}".format(l, i, msg))
                    sys.exit(1)
                dic[i] = vc.copy()
                i += 1

            # Check delivery
            if tokens[0] == 'd':
                sender = int(tokens[1])
                msg = int(tokens[2])
                if sender in dependencies[j+1]:
                    vc[sender-1] += 1
                
            # Increment line number counter
            l += 1
    dictionaries[j+1] = dic


for j in range(num_processes):
    l = 1
    vc = [0] * num_processes
    dic = {}
    with open("da_proc_" + str(j+1) + ".out") as f:
        for line in f:
            tokens = line.split()
            
            # Check delivery
            if tokens[0] == 'd':
                sender = int(tokens[1])
                msg = int(tokens[2])
                if any([x < y for (x,y) in zip(vc,dictionaries[sender][msg])]):
                    print("Line {0}: Messages delivered out of order by process {1}. Required VC {2}, but actual VC is {3}".format(l, j+1,dictionaries[sender][msg], vc))
                    sys.exit(1)
                else:
                    vc[sender-1] += 1

            # Increment line number counter
            l += 1
