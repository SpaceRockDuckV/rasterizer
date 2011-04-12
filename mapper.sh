#!/bin/sh

IN_FILE=$1
TEMP_DIR=./tmp
OUT_FILE=$TEMP_DIR/output
SUFFIXES=(.shp .shx)
PROGRAM=./quadtree 

echo $@ 1>&2 
echo $IN_FILE 1>&2 

mkdir $TEMP_DIR
for ext in "${SUFFIXES[@]}"
do
    filename=$IN_FILE$ext
    hadoop fs -get $filename $TEMP_DIR/
done

$PROGRAM $TEMP_DIR/${IN_FILE##*/}.shx $OUT_FILE

cat $OUT_FILE.txt

rm -rf $TEMP_DIR
