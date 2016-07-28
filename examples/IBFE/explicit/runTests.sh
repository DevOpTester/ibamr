#!/bin/bash
#set -e 
#set -o pipefail

for dir in ./*/
do
    dir=${dir%*/}
    cd ${dir##*/}
    if [ -f "test_main.cpp" ];  
    then
        make examples
    fi
    if [ -f "test2d" ];
    then
        echo "************Running "test2d"************"
        ./test2d input2d --gtest_filter=*.2d
    fi
    if [ -f "test3d" ];
    then
        echo "************Running "test3d"************"
        ./test2d input3d --gtest_filter=*.3d
    fi
    cd ..
done
