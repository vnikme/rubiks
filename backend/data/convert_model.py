#!/usr/bin/python
# coding: utf-8


import sys, base64, json


def main(path):
    js = open(path, "rt").read()
    js = json.loads(js)
    js = json.dumps(js)
    print 'var model = JSON.parse(window.atob("%s"));' % base64.b64encode(js)

if __name__ == "__main__":
    main(sys.argv[1])

