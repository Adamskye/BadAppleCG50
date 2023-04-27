#include "../extern/fastlz.h"
#include "headers/blocklengths.h"
#include "stddef.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/rtc.h>
#include <fxcg/system.h>

#define DECOMP_BLOCK_SIZE 4096

// Note: RTC is only capable of measuring multiples of 1/128 seconds.
// It is measured in milliseconds.
#define FRAMETIME 66
#define N_FRAMES 3505

#define FILE_PATH "\\\\fls0\\badata.bin"

#define SCREENWIDTH 384
#define SCREENHEIGHT 216

#define GRIDWIDTH 119
#define GRIDHEIGHT 90

#define HORIZ_PADDING (SCREENWIDTH - (GRIDWIDTH * 2)) / 2
#define VERT_PADDING (SCREENHEIGHT - (GRIDHEIGHT * 2)) / 2

int load_file()
{
	unsigned short fname_buffer[sizeof(FILE_PATH) * 2];

	Bfile_StrToName_ncpy(fname_buffer, FILE_PATH, sizeof(FILE_PATH));

	int fhandle = Bfile_OpenFile_OS(fname_buffer, READ, 0);

	// check
	if (fhandle < 0) {
		int x = 0;
		int y = 0;
		PrintMiniMini(&x, &y, "Error reading file!", TEXT_MODE_NORMAL, TEXT_COLOR_RED, TEXT_MODE_NORMAL);
		exit(fhandle);
	}
	return fhandle;
}

void idx_to_grid(unsigned *gridrow, unsigned *gridcolumn, unsigned idx)
{
	*gridcolumn = idx % GRIDWIDTH;
	*gridrow = idx / GRIDWIDTH;
}

void invert_color(unsigned short *color)
{
	*color = *color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE;
}

int shift_digits(int num, unsigned shift_amount)
{
	switch (shift_amount) {
		case 1:
			return num * 10;
		case 2:
			return num * 100;
		case 3:
			return num * 1000;
		case 4:
			return num * 10000;
		default:
			return num;
	}
}

unsigned short get_px(unsigned x, unsigned y)
{
	return Bdisp_GetPoint_VRAM((x * 2) + HORIZ_PADDING, (y * 2) + VERT_PADDING);
}

void draw_px(unsigned x, unsigned y, color_t color)
{
	if (get_px(x, y) == color) {
		return;
	}

	Bdisp_SetPoint_VRAM((x * 2) + HORIZ_PADDING,
		(y * 2) + VERT_PADDING,
		color);
	Bdisp_SetPoint_VRAM((x * 2) + HORIZ_PADDING + 1,
		(y * 2) + VERT_PADDING,
		color);
	Bdisp_SetPoint_VRAM((x * 2) + HORIZ_PADDING,
		(y * 2) + VERT_PADDING + 1,
		color);
	Bdisp_SetPoint_VRAM((x * 2) + HORIZ_PADDING + 1,
		(y * 2) + VERT_PADDING + 1,
		color);
}

void fill_gaps()
{
	unsigned row = 0;
	unsigned col = 0;

	for (unsigned ptr = 1; ptr < GRIDHEIGHT * GRIDWIDTH; ptr += 2) {
		idx_to_grid(&row, &col, ptr);

		// get adjacent pixel colours
		unsigned top = row > 0 ? get_px(col, row - 1) : get_px(col, row + 1);
		unsigned bottom = row < (GRIDHEIGHT - 1) ? get_px(col, row + 1) : top;
		unsigned left = col > 0 ? get_px(col - 1, row) : get_px(col + 1, row);
		unsigned right = col < GRIDWIDTH - 1 ? get_px(col + 1, row) : left;

		// default to grey
		color_t color = COLOR_GRAY;

		if (top == bottom && left == right) {
			if (top == left) {
				// if all four adjacent pixels are the same
				color = top;
			}
		} else if (top == bottom) {
			// if only vertical adjacent pixels are the same
			color = top;
		} else if (left == right) {
			// if only horizontal adjacent pixels are the same
			color = left;
		}

		draw_px(col, row, color);
	}
}

