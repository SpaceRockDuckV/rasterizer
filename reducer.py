#!/usr/bin/env python
import sys

def main():
    res = {}
    for line in sys.stdin:
        line = line.strip('\n').split(' ')
        k = line[0]

        if not res.has_key(k):
            res[k] = set()

        for val in line[1:]:
            res[k].add(val)

    for k,v in res.items():
        print "%s\t%s" % (k, ','.join(v))


if __name__ == "__main__":
    main()
