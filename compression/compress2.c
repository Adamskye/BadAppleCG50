#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../extern/fastlz.h"

#define BLOCK_SIZE 4096

void compress(const char *input_filename, const char *out_filename)
{
	FILE *src = fopen(input_filename, "r");

	// overwrite destination file if it already exists
	FILE *dst = fopen(out_filename, "w");
	FILE *length_list = fopen("length_list.txt", "w");

	// buffers
	char *comp_buffer = (char *)malloc(BLOCK_SIZE * 2);
	char decomp_buffer[BLOCK_SIZE + 1]; // leaving room for NULL terminator
	size_t decomp_buffer_size;
	size_t comp_buffer_size;

	// compress
	do {
		decomp_buffer_size = fread(decomp_buffer, 1, BLOCK_SIZE, src);
		comp_buffer_size = fastlz_compress_level(1, decomp_buffer, strlen(decomp_buffer), comp_buffer);

		fprintf(length_list, "%du\n", comp_buffer_size);
		fwrite(comp_buffer, 1, comp_buffer_size, dst);
		printf("Compressed: %lu bytes\n", ftell(src));
	} while (decomp_buffer_size >= BLOCK_SIZE);

	// clean up
	free(comp_buffer);
	fclose(src);
	fclose(dst);
	fclose(length_list);
}

int main(int argc, char **argv)
{
	/* COMPRESS DATA */
	char input_filename[128];
	char out_filename[128];

	printf("Please enter the input filename: ");
	scanf("%s", input_filename);

	printf("Please enter the output filename: ");
	scanf("%s", out_filename);

	compress(input_filename, out_filename);

	return 0;
}
