#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import argparse
import charts_common as cc


def plot(data, filename, kind):

    kindstr = ""
    if kind == "func":
        kindstr = "functions"
    else:
        kindstr = "classes"

    xs = []
    ys = []
    for key, value in data.iteritems():
        xs.append(key)
        ys.append(value)

    width = .95
    ind = np.arange(len(ys))
    plt.bar(ind, ys, width=width)
    # plt.xticks(ind + width / 2, xs)

    lib = cc.getLib(filename)
    libstr = "%s, version:%s" % (cc.getName(lib), cc.getVersion(lib))
    #plt.title('Privates usage ratio in friend %s\n%s' % (kindstr, libstr))
    plt.xlabel('private usage ratio (%)')
    plt.ylabel('No. friendly function instances')
    plt.xlim(xmax=102)

    #plt.show()
    plt.savefig('%s.ratio.%s.png' % (filename, kind))
    plt.clf()


def parse_file(filename):
    with open(filename) as f:
        is_friend_data_line = False
        is_indirect_friend_data_line = False
        friend_data_dict = {n: 0 for n in range(101)}
        indirect_friend_data_dict = {n: 0 for n in range(101)}
        for line in f:
            if ("Friend functions private usage (in percentage) distribution:"
                    in line):
                is_friend_data_line = True
                continue
            if is_friend_data_line:
                ws = line.split()
                try:
                    rangex = ws[0].strip("(").strip(")")
                    rangex_tokens = rangex.split(',')
                    x = int(rangex_tokens[0])
                    y = int(ws[1])
                    if x == 0 and int(rangex_tokens[1]) == 0:
                        continue
                    friend_data_dict[x] = y
                except ValueError:
                    is_friend_data_line = False

            if ('"Indirect friend" functions private usage (in percentage)'
                    in line):
                is_indirect_friend_data_line = True
                continue
            if is_indirect_friend_data_line:
                ws = line.split()
                try:
                    rangex = ws[0].strip("(").strip(")")
                    rangex_tokens = rangex.split(',')
                    x = int(rangex_tokens[0])
                    y = int(ws[1])
                    if x == 0 and int(rangex_tokens[1]) == 0:
                        continue
                    indirect_friend_data_dict[x] = y
                except ValueError:
                    is_indirect_friend_data_line = False

        plot(friend_data_dict, filename, "func")
        plot(indirect_friend_data_dict, filename, "class")

parser = argparse.ArgumentParser()
parser.add_argument('ps', nargs='*')
args = parser.parse_args()
for arg in args.ps:
    print arg
    parse_file(arg)
