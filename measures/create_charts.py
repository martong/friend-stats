#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import argparse
import charts_common as cc
import re


def plot(xs, ys, filename, kind, meyers_candidates=0):
    kindstr = ""
    if kind == "func":
        kindstr = "functions"
    else:
        kindstr = "classes"
    width = .75
    ind = np.arange(len(ys))
    plt.bar(ind, ys, width=width)
    plt.xticks(ind + width / 2, xs)

    mc_ys = [0 for i in range(len(ys))]
    mc_ys[0] = meyers_candidates
    print ys
    print mc_ys
    plt.bar(ind + .05, mc_ys, color='r', width=width, hatch='///')

    lib = cc.getLib(filename)
    libstr = "%s, version:%s" % (cc.getName(lib), cc.getVersion(lib))
    #plt.title('Privates in friend %s\n%s' % (kindstr, libstr))
    plt.xlabel('private usage')
    plt.ylabel('No. friendly function instances')

    #plt.show()
    plt.savefig('%s.%s.png' % (filename, kind))
    plt.clf()


def parse_file(filename):
    with open(filename) as f:
        is_friend_data_line = False
        is_indirect_friend_data_line = False
        friend_data_xs = []
        friend_data_ys = []
        indirect_frind_data_xs = []
        indirect_frind_data_ys = []
        meyers_candidates = 0
        for line in f:
            if "Friend functions private usage (by piece) distribution" in line:
                is_friend_data_line = True
                continue
            if is_friend_data_line:
                ws = line.split()
                try:
                    x = int(ws[0])
                    y = int(ws[1])
                    friend_data_xs.append(x)
                    friend_data_ys.append(y)
                    print x, ' ', y
                except ValueError:
                    is_friend_data_line = False

            if '"Indirect friend" functions private usage ' in line:
                is_indirect_friend_data_line = True
                continue
            if is_indirect_friend_data_line:
                ws = line.split()
                try:
                    x = int(ws[0])
                    y = int(ws[1])
                    indirect_frind_data_xs.append(x)
                    indirect_frind_data_ys.append(y)
                    print x, ' ', y
                except ValueError:
                    is_indirect_friend_data_line = False

            if 'Meyers' in line:
                meyers_candidates = int(re.findall('\d+', line)[0])
                print "mc: ", meyers_candidates


        plot(friend_data_xs, friend_data_ys, filename, "func",
             meyers_candidates)
        plot(indirect_frind_data_xs, indirect_frind_data_ys, filename, "class")

parser = argparse.ArgumentParser()
parser.add_argument('ps', nargs='*')
args = parser.parse_args()
for arg in args.ps:
    print arg
    parse_file(arg)
