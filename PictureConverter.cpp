// Converts a 320x240 .bmp file (with 192 pixels overscanned) into raw hex data

#include <stdio.h>
#include <stdlib.h>

int main(const int argc, const char **argv)
{
	if (argc < 3)
	{
		printf("Arguments: <input.bmp> <output.hex>\n");

		return 0;
	}

	FILE *input, *output;

	input = fopen(argv[1], "rb");
	if (!input)
	{
		printf("Error reading .bmp\n");

		return 0;
	}

	output = fopen(argv[2], "wt");
	if (!output)
	{
		printf("Error writing .hex\n");

		return 0;
	}

	unsigned char buffer;

	unsigned char value;

	unsigned char red, green, blue;

	unsigned char full[131072];

	for (int i=0; i<131072; i++)
	{
		full[i] = 0;
	}

	unsigned int total = 0;

	for (int i=0; i<54; i++) fscanf(input, "%c", &buffer); // header

	for (int i=0; i<240; i++)
	{
		for (int j=0; j<320; j++)
		{
			value = 0x00;

			fscanf(input, "%c%c%c", &blue, &green, &red);

			if (red < 64) value |= 0x00;
			else if (red < 128) value |= 0x10;
			else if (red < 192) value |= 0x20;
			else value |= 0x30;

			if (green < 64) value |= 0x00;
			else if (green < 128) value |= 0x04;
			else if (green < 192) value |= 0x08;
			else value |= 0x0C;

			if (blue < 64) value |= 0x00;
			else if (blue < 128) value |= 0x01;
			else if (blue < 192) value |= 0x02;
			else value |= 0x03;
			
			full[total] = value;

			total++;
		}
		
		for (int j=0; j<192; j++)
		{
			full[total] = 0x00;
		
			total++;
		}
	}

	int spacer = 0;

	char lower, upper;

	for (int i=240-1; i>=0; i--) // inverting the y-values, because... we need to?
	{
		for (int j=0; j<512; j++)
		{
			if (spacer == 0)	
			{
				fprintf(output, ".DAT ");
			}

			if ((full[(i*512)+j] / 16) <= 9) upper = (char)((full[(i*512)+j] / 16) + '0');
			else upper = (char)((full[(i*512)+j] / 16) - 10 + 'A');

			if ((full[(i*512)+j] % 16) <= 9) lower = (char)((full[(i*512)+j] % 16) + '0');
			else lower = (char)((full[(i*512)+j] % 16) - 10 + 'A');

			fprintf(output, "$%c%c", upper, lower);

			spacer++;

			if (spacer >= 8)
			{
				spacer = 0;

				fprintf(output, "\n");
			}
			else
			{	
				fprintf(output, ",");
			}
		}
	}

	fclose(input);

	fclose(output);
	
	return 1;
}
