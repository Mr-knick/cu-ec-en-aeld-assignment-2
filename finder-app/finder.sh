#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Invalid input:"
    echo "Usage: ./finder.sh <filedir> <string>"
    exit 1
fi

filesdir="$1"
searchstr="$2"

if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir does not exist."
    exit 1
fi

echo "Searching $filesdir for $searchstr"

results=$(grep -rIHc "$searchstr" "$filesdir")

file_count=$(echo "$results" | awk -F: '$NF > 0 {count++} END {print count+0}')
match_count=$(echo "$results" | awk -F: '{sum+=$NF} END {print sum+0}')


echo "The number of files are $file_count and the number of matching lines are $match_count"
