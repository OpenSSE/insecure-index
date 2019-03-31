#!/usr/bin/env python3

# from optparse import OptionParser
import argparse
import math
import re
import json
import matplotlib.pyplot as plt


def parse_file(f):
    stats = dict()
    lines = f.readlines()

    # We suppose that the benchark log has a spdlog format ending with %v
    pattern = r"(\{.*\})$"
    prog = re.compile(pattern)

    for l in lines:
        line = l.strip('\n\t')
        # print(line)
        m = prog.search(line)

        json_data = json.loads(m.group(0))
        # print(json_data)

        # name = json_data['message']
        items = json_data['items']
        time = float(json_data['time'])
        time_per_item = float(json_data['time/item'])

        if items in stats:
            old_sum = stats[items]["sum"]
            old_square_sum = stats[items]["square_sum"]
            counter = stats[items]["counter"]
            min_v = stats[items]["min"]
            max_v = stats[items]["max"]
        else:
            old_sum = 0
            old_square_sum = 0
            counter = 0
            min_v = 1e9
            max_v = 0
            stats[items] = dict()

        stats[items]["sum"] = old_sum+time_per_item
        stats[items]["square_sum"] = old_square_sum + \
            time_per_item*time_per_item
        stats[items]["counter"] = counter+1
        stats[items]["min"] = min(min_v, time_per_item)
        stats[items]["max"] = max(max_v, time_per_item)

    return stats


def compute_mean_var(stats):
    res = {}
    for key in stats:
        res[key] = {}
        res[key]["mean"] = stats[key]["sum"] / \
            stats[key]["counter"]
        res[key]["dev"] = math.sqrt(
            stats[key]["square_sum"]/stats[key]["counter"] - res[key]["mean"]*res[key]["mean"])
        res[key]["min"] = stats[key]["min"]
        res[key]["max"] = stats[key]["max"]

    return res


def plot_stats(stats):
    sorted_list = sorted(stats.keys())
    vals = [stats[k]["mean"] for k in sorted_list]
    plt.plot(sorted_list, vals)
    plt.xscale('log')
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Process a search benchmark file.')

    parser.add_argument("-i", "--input", dest="in_file", nargs=1,
                        required=True, help="Input file", type=argparse.FileType('r'))
    # parser.add_argument("-o", "--out", dest="out_file", nargs=1,
    #                     required=True, help="Output file", type=argparse.FileType('w'))
    args = parser.parse_args()

    print("Parsing {0}".format(
        args.in_file[0].name))
    stats = parse_file(args.in_file[0])

    stats2 = compute_mean_var(stats)
    plot_stats(stats2)
    print("Done")
