#!/bin/bash
clear; make clean && ls mdu; make;
echo --- Output:;
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=log.valgrind  ./mdu -j 3 Dir_1 Dir_2;
valgrind --tool=helgrind -s --log-file=hell.valgrind ./mdu -j 2 Dir_1 Dir_2