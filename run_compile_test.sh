#!/bin/bash
./configure_and_compile.sh

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

failed_tests=()
disabled_tests=()
declare -i id=0
declare -i totnum=${#file_list[@]}

calc() { awk "BEGIN{ printf \"%.2f\n\", $* }"; }

for file in ${file_list[@]}
do
    filename=$(basename "${file}")
    if test -f $file
    then
        if [ "${filename##*.}" = "sy" ];
        then 
            if [[ "$file" =~ .*_DISABLED_.* ]]
            then
                echo "Disabled Test: ${filename}, ignored." 
                disabled_tests+=("DisabledTest:${filename}")
                continue
            fi

            id=$id+1
            cp $file ./build/source/test.sy
            prog=$(calc 100*$id/$totnum)
            echo "[$prog%] Compiling ${filename}..."
            ./build/source/hfb -o ./build/source/out.s ./build/source/test.sy -O2
            retcode=$?
            if [ $retcode == 0 ]
            then 
                echo "Compiling ${filename} ended succeessfully!"
            else
                echo "Compiling ${filename} failed. Exit Code: ${retcode}"
                failed_tests+=("Failure:${file},ExitCode:${retcode}")
            fi
        fi
    fi    
done

echo "Summary"
id=0

if [ ${#failed_tests[@]} -eq 0 ]
then
    echo "No failure test case."
else
    echo "Failure test cases(${#failed_tests[@]}):"
    for failure in ${failed_tests[@]}
    do
        id=$id+1
        echo "[$id] $failure"
    done
fi


id=0
if [ ${#disabled_tests[@]} -eq 0 ]
then
    echo "No disabled test case."
else
    echo "Disabled test cases(${#disabled_tests[@]}):"
    for disable in ${disabled_tests[@]}
    do
        id=$id+1
        echo "[$id] $disable"
    done
fi


# fullfilename=$1
# filename=$(basename "$fullfilename")
# fname="${filename%.*}"
# ext="${filename##*.}"

# echo "Input File: $fullfilename"
# echo "Filename without Path: $filename"
# echo "Filename without Extension: $fname"
# echo "File Extension without Name: $ext"