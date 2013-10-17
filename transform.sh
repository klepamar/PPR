#!/bin/bash

if [[ "$#" -ne 4 ]]
then
	echo "Usage:"
	echo "./transform.sh x y noOfElements inputFile.txt"
	exit 1
fi

dimX=$1
dimY=$2
noOfElements=$3
fileName=$4

! [[ "$dimX" =~ ^[0-9]+$ ]] && echo "$dimX is not a positive integer value" && exit 1 
! [[ "$dimY" =~ ^[0-9]+$ ]] && echo "$dimY is not a positive integer value" && exit 1 
! [[ -f "$fileName" ]] && echo "$fileName is not a valid file" && exit 1
! [[ -r "$fileName" ]] && echo "$fileName is not readable" && exit 1

################################################################
# ! generator_hlav produces x,y dimensions in terms of [1..n] !#
################################################################

declare -A field

# fill every element within the field with zero
for i in $(seq $dimX)
do
	for j in $(seq $dimY)
	do
		field[$i,$j]=0
	done
done

currentX=""
currentY=""
currentValue=""

# process $fileName to change values of respective non-zero elements
while read line
do 
	currentX=$(echo "$line" | awk '{print $1}')
	currentY=$(echo "$line" | awk '{print $2}')
	currentValue=$(echo "$line" | awk '{print $3}')
	# echo "[$currentX,$currentY] = $currentValue"
	field[$currentX,$currentY]=${currentValue}
done < $fileName

# include x,y dimensions + number of non-zero elements
echo "$dimX $dimY"
echo "$noOfElements"

for i in $(seq $dimX)
do
	for j in $(seq $dimY)
	do
		if [[ "$j" -eq "$dimY" ]] 
		then	# do not include additional space
			printf "%s" ${field[$i,$j]}
		else	# add a space after producing the number
			printf "%s " ${field[$i,$j]}
		fi
	done
	echo 
done
