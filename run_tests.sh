#!/bin/bash

OPT="opt"

PASS_LIB="./build/libMyDCE.so"

PASS_NAME="my-dce"

TEST_DIR="./tests"

echo "---- TEST BEGINNING ----"

find "$TEST_DIR" -type f -name "*.ll" | while read -r file; do

    echo "---- running test : $file -----"

    $OPT -load-pass-plugin="$PASS_LIB" -passes="$PASS_NAME,verify" -S < "$file" | FileCheck "$file"

    if [ $? -eq 0 ]; then
        echo "**** PASS ****"
    else
        echo "**** FAIL ****"
    fi

    echo ""

done

echo "---- TEST ENDED ------"
