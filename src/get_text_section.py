#!/usr/bin/python
import sys

infile = sys.argv[1] 
f = open(infile)

line = f.readline()
start = False
count = 0
while line != '':
    arr = line.split();

    if len(arr) < 7:
        line = f.readline()
        continue

    if arr[2] == '.text' or arr[2] == '.rel.text':
       print arr[4], arr[5], arr[6]
            
    line = f.readline()
