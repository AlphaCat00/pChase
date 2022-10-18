import csv
import sys

assert len(sys.argv) == 3
headers = sys.argv[2].split(',')
data = [[] for _ in headers]
with open(sys.argv[1]) as f:
    for col in zip(*csv.reader(f)):
        try:
            col = list(col)
            data[headers.index(col[0])] = col
        except ValueError:
            continue
csv.writer(sys.stdout).writerows(zip(*data))
