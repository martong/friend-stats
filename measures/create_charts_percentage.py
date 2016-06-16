#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt
import argparse


def plot(data):
    xs = []
    ys = []
    for key, value in data.iteritems():
        xs.append(key)
        ys.append(value)

    #plt.plot(xs, ys, 'rs')
    #plt.axis([-1, max(xs)+1, 0, max(ys)+int(max(ys)/10)])
    #plt.show()

    #fig = plt.figure()
    width = .95
    ind = np.arange(len(ys))
    plt.bar(ind, ys, width=width)
    #plt.xticks(ind + width / 2, xs)
    plt.show()


def hist_plot(data):
    #plt.plot(xs, ys, 'rs')
    #plt.axis([-1, max(xs)+1, 0, max(ys)+int(max(ys)/10)])
    #plt.show()

    #fig = plt.figure()
    #width = .75
    #ind = np.arange(len(ys))
    #plt.bar(ind, ys, width=width)
    #plt.xticks(ind + width / 2, xs)
    #plt.show()

    #print data
    hist_data = [np.full((value), key, dtype=int)
                 for key, value in data.iteritems()]
    #hist_data = np.full((3, 5), 7, dtype=int)
    #print hist_data
    #return

    n, bins, patches = plt.hist(hist_data, 5, facecolor='g', alpha=0.95)

    plt.xlabel('Smarts')
    plt.ylabel('Probability')
    plt.title('Histogram of IQ')
    plt.text(60, .025, r'$\mu=100,\ \sigma=15$')
    #plt.axis([40, 160, 0, 0.03])
    plt.grid(True)
    plt.show()

def parse_file(filename):
    with open(filename) as f:
        is_friend_data_line = False
        is_indirect_friend_data_line = False
        friend_data_dict = {n: 0 for n in range(101)}
        indirect_friend_data_dict = {n: 0 for n in range(101)}
        #friend_data_dict = {}
        for line in f:
            if "Friend functions private usage (in percentage) distribution:" in line:
                is_friend_data_line = True
                continue
            if is_friend_data_line:
                ws = line.split()
                try:
                    rangex = ws[0].strip("(").strip(")")
                    rangex_tokens = rangex.split(',')
                    x = int(rangex_tokens[0])
                    y = int(ws[1])
                    if x==0 and int(rangex_tokens[1])==0:
                        continue
                    #friend_data_xs.append(x)
                    #friend_data_ys.append(y)
                    friend_data_dict[x] = y
                    #print x, ' ', y
                except ValueError:
                    is_friend_data_line = False

            if '"Indirect friend" functions private usage (in percentage)' in line:
                is_indirect_friend_data_line = True
                continue
            if is_indirect_friend_data_line:
                ws = line.split()
                try:
                    rangex = ws[0].strip("(").strip(")")
                    rangex_tokens = rangex.split(',')
                    x = int(rangex_tokens[0])
                    y = int(ws[1])
                    if x==0 and int(rangex_tokens[1])==0:
                        continue
                    #friend_data_xs.append(x)
                    #friend_data_ys.append(y)
                    indirect_friend_data_dict[x] = y
                    #print x, ' ', y
                except ValueError:
                    is_indirect_friend_data_line = False

        plot(friend_data_dict)
        plot(indirect_friend_data_dict)

parser = argparse.ArgumentParser()
parser.add_argument('ps', nargs='*')
args = parser.parse_args()
for arg in args.ps:
    print arg
    parse_file(arg)
