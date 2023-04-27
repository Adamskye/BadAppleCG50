from PIL import Image
import numpy as np
import os
from natsort import natsorted

def apply_runlength(arr, filename):
    current_clr = True  # true is white, false is black
    runlength = 0
    newarr = np.array([], dtype='i')

    pixel_num = 1

    for pixel in np.nditer(arr):
        if pixel_num % 2 == 0:
            pixel_num += 1
            continue

        if pixel == current_clr:
            runlength += 1
            pixel_num += 1
        else:
            newarr = np.append(newarr, runlength)

            # set runlength to 1 and not 0, because we should count this pixel
            # for the next run
            runlength = 1
            current_clr = not current_clr
            pixel_num += 1

    # account for the last run
    newarr = np.append(newarr, runlength)

    return newarr
            


def reverse_runlength(arr, dimensions):
    newarr = np.array([], dtype='b')

    current = True

    # convert runs back into binary 1s and 0s
    for run in arr:
        newarr = np.append(newarr, [current, 0] * run)
        current = not current

    # reshape to original dimensions
    newarr = np.reshape(newarr, dimensions)

    return newarr.astype(bool)
    


def compress(directory, filename, save_filename):
    # open a file
    img = Image.open(directory + filename, 'r')
    data = np.array(img)

    # runlength compress the file
    compressed = apply_runlength(data, filename)

    # save compressed information to new file
    with open(save_filename, mode='a') as file:
        np.savetxt(file, compressed, fmt='%s', newline=' ')



def decompress(filename):
    # load compressed data
    compressed = np.loadtxt(filename).astype(int)

    # reverse runlength encoding
    decompressed = reverse_runlength(compressed, (90, 119))

    # save new array to image
    img = Image.fromarray(decompressed)
    img.save("decompressed.png")



def main():
    choice = input("Do you want to compress or decompress? ")   

    if choice == "compress":
        directory = input("Please enter directory where frames are: ") + "/"
        save_filename = input("Please enter filename for compressed data: ").strip()

        framelist = os.listdir(directory)
        framelist = natsorted(framelist)

        num = 0
        for frame_filename in framelist:
            compress(directory, frame_filename, save_filename)
            num += 1
            print(f"Compressed {num} out of {len(framelist)}")

    elif choice == "decompress":
        filename = input("Filename: ")
        decompress(filename)


if __name__ == "__main__":
    main()
