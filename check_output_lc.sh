#!/bin/bash
#
# Checks the output of processes.
#

echo -e "\nTESTING PROCESS OUTPUT.\n"

echo "Checking LCausal."
python check_lc.py
if [ $? -ne 0 ]; then
    echo "Error."
    echo -e "\nTEST FAILED.\n"
    exit 1
fi

echo -e "\nTEST SUCCEEDED.\n"

touch PASS
