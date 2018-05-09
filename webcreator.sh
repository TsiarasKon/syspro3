#!/bin/bash
if [ ! -d "$1" ]; then
	echo root_directory \""$1"\" does not exist.
	exit 1
fi
if [ ! -f "$2" ]; then
	echo text_file \""$2"\" does not exist.
	exit 1
fi
if ! [[ $3 =~ ^[1-9][0-9]*$ ]]; then
	echo w must be a positive integer.
	exit 1
fi
if ! [[ $4 =~ ^[1-9][0-9]*$ ]]; then
	echo p must be a positive integer.
	exit 1
fi
lines_num=$(wc -l < "$2")
if [ "$lines_num" -lt 10000 ]; then
	echo text_file must have at least 10000 lines
	exit 1
fi
