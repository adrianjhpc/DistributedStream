import sys
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import math
import pandas as pd

# Replace NaN values with the minimum actual value in the array (i.e. ignoring NaNs).
# This is required to deal with empty cells in the heatmap generated by node numbers that 
# don't evenly decompose into a 2d node grid.
def replace_nans(avg_array, min_array, max_array):
    min = np.nanmin(avg_array)
    np.nan_to_num(avg_array, nan=min)
    min = np.nanmin(min_array)
    np.nan_to_num(min_array, nan=min)
    min = np.nanmin(max_array)
    np.nan_to_num(max_array, nan=min)


# Plot a heat map of give data
def plot_graphs(data_set, x, y, nodes_used, names, graph_title, experiment_name, filename, dpi_value, vmin=None, vmax=None):

    fig, ax = plt.subplots(figsize=(y*2,x*2))
    if not vmin == None and not vmax == None:
       im = ax.imshow(data_set, vmin=0, vmax=5325)
    elif not vmin == None:
       im = ax.imshow(data_set, vmin=vmin)
    elif not vmax == None: 
       im = ax.imshow(data_set, vmax=vmax)
    else:
       im = ax.imshow(data_set)
    cbar = plt.colorbar(im);
    cbar.set_label('Sustained Flop Rate (GFlop/s)', fontsize=y*2)
    cbar.ax.tick_params(labelsize=y*2)
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*x) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(data_set[i ,j],0)) + " GFlop/s", ha="center", va="center", color="b", fontsize=10, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title(graph_title, fontsize=y*2)
    fig.tight_layout()
    fig.savefig(experiment_name + filename, dpi=dpi_value)


# Calculate a sensible 2d grid based on a number to enable 
# us to arrange our data into a 2d heat map.
# The divisor approach won't work for prime numbers, where it would 
# just return the factors 1 and number.  In this case we add 1 on to the
# number to make it non-prime and then find the divisors of that number.
# This means that for prime numbers the grid will contain an empty cell, 
# but that's more acceptable than having a 1d heat map
def calculate_factors(number):

    # Calculate the range of numbers that divide the provided number
    iters = 0
    num_found = 0
    # Iterate twice if we don't find any divisor other than -1 
    while num_found <= 1 and iters < 2:
        num_found = 0
        dividers = np.empty([0])
        for i in range(1,int(number/2)+1):
            if(number%i == 0):
                dividers.resize(dividers.size + 1)
                dividers[-1] = i
                num_found =  num_found + 1
        # If we go around the whole range and don't find a divisor, then this is a prime number. 
        # In this scenario, add one to the number, find the factors of that new number and return these.
        # This will enable a rectangle grid to be used but will mean there is a empty square.
        number = number + 1
        iters = iters + 1

    # Reset number to the last value in the loop so we can use it in the test below.
    number = number - 1
    # Choose the middle values in the divisor list to give the squarest grid possible
    # We use floor and ceiling here to so that if the list of divisors is even it choose the same 
    # value twice, and if the list has an odd number of entries choose the two elements next to 
    # each other near the middle of the list
    lower = int(dividers[int(math.floor(dividers.size/2))])
    upper = int(dividers[int(math.ceil(dividers.size/2))])

    # Check that the grid size matches the number used.
    if(lower*upper != number):
        print("Error calculating the size of the heat grid")
        exit()

    return (lower, upper)

def main():
    if(len(sys.argv) != 2):
        print("Error, expecting a single argument (the name of the results file to process)")
        print("Exiting")
        exit()

    dpi_value = 150
   
    procs_per_node = 0
    threads_per_proc = 0
    nodes_used = 0

    filename = sys.argv[1]

    df = pd.read_csv(filename, names=['nodename', 'gflops'])

    experiment_name = "HPL"

    nodes_used = df.shape[0]

    x, y = calculate_factors(nodes_used)

    gflops = np.full([x, y], np.NaN)

    names = np.empty([x, y], dtype=object)

    i = 0
    j = 0
    # Calculate the bandwidths from the recorded times and data sizes
    # The reason we use "Maximum" to set the min value and vice versa
    # is because the stored data are times, so the maximum runtime 
    # corresponds to the minimum bandwdith etc...
    for index, row in df.iterrows():
        if(j == y):
            print("Error, too many nodes added")
            exit()
        names[i, j] = row['nodename']
        gflops[i, j] = row['gflops']

        i = i + 1
        if(i == x):
            i = 0
            j = j + 1

    # Flip the arrays to make node numbering row rather than column format.
    names = names.transpose()
    gflops = gflops.transpose()
    
    plot_graphs(gflops, x, y, nodes_used, names, "HPL GFlops", experiment_name, "gflops_base.png", dpi_value)
    plot_graphs(gflops, x, y, nodes_used, names, "HPL GFlops", experiment_name, "gflops_zeroed.png", dpi_value, vmin=0)
    plot_graphs(gflops, x, y, nodes_used, names, "HPL GFlops", experiment_name, "gflops_theory.png", dpi_value, vmin=0, vmax=5325)
    
if __name__ == "__main__":
    main()
