#!/bin/sh

BINARY=$1
MAPFILE=link.map

# Color output
RED='\033[0;31m'
AMBER='\033[0;33m'
GREEN='\033[0;32m'
NC='\033[0m'

cat $MAPFILE | grep "\.persistent" > mem_summary.txt
cat $MAPFILE | grep "\.scratchpad" >> mem_summary.txt
cat $MAPFILE | grep "\.bank[0-9]"  >> mem_summary.txt
cat $MAPFILE | grep "\.data" | grep "load address" >> mem_summary.txt
cat $MAPFILE | grep "\.bss" | grep "load address" >> mem_summary.txt

# Initialize variables
BANK0_TOTAL=0 		# bank0 also contains persistent and scratchpad data, so we add these up
SECTION_OVERLAP=0   # If sections overlap, we will remove the resulting binary

echo
echo "memory usage summary"
echo "------------------------------------------------"

cat mem_summary.txt | while IFS= read -r line
do
	NAME=`echo $line | cut -d ' ' -f 1 | cut -d '.' -f 2`
	SIZE=`echo $line | cut -d ' ' -f 3 | cut -d 'x' -f 2`

	DECIMAL=$((16#$SIZE))

	if [[ $NAME == "bss" ]]; then
		if [ $DECIMAL -gt 24576 ]; then
			printf "${RED}%5d${NC} bytes in %s\n" $DECIMAL $NAME
			SECTION_OVERLAP=1
		else
			if [ $DECIMAL -gt 22528 ]; then
				printf "${AMBER}%5d${NC} bytes in %s\n" $DECIMAL $NAME
			else
				printf "${GREEN}%5d${NC} bytes in %s\n" $DECIMAL $NAME
			fi
		fi
	else
		if [[ $NAME == "persistent" ]]; then
			BANK0_TOTAL=$(($BANK0_TOTAL+$DECIMAL))
		else
			if [[ $NAME == "scratchpad" ]]; then
				BANK0_TOTAL=$(($BANK0_TOTAL+$DECIMAL))
			else
				if [[ $NAME == "bank0" ]]; then
					BANK0_TOTAL=$(($BANK0_TOTAL+$DECIMAL))
					if [ $BANK0_TOTAL -gt 8192 ]; then
						printf "${RED}%5d${NC} bytes in %s\n" $BANK0_TOTAL "(bank0 + scratchpad + persistent)"
						SECTION_OVERLAP=1
					else
						if [ $BANK0_TOTAL -gt 7680 ]; then
							printf "${AMBER}%5d${NC} bytes in %s\n" $BANK0_TOTAL "(bank0 + scratchpad + persistent)"
						else
							printf "${GREEN}%5d${NC} bytes in %s\n" $BANK0_TOTAL "(bank0 + scratchpad + persistent)"
						fi
					fi
				else
					if [ $DECIMAL -gt 8192 ]; then
						printf "${RED}%5d${NC} bytes in %s\n" $DECIMAL $NAME
						SECTION_OVERLAP=1
					else
						if [ $DECIMAL -gt 7680 ]; then
							printf "${AMBER}%5d${NC} bytes in %s\n" $DECIMAL $NAME
						else
							printf "${GREEN}%5d${NC} bytes in %s\n" $DECIMAL $NAME
						fi
					fi		
				fi
			fi
		fi
	fi
done

if [ $SECTION_OVERLAP -gt 0 ]; then
	echo ""
	echo "${RED}ERROR${NC}: resulting binary contains overlapping sections!"
	echo "Ensure banks are less than 8192 bytes and bss is less than 24576 bytes."
	echo "Removing" $BINARY
	rm $BINARY
fi

rm mem_summary.txt
