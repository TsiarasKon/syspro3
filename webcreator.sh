#!/bin/bash
# Input checking:
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
w1=$(($3 - 1))
p1=$(($4 - 1))

# Purging root_directory, if full:
if test "$(ls -A "$1")"; then
    echo Warning: directory is full, purging ...
	ls -dA1 $PWD/$1/* | xargs rm -r 		# also deletes hidden files
fi

# Create web sites names:
sitelist=()
for i in $(seq 0 "$w1"); do
	di="$1"/site"$i";
	echo "$di";
	randnums=($(shuf -i 100-9999 -n "$4"))
	insitelist=""
	for j in $(seq 0 $p1); do
		pg="$1"/site"$i"/page"$j"_"${randnums[$j]}".html;
		echo " $pg";
		insitelist+="$pg ";
	done
	sitelist+=("$insitelist");
done

echo ${sitelist[@]}
