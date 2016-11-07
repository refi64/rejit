#!/usr/bin/env python3
from xml.parsers import expat
import sys

chars = []
i = 0

def start_element(kind, attr):
    global i
    i += 1
    print('Parsed', i, 'entires...', end='\r')
    sys.stdout.flush()
    if kind == 'char' and attr['gc'] == 'Nd':
        chars.append(int(attr['cp'], base=16))

parser = expat.ParserCreate()
parser.StartElementHandler = start_element
with open('ucd.all.flat.xml', 'rb') as f:
    parser.ParseFile(f)
    print()

ranges = []
prev = None

print('Grouping characters...')
for cp in chars:
    if prev is not None and prev + 1 == cp:
        if ranges and ranges[-1][1] == prev:
            ranges[-1][1] = cp
        else:
            ranges.append([prev, cp])
    prev = cp

print('Printing number table...')

with open('number_table', 'w') as f:
    for start, end in ranges:
        assert start+1 != end
        print('%#x, %#x,' % (start, end), file=f)

print('Done!')
