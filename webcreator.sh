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
p9000=$(($4 + 9000))
sitelist=""
for i in $(seq 0 "$w1"); do
	di="$1"/site"$i";
	echo "$di";
	randnums=($(shuf -i 1000-"$p9000" -n "$4"))
	for j in $(seq 0 $p1); do
		pg="$1"/site"$i"/page"$j"_"${randnums[$j]}".html;
		echo " $pg";
		sitelist+="$pg
";
	done
done
sitelist=$(head -n -1 <<< "$sitelist")		# remove empty last line

f=$w1
q=$p1

for i in $(seq 0 "$w1"); do
	out_links=$(echo "$sitelist" | grep -v "site$i" | shuf -n "$f")
	for j in $(seq 0 $p1); do
		in_links=$(echo "$sitelist" | grep "site$i" | grep -v "page$j" | shuf -n "$q")
		#page_links=$("$out_links$in_links")
		#echo "$page_links"
	done
done
