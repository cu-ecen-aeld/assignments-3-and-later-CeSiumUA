#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

if [ $# -lt 2 ]; then
    echo "Not enough arguments supplied"
    exit 1
fi

FILESDIR=$1
SEARCH_TEXT=$2

if [ ! -d $FILESDIR ]; then
    echo "Directory $FILESDIR does not exists"
    exit 1
fi

MATCHING_FILES_CNT=$(find "$FILESDIR" -type f | wc -l)
MATCHING_ENTRIES_CNT=$(grep -r "$SEARCH_TEXT" "$FILESDIR" | wc -l)

echo "The number of files are $MATCHING_FILES_CNT and the number of matching lines are $MATCHING_ENTRIES_CNT"