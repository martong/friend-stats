#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt
import argparse
import re


def getLib(path):
    m = re.search('/\w*__.*?/', path)
    return m.group(0).strip('/')

def getName(lib):
    return lib.split('__')[0]

def getVersion(lib):
    return lib.split('__')[1]

def plot_func(xs, ys, filename):
    fig = plt.figure()
    width = .75
    ind = np.arange(len(ys))
    plt.bar(ind, ys, width=width)
    plt.xticks(ind + width / 2, xs)

    lib = getLib(filename)
    libstr = "%s, version:%s" % (getName(lib), getVersion(lib))
    plt.title('Privates in friend functions\n' + libstr)
    #plt.title('The usage of privates in friend functions')
    #plt.title('\n'+libstr, loc='right');
    plt.xlabel('No. used private entities')
    plt.ylabel('No. friend function instances')

    #plt.show()
    plt.savefig(filename+'.func.png')

def plot_class(xs, ys, filename):
    fig = plt.figure()
    width = .75
    ind = np.arange(len(ys))
    plt.bar(ind, ys, width=width)
    plt.xticks(ind + width / 2, xs)

    lib = getLib(filename)
    libstr = "%s, version:%s" % (getName(lib), getVersion(lib))
    plt.title('Privates in friend classes\n' + libstr)
    plt.xlabel('No. used private entities')
    plt.ylabel('No. friend class function instances')

    #plt.show()
    plt.savefig(filename+'.class.png')

def parse_file(filename):
    with open(filename) as f:
        is_friend_data_line = False
        is_indirect_friend_data_line = False
        friend_data_xs = []
        friend_data_ys = []
        indirect_frind_data_xs = []
        indirect_frind_data_ys = []
        for line in f:
            if "Friend functions private usage (by piece) distribution:" in line:
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

        plot_func(friend_data_xs, friend_data_ys, filename)
        plot_class(indirect_frind_data_xs, indirect_frind_data_ys, filename)

parser = argparse.ArgumentParser()
parser.add_argument('ps', nargs='*')
args = parser.parse_args()
for arg in args.ps:
    print arg
    parse_file(arg)
