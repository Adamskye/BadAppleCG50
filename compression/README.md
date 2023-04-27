# Compression
Compression works in two stages.

## How stage 1 works
This is performed by ```compress.py```.

It performs run-length encoding on each frame and stores each run of pixels as numbers, separated by spaces.

There is no need to specify the colour before each run because there are only two colours, and so it is assumed that the colour of each run is the inverse of the colour of the previous run. It is assumed that each frame starts with a run of white.

Additionally, each frame is stored as a chequerboard. The gaps between the pixels will be reconstructed when the video is played.

## How stage 2 works
Stage 2 is performed by ```compress2.c``` on the output of stage 1. It involves using fastlz to compress the run-length encoded data in fixed blocks.

Larger block sizes allow for better compression ratios, but will use more memory on the calculator.

The fixed block sizes relate to the block sizes of the uncompressed data. The compressed data will, of course, vary wildly. Therefore, a file called ```length_list.txt``` will be created which contains a list of the length of each block, in order, which can then be transferred to the array in ```src/headers/blocklengths.h```.

## How to compress your own videos

### Preparing your video
Firstly, you need to convert your video frames into images, following a certain criteria:

* The resolution should fit within the calculator's display (384x216). I recommend 119x90 because that's what I used, though you are free to experiment with this.

* The width of the video should be odd due to how the chequerboard works.

* The video must be purely black and white (no greys).

* The frames must be numbered starting from 1 (e.g. 1.png, 2.png, 3.png, etc.).

### Stage 1

Once that is working, run ```compress.py```. If it throws any errors, you are probably missing dependencies and thus should install them. It will prompt you for the location of the frames and the filename of the compressed file.

### Stage 2

The default block size is set to 4096 bytes. You can change this in compress2.c if you want. In my experience, 6144 bytes and above don't work.

Run ```make``` to compile ```compress2.c```. Make sure the environment variable FXCGSDK is set to the location of the SDK.

Upon running the resulting executable, you will be asked the input filename (which you created in stage 1) and the output filename.

You should receive a .bin file containing the compressed data as well as a file called 'length_list.txt'.

### Configuring the player program

There will be an array in ```src/headers/blocklengths.h``` which contains the lengths of the compressed block lengths. Replace all numbers in this array with the numbers in ```length_list.txt``` you generated earlier.

Inside ```main.c```, there will be a variety of constants to edit.

* ```DECOMP_BLOCK_SIZE``` is the block size you used in the second compression stage.

* ```FRAMETIME``` is the length of each frame in milliseconds. Note that this is only accurate to intervals of 1/128 seconds, hence why I recommend 16 fps.

* ```N_FRAMES``` is the total number of frames.

* ```FILE_PATH``` is the path to the video data file on the calculator. Replace ```badata.bin``` with whatever the filename you chose for it is.

* ```SCREENWIDTH``` and ```SCREENHEIGHT``` are the pixel dimensions of the calculator screen. You probably don't need to edit this.

* ```GRIDWIDTH``` and ```GRIDHEIGHT``` are the pixel dimensions of the video.

### Compiling

Before you compile, you might want to change ```selected.bmp``` and ```unselected.bmp``` to some other bitmaps with the same resolution.

Now you are ready to compile. With the ```FXCGSDK``` environment variable set to the directory of the SDK, run ```make```. Copy the resulting ```.g3a``` file and the ```.bin``` file which contains the video to your calculator's root directory.
