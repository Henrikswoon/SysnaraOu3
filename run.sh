#!/bin/bash
clear; make clean && ls mdu; make;
echo --- Output:;
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=log.valgrind  ./mdu Dir_1;