#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt
import argparse


def plot_func(xs, ys, filename):
    fig = plt.figure()
    width = .75
    ind = np.arange(len(ys))
    plt.bar(ind, ys, width=width)
    plt.xticks(ind + width / 2, xs)

    plt.title('The usage of privates in friend functions\n' + filename)
    plt.xlabel('No. used private entities')
    plt.ylabel('No. friend function instances')

    plt.show()

def plot_class(xs, ys, filename):
    fig = plt.figure()
    width = .75
    ind = np.arange(len(ys))
    plt.bar(ind, ys, width=width)
    plt.xticks(ind + width / 2, xs)

    plt.title('The usage of privates in friend classes')
    plt.xlabel('No. used private entities')
    plt.ylabel('No. friend class function instances')

    plt.show()

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
