import os
import math
import numpy
import scipy.stats

import matplotlib.pyplot as plt

# Valid fuzzers
FUZZERS = ["afldumb", "afl", "aflfast"]

# Number of trials
NUM_TRIALS = 40

# Results are accessed like: results[program][fuzzer][trial num]
#  0: Number of bugs
#  1: List of times
results = dict()

# Go through each subdir
for program in os.listdir(os.path.curdir):
    # Skip non-dirs
    if not os.path.isdir(os.path.join(os.path.curdir, program)): continue

    # Make a new dict entry
    results[program] = dict()

    # Go through each fuzzer
    for fuzzer in FUZZERS:
        # Make a new dict entry
        results[program][fuzzer] = dict()

        # Handle each trial
        for i in range(0, NUM_TRIALS):
            # Open the results file
            f = open(program + "/" + fuzzer + "\uf03a" + str(i) + ".txt")

            # Calculate the results
            lines = f.readlines()[1:]
            for j in range(0,len(lines)):
                lines[j] = float(lines[j].split(" ")[0])
            res = [len(lines),lines]

            # Store the results
            results[program][fuzzer][i] = res

        # Calculate summary stats
        mean = numpy.mean([v[0] for v in results[program][fuzzer].values()])
        stdev = numpy.std([v[0] for v in results[program][fuzzer].values()], ddof=1)
        results[program][fuzzer][-1] = (mean, stdev)

        # Show results
        print(program + ", " + fuzzer)
        print("mean  = " + str(mean))
        print("stdev = " + str(stdev))
        print("data  = " + str([v[0] for v in results[program][fuzzer].values()][:-1]))
        print()

    print("\n")

# Plot a program's time series
def plot_prog(program):
    fuzzers = {"afl":"b", "aflfast":"r"}

    for fuzzer in fuzzers.keys():
        for i in range(0, NUM_TRIALS):
            # Get x data (time)
            xvars = results[program][fuzzer][i][1]

            # Make y values (# bugs)
            yvars = [n for n in range(1,len(xvars)+1)]

            # Plot the series
            plt.plot(xvars, yvars, fuzzers[fuzzer] + "-", label=fuzzer+str(i))

    # Show plot
    #plt.legend(loc="best")
    plt.show()

#plot_prog("nm-new")

# Make an average time series (w/ confidence intervals) for a program,fuzzer combo
INTERVAL = 1
def avg_plot(program, fuzzer):
    for i in range(0, NUM_TRIALS):
        # Time info
        times = results[program][fuzzer][i][1]
        
        # Number of bugs found, current position in time series
        num_bugs = 0
        times_pos = 0

        # Traverse from 0 seconds to 86400 seconds (24 hours)
        new_times = []
        for t in range(0, 86400, INTERVAL):
            # Past the valid time series entries, so just put the old bug count and continue
            if times_pos >= len(times):
                new_times.append(num_bugs)
                continue

            # Get the current time from the time series and round it
            curtime = round(times[times_pos], 0)

            # Current second not close enough to rounded cur time
            if abs(t - curtime) > INTERVAL:
                new_times.append(num_bugs)
            # Current second within rounded cur time
            elif abs(t - curtime) <= INTERVAL:
                # We have at least one new bug
                num_bugs += 1

                # Advance, and continue our iteration until this condition now fails
                times_pos += 1
                try:
                    curtime = round(times[times_pos], 0)
                    while abs(t - curtime) <= INTERVAL:
                        num_bugs += 1

                        times_pos += 1
                        curtime = round(times[times_pos], 0)
                except IndexError:
                    pass

                new_times.append(num_bugs)
        
        # Store new times
        results[program][fuzzer][i][1] = new_times

    # Average for each interval, along with the lower and upper (CI) bounds
    """avg = []
    lb = []
    ub = []
    DIVISOR = math.sqrt(NUM_TRIALS)
    for tint in range(0, 86400//INTERVAL):
        # Get the data of all the trials
        data = [results[program][fuzzer][i][1][tint] for i in range(0, NUM_TRIALS)]

        # Find mean and standard deviation
        mean = numpy.mean(data)
        stdev = numpy.std(data, ddof=1)

        # Get the CI results
        ci = scipy.stats.norm.interval(0.95, loc=mean, scale=stdev/DIVISOR)

        # Fill out the outputs
        avg.append(mean)
        lb.append(ci[0])
        ub.append(ci[1])"""

    # Median for each interval, along with the Q1 and Q3 bounds
    avg = []
    lb = []
    ub = []
    for tint in range(0, 86400//INTERVAL):
        # Get the data of all the trials
        data = [results[program][fuzzer][i][1][tint] for i in range(0, NUM_TRIALS)]

        # Find the median
        median = scipy.stats.mstats.mquantiles(data)[1]

        # Get the confidence interval positions
        data.sort()
        
        lpos = (NUM_TRIALS / 2) - (1.96 * math.sqrt(NUM_TRIALS) / 2) - 1
        if lpos < 0: lpos = 0
        lpos = round(lpos, 0)
        lpos = int(lpos)

        upos = 1 + (NUM_TRIALS / 2) + (1.96 * math.sqrt(NUM_TRIALS) / 2) - 1
        upos = round(upos, 0)
        if upos >= NUM_TRIALS: upos = NUM_TRIALS - 1
        upos = int(upos)

        # Fill out the outputs
        avg.append(median)
        lb.append(data[lpos])
        ub.append(data[upos])

    # Return all the relevant info
    return avg, lb, ub

# Plot the averages for one program/fuzzer
def plot_avg_one(program, fuzzer, color):
    # Gather the data
    avg, lb, ub = avg_plot(program, fuzzer)

    # Make an X series
    xvars = [t for t in range(0, 86400, INTERVAL)]

    # Plot averages
    plt.plot(xvars, avg, color + "-", label=fuzzer)

    # Plot CI bounds
    plt.plot(xvars, lb, color + "--", label=fuzzer)
    plt.plot(xvars, ub, color + "--", label=fuzzer)

# Plot the averages for one program
def plot_avg(program):
    plot_avg_one(program, "afl", "b")
    plot_avg_one(program, "aflfast", "r")
    plt.ylim(ymin=0)
    plt.title(program + ": AFL vs. AFLFast")
    plt.legend(loc="best")
    plt.show()

#plot_avg("FFmpeg")

def compare_prog(program):
    # Construct the bug count samples for AFL and AFLFast
    afldata = [results[program]["afl"][i][0] for i in range(0, NUM_TRIALS)]
    aflfastdata = [results[program]["aflfast"][i][0] for i in range(0, NUM_TRIALS)]

    # Perform the test
    uval, pval = scipy.stats.mannwhitneyu(afldata, aflfastdata, alternative="two-sided")

    # Return the p-value
    return pval

print(compare_prog("FFmpeg"))
