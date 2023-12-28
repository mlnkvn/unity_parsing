#!/bin/bash

mkdir /app/build
cd /app/build

cmake ..
make

./tool.exe /app/TestCase01 /app/TestCase01Output

for file in /app/TestCase01Output/*; do
    echo "$file content"
    echo "==========================="
    cat $file
    echo 
done

