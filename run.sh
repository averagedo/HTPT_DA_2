#!/bin/bash
make
rm ./log/*
for((i=1;i<=7;i++))
do
  ./main $i &
done

