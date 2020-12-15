#!/bin/bash
make

for((i=1;i<=2;i++))
do
  ./main $i &
done