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
if ! [[ $3 =~ ^[1-9][0-9]*$ ]]; then		# regex for positive integer
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
w1=$(($3 - 1))		# for 0-based indexing later on
p1=$(($4 - 1))

# Purging root_directory, if full:
if test "$(ls -A "$1")"; then
    echo "Warning: directory is full, purging ..."
	ls -dA1 $PWD/$1/* | xargs rm -r 		# also deletes hidden files
fi

# Create directories and web sites names:
p9000=$(($4 + 9000))
sitelist=""
for i in $(seq 0 "$w1"); do
	di="$1"/site"$i"
	mkdir "$di"
	randnums=($(shuf -i 1000-"$p9000" -n "$4"))		# effectively creating 4-digit random numbers
	for j in $(seq 0 $p1); do
		pg="$1"/site"$i"/page"$j"_"${randnums[$j]}".html
		sitelist+="$pg
"
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

# Create sites' contents and the sites themseleves:
linked=""		# keep sites with incoming links here
for i in $(seq 0 "$w1"); do
	echo "Creating web site $i ..."
	out_links=$(echo "$sitelist" | grep -v "site$i" | shuf -n "$f")
	echo "$out_links"	
	for j in $(seq 0 $p1); do
		site_index=$(($i * $4 + $j + 1))
		site_name=$(sed -n "$site_index {p;q;}" <<< "$sitelist")	# get site_name from sitelist
		in_links=$(echo "$sitelist" | grep "site$i" | grep -v "page$j" | shuf -n "$q")
		echo "$in_links"	
		page_links=$(head -n -1 <<< "$in_links
$out_links
" | shuf)	# shuffle the selected in_links and out_links
		linked+="$page_links
"
		k2=$(($lines_num - 2001))
		k=$(shuf -i 2-$k2 -n 1)
		m=$(shuf -i 1001-1999 -n 1)
		site_text="$html_headers"
		link_index=1
		inc=$(($m / ($f + $q)))
		sd1=$k
		sd2=$(($sd1 + $inc))
		echo "  Creating page $site_name with $m lines starting at line $k of \"$2\" ..."
		steps=$(($f + $q))
		for st in $(seq 1 "$steps"); do
			# add "m/(f+q)" lines to site_text:
			site_text+=$(sed -n "$sd1,$sd2 p" "$2")
			#sed 's/$/<br>/'	# appending <br> for visibility reasons
			sd1=$(($sd2 + 1))
			((sd2 += $inc))
			line_link=$(sed -n "$link_index {p;q;}" <<< "$page_links")		# add one link after
			# convert links from abolute to relative paths:
			if [[ $line_link =~ /$1/site$site_index/* ]]; then	# page is in same site
				rel_line_link=$(sed "s/$1\/site[0-9]*/./" <<< "$line_link")
			else
				rel_line_link=$(sed "s/$1/../" <<< "$line_link")	
			fi
			site_text+="<a href=\"$rel_line_link\">link${link_index}_text</a><br>
"
			echo "    Adding link to $line_link"
			((link_index ++))
		done
		site_text+="$html_footers"
		echo "$site_text" > "$site_name"	# finally create the file containing the site_text
	done
done
linked=$(head -n -1 <<< "$linked")		# remove empty last line

# If the (uniqely sorted) "linked" contains the same amount of site names as sitelist
# then all the sites have incoming links
sites_num=$(wc -l <<< "$sitelist")
linked_num=$(sort -u <<< "$linked" | wc -l)
if [ $sites_num -eq $linked_num ]; then
	echo "All pages have at least one incoming link"
else
	unlinked_pages_num=$(($sites_num - $linked_num))
	echo "$unlinked_pages_num page(s) have no incoming link"	
fi
echo "Done."

