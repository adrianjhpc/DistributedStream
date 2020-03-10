import xml.dom.minidom
import sys
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import math


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

    procs_per_node = 0
    threads_per_proc = 0
    nodes_used = 0
    copy_size = 0
    scale_size = 0
    add_size = 0
    triad_size = 0

    filename = sys.argv[1]

    doc = xml.dom.minidom.parse(filename)

    configuration = doc.getElementsByTagName("configuration")
    for element in configuration:
        procs_per_node_element = element.getElementsByTagName("processes_per_node")
        procs_per_node = int(procs_per_node_element[0].firstChild.nodeValue)
        threads_per_proc_element = element.getElementsByTagName("threads_per_process")
        threads_per_proc = int(threads_per_proc_element[0].firstChild.nodeValue)
        nodes_used_element = element.getElementsByTagName("number_of_nodes")
        nodes_used = int(nodes_used_element[0].firstChild.nodeValue)
        nodes_used_element = element.getElementsByTagName("copy_size")
        copy_size = int(nodes_used_element[0].firstChild.nodeValue)
        nodes_used_element = element.getElementsByTagName("scale_size")
        scale_size = int(nodes_used_element[0].firstChild.nodeValue)
        nodes_used_element = element.getElementsByTagName("add_size")
        add_size = int(nodes_used_element[0].firstChild.nodeValue)
        nodes_used_element = element.getElementsByTagName("triad_size")
        triad_size = int(nodes_used_element[0].firstChild.nodeValue)


    print(str(procs_per_node) + " processes, each with " + str(threads_per_proc) + " thread(s) on a total of " + str(nodes_used) + " nodes.")

    x, y = calculate_factors(nodes_used)

    copy_avg = np.full([x, y], np.NaN)
    copy_min = np.full([x, y], np.NaN)
    copy_max = np.full([x, y], np.NaN)

    scale_avg = np.full([x, y], np.NaN)
    scale_min = np.full([x, y], np.NaN)
    scale_max = np.full([x, y], np.NaN)

    add_avg = np.full([x, y], np.NaN)
    add_min = np.full([x, y], np.NaN)
    add_max = np.full([x, y], np.NaN)

    triad_avg = np.full([x, y], np.NaN)
    triad_min = np.full([x, y], np.NaN)
    triad_max = np.full([x, y], np.NaN)

    names = np.empty([x, y], dtype=object)

    i = 0
    j = 0
    nodes = doc.getElementsByTagName("node")
    for node in nodes:
        if(j == y):
            print("Error, too many nodes added")
            exit()
        name = node.getElementsByTagName("name")
        names[i, j] = name[0].firstChild.nodeValue
        copy = node.getElementsByTagName("Copy")
        for result in copy:
            avg = result.getElementsByTagName("Average")
            copy_avg[i,j] = (1E-6*procs_per_node*copy_size)/float(avg[0].firstChild.nodeValue)
            min = result.getElementsByTagName("Minimum")
            copy_min[i,j] = (1E-6*procs_per_node*copy_size)/float(min[0].firstChild.nodeValue)
            max = result.getElementsByTagName("Maximum")
            copy_max[i,j] = (1E-6*procs_per_node*copy_size)/float(max[0].firstChild.nodeValue)
        scale = node.getElementsByTagName("Scale")
        for result in scale:
            avg = result.getElementsByTagName("Average")
            scale_avg[i,j] = (1E-6*procs_per_node*scale_size)/float(avg[0].firstChild.nodeValue)
            min = result.getElementsByTagName("Minimum")
            scale_min[i,j] = (1E-6*procs_per_node*scale_size)/float(min[0].firstChild.nodeValue)
            max = result.getElementsByTagName("Maximum")
            scale_max[i,j] = (1E-6*procs_per_node*scale_size)/float(max[0].firstChild.nodeValue)
        add = node.getElementsByTagName("Add")
        for result in add:
            avg = result.getElementsByTagName("Average")
            add_avg[i,j] = (1E-6*procs_per_node*add_size)/float(avg[0].firstChild.nodeValue)
            min = result.getElementsByTagName("Minimum")
            add_min[i,j] = (1E-6*procs_per_node*add_size)/float(min[0].firstChild.nodeValue)
            max = result.getElementsByTagName("Maximum")
            add_max[i,j] = (1E-6*procs_per_node*add_size)/float(max[0].firstChild.nodeValue)
        triad = node.getElementsByTagName("Triad")
        for result in triad:
            avg = result.getElementsByTagName("Average")
            triad_avg[i,j] = (1E-6*procs_per_node*triad_size)/float(avg[0].firstChild.nodeValue)
            min = result.getElementsByTagName("Minimum")
            triad_min[i,j] = (1E-6*procs_per_node*triad_size)/float(min[0].firstChild.nodeValue)
            max = result.getElementsByTagName("Maximum")
            triad_max[i,j] = (1E-6*procs_per_node*triad_size)/float(max[0].firstChild.nodeValue)

        i = i + 1
        if(i == x):
            i = 0
            j = j + 1

    # Flip the arrays to make node numbering row rather than column format.
    names = names.transpose()
    copy_avg = copy_avg.transpose()
    copy_min = copy_min.transpose()
    copy_max = copy_max.transpose()
    scale_avg = scale_avg.transpose()
    scale_min = scale_min.transpose()
    scale_max = scale_max.transpose()
    add_avg = add_avg.transpose()
    add_min = add_min.transpose()
    add_max = add_max.transpose()
    triad_avg = triad_avg.transpose()
    triad_min = triad_min.transpose()
    triad_max = triad_max.transpose()

    # Replace NaN values with the minimum actual value in the array (i.e. ignoring NaNs).
    # This is required to deal with empty cells in the heatmap generated by node numbers that 
    # don't evenly decompose into a 2d node grid.
    min = np.nanmin(copy_avg)
    np.nan_to_num(copy_avg, nan=min)
    min = np.nanmin(copy_min)
    np.nan_to_num(copy_min, nan=min)
    min = np.nanmin(copy_max)
    np.nan_to_num(copy_max, nan=min)
        
    fig, ax = plt.subplots()
    im = ax.imshow(copy_avg)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(copy_avg[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Copy Average")
    fig.tight_layout()
    fig.savefig('copy_avg.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(copy_min)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(copy_min[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Copy Minimum")
    fig.tight_layout()
    fig.savefig('copy_min.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(copy_max)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(copy_max[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Copy Maximum")
    fig.tight_layout()
    fig.savefig('copy_max.png', dpi=fig.dpi)

    # Replace NaN values with the minimum actual value in the array (i.e. ignoring NaNs).
    # This is required to deal with empty cells in the heatmap generated by node numbers that 
    # don't evenly decompose into a 2d node grid.
    min = np.nanmin(scale_avg)
    np.nan_to_num(scale_avg, nan=min)
    min = np.nanmin(scale_min)
    np.nan_to_num(scale_min, nan=min)
    min = np.nanmin(scale_max)
    np.nan_to_num(scale_max, nan=min)
        
    fig, ax = plt.subplots()
    im = ax.imshow(scale_avg)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(scale_avg[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Scale Average")
    fig.tight_layout()
    fig.savefig('scale_avg.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(scale_min)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(scale_min[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Scale Minimum")
    fig.tight_layout()
    fig.savefig('scale_min.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(scale_max)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(scale_max[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Scale Maximum")
    fig.tight_layout()
    fig.savefig('scale_max.png', dpi=fig.dpi)

    # Replace NaN values with the minimum actual value in the array (i.e. ignoring NaNs).
    # This is required to deal with empty cells in the heatmap generated by node numbers that 
    # don't evenly decompose into a 2d node grid.
    min = np.nanmin(add_avg)
    np.nan_to_num(add_avg, nan=min)
    min = np.nanmin(add_min)
    np.nan_to_num(add_min, nan=min)
    min = np.nanmin(add_max)
    np.nan_to_num(add_max, nan=min)
        
    fig, ax = plt.subplots()
    im = ax.imshow(scale_avg)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(add_avg[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Add Average")
    fig.tight_layout()
    fig.savefig('add_avg.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(add_min)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(add_min[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Add Minimum")
    fig.tight_layout()
    fig.savefig('add_min.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(add_max)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(add_max[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Add Maximum")
    fig.tight_layout()
    fig.savefig('add_max.png', dpi=fig.dpi)

    # Replace NaN values with the minimum actual value in the array (i.e. ignoring NaNs).
    # This is required to deal with empty cells in the heatmap generated by node numbers that 
    # don't evenly decompose into a 2d node grid.
    min = np.nanmin(triad_avg)
    np.nan_to_num(triad_avg, nan=min)
    min = np.nanmin(triad_min)
    np.nan_to_num(triad_min, nan=min)
    min = np.nanmin(triad_max)
    np.nan_to_num(triad_max, nan=min)
        
    fig, ax = plt.subplots()
    im = ax.imshow(triad_avg)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(triad_avg[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Triad Average")
    fig.tight_layout()
    fig.savefig('triad_avg.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(triad_min)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(triad_min[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Triad Minimum")
    fig.tight_layout()
    fig.savefig('triad_min.png', dpi=fig.dpi)

    fig, ax = plt.subplots()
    im = ax.imshow(triad_max)
    cbar = plt.colorbar(im);
    cbar.set_label('Bandwidth (MB/s)')
    plt.axis('off')

    for i in range(0,y):
        for j in range(0,x):
            if j+(i*(y-1)) < nodes_used:
                text = ax.text(j, i, names[i, j] + "\n" + str(round(triad_max[i ,j],0)) + " MB/s", ha="center", va="center", color="b", fontsize=6, wrap=True)
            else:
                text = ax.text(j, i, "N/A", ha="center", va="center", color="b")

    ax.set_title("STREAM Triad Maximum")
    fig.tight_layout()
    fig.savefig('triad_max.png', dpi=fig.dpi)



if __name__ == "__main__":
    main()
