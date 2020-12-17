#!/bin/bash
make
rm ./log/*

th=$1

if [[ $th == 1 ]];
then
  for((i=1;i<=5;i++))
  do
    ./main $i &
  done
  for((i=6;i<=7;i++))
  do
    ./main $i B &
  done
fi

if [[ $th == 2 ]];
then
  echo "TH 2"
  for((i=1;i<=2;i++))
  do
    ./main $i &
  done
  for((i=3;i<=7;i++))
  do
    ./main $i B &
  done
fi