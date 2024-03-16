#!/bin/bash
# Script to generate defines for resource locations in cartridge image

# Parameters
MAKEFILE=Makefile.res
MAPFILE=res.map
OUTFILE=$1
PLATFORM_DEFINE=$2

# Color output
RED='\033[0;31m'
AMBER='\033[0;33m'
GREEN='\033[0;32m'
NC='\033[0m'

rev_builtin() {
	x=$1
	for ((i=${#x}; i>0; i--)); do echo -n ${x:$i:1}; done
}

shorten_name() {
	INPUT=$1
	NAME_SHORT=($(echo $INPUT | rev | cut -f 2- -d '_' | rev))
	NAME_SHORT=($(echo $NAME_SHORT | rev | cut -f 2- -d '_' | rev))
	NAME_SHORT=($(echo $NAME_SHORT | cut -f 4- -d '_'))
}

# First, generate map file via linker
make -f $MAKEFILE $PLATFORM_DEFINE

# Check if resource defs need to be rebuilt
MD5=("$(md5 $MAPFILE)")
MD5_OLD=("$(cat $MAPFILE.md5 2>/dev/null)")

if [ "$MD5" == "$MD5_OLD" ]; then
	if [ -f $OUTFILE ]; then
		# Resources unchanged, no need to rebuild
		exit;
	fi
fi

# Save checksum for later
echo $MD5 > $MAPFILE.md5

# Create arrays
LOCATIONS=($(cat $MAPFILE | grep _binary_resources | awk -F_binary '{print $(NF-1)}' | awk -F0x '{print $(NF)}' ))
NAMES=($(cat $MAPFILE | grep _binary_resources | awk -F' ' '{print $(NF)}'))

# Output file header
echo "#ifndef RESOURCE_DEFS_H" > $OUTFILE
echo "#define RESOURCE_DEFS_H" >> $OUTFILE
echo "" >> $OUTFILE

# Create arrays
LOCATIONS_START=($(cat $MAPFILE | grep _binary_resources | grep _dat_start | cut -b 27-34))
LOCATIONS_END=($(cat $MAPFILE | grep _binary_resources | grep _dat_end | cut -b 27-34))
NAMES_START=($(cat $MAPFILE | grep _binary_resources | grep _dat_start | cut -b 51-))

# Output defines to file
echo "" >> $OUTFILE
for i in "${!NAMES_START[@]}"; do
	NAME_SHORT=($(echo ${NAMES_START[i]} | awk -F_binary_resources_ '{print $(NF)}' | awk -F_dat_start '{print $(NF-1)}'))
	BANKNUMBER=$(( 0x${LOCATIONS_START[i]} / 0x2000 ))
	OFFSET=$(( 0x${LOCATIONS_START[i]} - ($BANKNUMBER * 0x2000) ))
	LENGTH=$(( 0x${LOCATIONS_END[i]} - 0x${LOCATIONS_START[i]} ))
	echo -e "#define " $NAME_SHORT"_bank " $BANKNUMBER  >> $OUTFILE
	echo -e "#define " $NAME_SHORT"_offset " $OFFSET >> $OUTFILE
	echo -e "#define " $NAME_SHORT"_length " $LENGTH  >> $OUTFILE
	REF_COUNT=($(grep -R $NAME_SHORT * | grep .c: | wc -l))

	if [ "$REF_COUNT" -eq "0" ]; then
		echo -e "\t\t${AMBER}WARNING${NC}: " $NAME_SHORT".dat has zero references!"
	fi
done

echo "" >> $OUTFILE
echo "#endif" >> $OUTFILE

