#!/bin/sh

MAPFILE=link.map

cat $MAPFILE | grep "\.persistent" > mem_summary.txt
cat $MAPFILE | grep "\.scratchpad" >> mem_summary.txt
cat $MAPFILE | grep "\.bank[0-9]"  >> mem_summary.txt
cat $MAPFILE | grep "\.data" | grep "load address" >> mem_summary.txt
cat $MAPFILE | grep "\.bss" | grep "load address" >> mem_summary.txt

echo
echo "memory usage summary"
echo "----------------------------------"

cat mem_summary.txt | while IFS= read -r line
do
	NAME=`echo $line | cut -d ' ' -f 1 | cut -d '.' -f 2`
	SIZE=`echo $line | cut -d ' ' -f 3 | cut -d 'x' -f 2`

	DECIMAL=$((16#$SIZE))
	# printf -v PRINTABLE "%5d" $DECIMAL
	printf "%5d bytes in %s\n" $DECIMAL $NAME
	# echo $PRINTABLE bytes in $NAME
done

rm mem_summary.txt
echo