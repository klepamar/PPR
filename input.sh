#!/bin/bash

# check all files in the form of vstup_<no>.txt
# result for every input is expected to be found in vstup_<no>.result

for file in $(ls vstup_[0-9]*.txt)
do
	resultFile=$(echo $file | sed 's/\(.*\).txt/\1.result/')
	if [[ -f $resultFile ]]
	then
		cir=$(cat $resultFile)
		echo "Processing $file, expected circumference = $cir"
		mv $file "vstup.txt"
		./ppr > output.log
		actualCir=$(grep 'perimeter sum:' output.log | tail -1)
		actualCir=$(echo $actualCir | cut -d ':' -f2)
		if [[ $actualCir -eq $cir ]]
		then
			echo "...OK"
		else
			echo "...FAIL, actual circumference = $actualCir"
		fi
		mv "vstup.txt" $file 
	else
		echo "No result found for $file!"
	fi
	
done

rm output.log
