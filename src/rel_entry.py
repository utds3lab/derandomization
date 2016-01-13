#!/usr/bin/python
import sys

infile = sys.argv[1] 
outfile = sys.argv[2]
f = open(infile)
fw = open(outfile,'w')

line = f.readline()
start = False
count = 0
while line != '':
    if(start == False and line.startswith("Relocation section")):
        start = True

    if start:
        if line.startswith('\n'):
            break

        count += 1
        if(count > 2):
            strings = line.split()
            outline = strings[0] + ' ' + strings[3] + '\n';
            fw.write(outline)

            
    line = f.readline()
