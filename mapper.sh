#!/bin/sh
read IN
IN_FILE=${IN#* }

TEMP_DIR=./tmp
OUT_FILE=$TEMP_DIR/output
SUFFIXES=(.shp .shx)
PROGRAM=./quadtree 

mkdir $TEMP_DIR
for ext in "${SUFFIXES[@]}"
do
    filename=$IN_FILE$ext
    echo hadoop fs -get $filename $TEMP_DIR/ 1>&2
    hadoop fs -get $filename $TEMP_DIR/
done

echo $PROGRAM $TEMP_DIR/${IN_FILE##*/}.shx $OUT_FILE 1>&2
$PROGRAM $TEMP_DIR/${IN_FILE##*/}.shx $OUT_FILE

cat $OUT_FILE.txt

rm -rf $TEMP_DIR
