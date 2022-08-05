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

for file in ${file_list[@]}
do
    filename=$(basename "${file}")
    if test -f $file
    then
        if [ "${filename##*.}" = "sy" ];
        then 
            cp $file ./build/source/test.sy
            echo "Compiling ${filename}..."
            ./build/source/hfb -o ./build/source/out.s ./build/source/test.sy
            retcode=$?
            if [ $retcode == 0 ]
            then 
                echo "Compiling ${filename} ended succeessfully!"
            else
                echo "Compiling ${filename} failed. Exit Code: ${retcode}"
                failed_tests+=("Failure:${filename},ExitCode:${retcode}")
            fi
        fi
    fi    
done

echo "Failure test cases:"

for failure in ${failed_tests[@]}
do
    echo $failure
done
# for line in $(ls -a ./tests/compile_test_cases/)
# do
#     echo ${line}
# done

# for file in $(ls -r ./tests/compile_test_cases/)
# do
#     filename=$(basename "${file}")
#     if test -f $file
#     then
#         if [ "${filename##*.}" = "sy" ];
#         then 
#             cp $file ./build/source/test.sy
#             echo "Compiling ${filename}..."
#             ./build/source/hfb -o ./build/source/out.s ./build/source/test.sy
#             retcode=$?
#             if [ retcode == 0 ]
#             then 
#                 echo "Compiling ${filename} ended succeessfully!"
#             else
#                 echo "Compiling ${filename} failed. Exit Code:${retcode}"
#             fi
#         fi
#     fi
# done

# fullfilename=$1
# filename=$(basename "$fullfilename")
# fname="${filename%.*}"
# ext="${filename##*.}"

# echo "Input File: $fullfilename"
# echo "Filename without Path: $filename"
# echo "Filename without Extension: $fname"
# echo "File Extension without Name: $ext"