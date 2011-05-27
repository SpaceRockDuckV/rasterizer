#!/bin/sh
read IN
IN_FILE=/${IN#*/}

TEMP_DIR=./tmp
SUFFIXES=(.shp .shx)
PROGRAM=./quadtree/quadtree 

mkdir $TEMP_DIR
for ext in "${SUFFIXES[@]}"
do
    filename=$IN_FILE$ext
    hadoop fs -get $filename $TEMP_DIR/
done

$PROGRAM $TEMP_DIR/${IN_FILE##*/}.shx

rm -rf $TEMP_DIR
