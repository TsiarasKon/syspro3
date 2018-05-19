#!/bin/bash
# Input checking:
if [ ! -d "$1" ]; then
	echo "root_directory \"$1\" does not exist."
	exit 1
fi
if [ ! -f "$2" ]; then
	echo "text_file \"$2\" does not exist."
	exit 1
fi
if ! [[ $3 =~ ^[1-9][0-9]*$ ]]; then
	echo "w must be a positive integer."
	exit 1
fi
if ! [[ $4 =~ ^[1-9][0-9]*$ ]]; then
	echo "p must be a positive integer."
	exit 1
fi
lines_num=$(wc -l < "$2")
if [ "$lines_num" -lt 10000 ]; then
	echo "text_file must have at least 10000 lines."
	exit 1
fi
w1=$(($3 - 1))
p1=$(($4 - 1))

# Purging root_directory, if full:
if test "$(ls -A "$1")"; then
    echo "Warning: directory is full, purging ..."
	ls -dA1 $PWD/$1/* | xargs rm -r 		# also deletes hidden files
fi

# Create web sites names:
p9000=$(($4 + 9000))
sitelist=""
for i in $(seq 0 "$w1"); do
	di="$1"/site"$i"
	mkdir "$di"
	randnums=($(shuf -i 1000-"$p9000" -n "$4"))
	for j in $(seq 0 $p1); do
		pg="$1"/site"$i"/page"$j"_"${randnums[$j]}".html
		sitelist+="$pg
";
	done
done
sitelist=$(head -n -1 <<< "$sitelist")		# remove empty last line

f=$(($w1 / 2 + 1))
q=$(($p1 / 2 + 1))

html_headers="<!DOCTYPE html>
<html>
	<body>
"
html_footers="	</body>
</html>"

linked=""
for i in $(seq 0 "$w1"); do
	out_links=$(echo "$sitelist" | grep -v "site$i" | shuf -n "$f")
	for j in $(seq 0 $p1); do
		site_index=$(($i * $4 + $j + 1))
		site_name=$(sed -n "$site_index {p;q;}" <<< "$sitelist")
		in_links=$(echo "$sitelist" | grep "site$i" | grep -v "page$j" | shuf -n "$q")
		page_links=$(head -n -1 <<< "$in_links
$out_links
" | shuf)
		linked+="$page_links
"
		k2=$(($lines_num - 1))
		k=$(shuf -i 2-$k2 -n 1)
		m=$(shuf -i 1001-1999 -n 1)
		site_text="$html_headers"
		link_index=1
		inc=$(($m / ($f + $q)))
		sd1=$k
		sd2=$(($sd1 + $inc))
		steps=$(($f + $q))
		for st in $(seq 1 "$steps"); do
			site_text+=$(sed -n "$sd1","$sd2"p "$2")
			sd1=$(($sd2 + 1))
			((sd2 += $inc))
			line_link=$(sed -n "$link_index {p;q;}" <<< "$page_links")
			site_text+="<a href=\"$line_link\">link${link_index}_text</a>
"
			((link_index ++))
		done
		site_text+="$html_footers"
		echo "$site_text" > "$site_name"
	done
done
linked=$(head -n -1 <<< "$linked")		# remove empty last line


sites_num=$(wc -l <<< "$sitelist")
linked_num=$(sort -u <<< "$linked" | wc -l)
if [ $sites_num -eq $linked_num ]; then
	echo "All pages have at least one incoming link"
else
	unlinked_pages_num=$(($sites_num - $linked_num))
	echo "$unlinked_pages_num page(s) have no incoming link"	
fi




