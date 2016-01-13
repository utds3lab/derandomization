#!/usr/bin/python
import sys

infile = sys.argv[1] 
f = open(infile)

line = f.readline()
section = 0

def esp_size_change(oprand):
    if not 'esp' in oprand:
        return False
    arr = oprand.split(',')
    if len(arr) >= 2 and arr[1] == '%esp':
        return True 
    return False 
    

while line != '':
    if line.startswith('Disassembly of section'):
        section += 1
        
    #only get data from the first section, which is .text section
    if(section > 1):
        break

    arr = line.split('\t')
    if len(arr) >= 3:
        assem = arr[2].split()
        opcode = assem[0]
        oprand = ''
        if len(assem) >= 2:
            oprand = assem[1]

        #if len(assem) == 1 or opcode == 'push' or opcode == 'pop':
        if opcode == 'push' or opcode == 'pop' or opcode == 'leave' or opcode == 'enter' or esp_size_change(oprand):
            #print assem
            print arr[0][0:8], len(arr[1].split())
            
    line = f.readline()

