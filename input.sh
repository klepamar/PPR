#!/bin/bash

# check all files in the form of vstup_<no>.txt
# result for every input is expected to be found in vstup_<no>.result

for file in $(ls vstup_*.txt)
do
	resultFile=$(echo $file | sed 's/\(.*\).txt/\1.result/')
	if [[ -f $resultFile ]]
	then
		cir=$(cat $resultFile)
		echo "Processing $file, expected circumference = $cir"
		mpirun -np 2 ./ppr -f $file > output.log
		actualCir=$(grep 'perimeter sum:' output.log | tail -1)
		actualCir=$(echo $actualCir | cut -d ':' -f2 | awk '{print $1}')
		if [[ $actualCir -eq $cir ]]
		then
			echo "...OK"
		else
			echo "...FAIL, actual circumference = $actualCir"
		fi
	else
		echo "No result found for $file!"
	fi
	
done

rm output.log