void show_video()
{
	/* Load file */
	int input_file = load_file();

	/* Show frames */
	// unsigned short grid[GRIDHEIGHT][GRIDWIDTH] = {0};

	char comp_buffer[DECOMP_BLOCK_SIZE];
	char decomp_buffer[DECOMP_BLOCK_SIZE + 2];
	decomp_buffer[DECOMP_BLOCK_SIZE] = '\0';
	decomp_buffer[DECOMP_BLOCK_SIZE + 1] = '\0';

	unsigned ptr_grid = 0; // ptr_grid always points to the next pixel to be drawn
	unsigned short color = COLOR_WHITE;

	char *run_str = NULL;
	unsigned run_length = 0; // run_str converted to uint

	unsigned n_frame = 1;

	unsigned prev_frame_time = RTC_GetTicks();

	// A run number could be split into two because part of it occurs at the
	// end of a block and the other part occurs at the start of the next
	// block.
	// This is set to true when a block doesn't end with a space. This is
	// then double checked when the next block is loaded by making sure the
	// first character of the newly loaded block is also not a space.
	bool split_block = false;

	// LOOP THROUGH BLOCKS
	for (unsigned i_block = 0; i_block < sizeof(comp_blocklengths) / sizeof(comp_blocklengths[0]); ++i_block) {

		if (PRGM_GetKey() == KEY_PRGM_EXIT) {
			Bfile_CloseFile_OS(input_file);
			exit(0);
		}

		/**
		 * FETCH BLOCK INTO COMPRESSED BUFFER
		 */
		unsigned actual_comp_length = Bfile_ReadFile_OS(input_file, comp_buffer, comp_blocklengths[i_block], -1);

		/**
		 * DECOMPRESS BLOCK AND STORE IN DECOMPRESSED BUFFER
		 */
		unsigned actual_decomp_length = fastlz_decompress(comp_buffer, actual_comp_length, decomp_buffer, DECOMP_BLOCK_SIZE);

		/**
		 * PUT FRAME INTO GRID
		 */

		// double check if previous block was split
		if (split_block) {
			if (decomp_buffer[0] == ' ') {
				split_block = false;
			}
		}

		run_str = strtok(decomp_buffer, " ");

		/**
		 * KEEP ADDING RUNS TO GRID UNTIL END OF BLOCK IS REACHED
		 */
		while (run_str != NULL || run_str) {

			/**
			 * ADD THE RUN TO THE GRID IF THE FRAME ISN'T COMPLETE
			 */
			if (ptr_grid < GRIDHEIGHT * GRIDWIDTH) {
				// account for if the previous block was split
				if (split_block) {
					invert_color(&color);
					run_length = shift_digits(run_length, strlen(run_str)) + atoi(run_str) - run_length;
					split_block = false;
				} else {
					run_length = atoi(run_str);
				}

				for (unsigned i_pixel = 0; i_pixel < run_length; ++i_pixel) {
					unsigned gridrow;
					unsigned gridcolumn;

					idx_to_grid(&gridrow, &gridcolumn, ptr_grid);

					draw_px(gridcolumn, gridrow, color);

					ptr_grid += 2;
				}

				// fetch new run and then start from beginning
				invert_color(&color);
				run_str = strtok(NULL, " ");
				continue;
			}

			// NOW THE FRAME IS COMPLETE

			/**
			 * FILL GRID GAPS
			 */
			fill_gaps();

			/**
			 * DISPLAY
			 */
			while (!RTC_Elapsed_ms(prev_frame_time, FRAMETIME)) { }
			prev_frame_time = RTC_GetTicks();
			Bdisp_PutDisp_DD();

			/**
			 * GO TO NEXT FRAME
			 */
			++n_frame;
			if (n_frame > N_FRAMES) {
				Bfile_CloseFile_OS(input_file);
				return;
			}
			ptr_grid = 0;
			color = COLOR_WHITE;
		}

		// check if this block is split before loading the next one
		if (decomp_buffer[actual_decomp_length - 1] != '\0') {
			split_block = true;
		}
	}
}

int main(void)
{
	Bdisp_EnableColor(0);
	EnableStatusArea(3);
	DefineStatusAreaFlags(0, 0, NULL, NULL);

	show_video();
	exit(0);

	return 0;
}
