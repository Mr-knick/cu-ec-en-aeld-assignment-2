#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Invalid input:"
    echo "Usage: ./writer.sh <writefile> <writestr>"
    exit 1
fi

writefile="$1"
writestr="$2"

filename=$(basename "$writefile")
path_only=$(dirname "$writefile")

echo "Creating or resetting $filename."
echo "In location $path_only"
echo "Writing $writestr"

mkdir -p $path_only
echo "$writestr" > "$writefile"
