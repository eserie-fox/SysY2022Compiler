#!/bin/bash

file_list=()

read_dir(){
    for file in `ls -a $1`
    do
        if [ -d $1"/"$file ]
        then
            if [[ $file != '.' && $file != '..' ]]
            then
                read_dir $1"/"$file
            fi
        else
            # echo $1"/"$file
            file_list+=($1"/"$file)
        fi
    done
}

read_dir ./tests/compile_test_cases


for file in ${file_list[@]}
do
    filename=$(basename "${file}")
    if test -f $file
    then
        if [ "${filename##*.}" != "sy" ];
        then 
            # echo $filename
            rm $file
        fi
    fi    
done


# fullfilename=$1
# filename=$(basename "$fullfilename")
# fname="${filename%.*}"
# ext="${filename##*.}"

# echo "Input File: $fullfilename"
# echo "Filename without Path: $filename"
# echo "Filename without Extension: $fname"
# echo "File Extension without Name: $ext"