// Assembler.cpp

// Assembler for the W65C816S

// g++ -o Assembler.o Assembler.cpp


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

unsigned long current_line = 0;
unsigned long current_location = 0;

const unsigned long equal_max = 10000;
char equal_name[equal_max][256];
unsigned long equal_location[equal_max];
unsigned long equal_total = 0;
unsigned long equal_count = 0;

const unsigned long label_max = 10000;
char label_name[label_max][256];
unsigned long label_location[label_max];
unsigned long label_total = 0;
unsigned long label_count = 0;

const unsigned long star_max = 10000;
unsigned long star_location[star_max];
unsigned long star_total = 0;
unsigned long star_count = 0;

unsigned char binary_code[16777216]; // 16 MB possible code

const char *instruction_list[1024] = {

	"BRK","s","7","2",
	"ORA","(d,x)","6","2",
	"COP","s","7","2",
	"ORA","d,s","4","2",
	"TSB","d","5","2",
	"ORA","d","3","2",
	"ASL","d","5","2",
	"ORA","[d]","6","2",
	"PHP","s","3","1",
	"ORA","#","2","2",
	"ASL","A","2","1",
	"PHD","s","4","1",
	"TSB","a","6","3",
	"ORA","a","4","3",
	"ASL","a","6","3",
	"ORA","al","5","4",

	"BPL","r","2","2",
	"ORA","(d),y","5","2",
	"ORA","(d)","5","2",
	"ORA","(d,s),y","7","2",
	"TRB","d","5","2",
	"ORA","d,x","4","2",
	"ASL","d,x","6","2",
	"ORA","[d],y","6","2",
	"CLC","i","4","1",
	"ORA","a,y","4","3",
	"INC","A","2","1",
	"TCS","i","2","1",
	"TRB","a","6","3",
	"ORA","a,x","4","3",
	"ASL","a,x","7","3",
	"ORA","al,x","5","4",

	"JSR","a","6","3",
	"AND","(d,x)","6","2",
	"JSL","al","8","4",
	"AND","d,s","4","2",
	"BIT","d","3","2",
	"AND","d","3","2",
	"ROL","d","5","2",
	"AND","[d]","6","2",
	"PLP","s","4","1",
	"AND","#","2","2",
	"ROL","A","2","1",
	"PLD","s","5","1",
	"BIT","a","4","3",
	"AND","a","4","3",
	"ROL","a","6","3",
	"AND","al","5","4",

	"BMI","r","2","2",
	"AND","(d),y","5","2",
	"AND","(d)","5","2",
	"AND","(d,s),y","7","2",
	"BIT","d,x","4","2",
	"AND","d,x","4","2",
	"ROL","d,x","6","2",
	"AND","[d],y","6","2",
	"SEC","i","2","1",
	"AND","a,y","4","3",
	"DEC","A","2","1",
	"TSC","i","2","1",
	"BIT","a,x","4","3",
	"AND","a,x","4","3",
	"ROL","a,x","7","3",
	"AND","al,x","5","4",

	"RTI","s","7","1",
	"EOR","(d,x)","6","2",
	"WDM","i","2","2",
	"EOR","d,s","4","2",
	"MVP","xyc","7","3",
	"EOR","d","3","2",
	"LSR","d","5","2",
	"EOR","[d]","6","2",
	"PHA","s","3","1",
	"EOR","#","2","2",
	"LSR","A","2","1",
	"PHK","s","3","1",
	"JMP","a","3","3",
	"EOR","a","4","3",
	"LSR","a","6","3",
	"EOR","al","5","4",

	"BVC","r","2","2",
	"EOR","(d),y","5","2",
	"EOR","(d)","5","2",
	"EOR","(d,s),y","7","2",
	"MVN","xyc","7","3",
	"EOR","d,x","4","2",
	"LSR","d,x","6","2",
	"EOR","[d],y","6","2",
	"CLI","i","2","1",
	"EOR","a,y","4","3",
	"PHY","s","3","1",
	"TCD","i","2","1",
	"JMP","al","4","4",
	"EOR","a,x","4","3",
	"LSR","a,x","7","3",
	"EOR","al,x","5","4",

	"RTS","s","6","1",
	"ADC","(d,x)","6","2",
	"PER","s","6","3",
	"ADC","d,s","4","2",
	"STZ","d","3","2",
	"ADC","d","3","2",
	"ROR","d","5","2",
	"ADC","[d]","6","2",
	"PLA","s","4","1",
	"ADC","#","2","2",
	"ROR","A","2","1",
	"RTL","s","6","1",
	"JMP","(a)","5","3",
	"ADC","a","4","3",
	"ROR","a","6","3",
	"ADC","al","5","4",

	"BVS","r","2","2",
	"ADC","(d),y","5","2",
	"ADC","(d)","5","2",
	"ADC","(d,s),y","7","2",
	"STZ","d,x","4","2",
	"ADC","d,x","4","2",
	"ROR","d,x","6","2",
	"ADC","[d],y","6","2",
	"SEI","i","2","1",
	"ADC","a,y","4","3",
	"PLY","s","4","1",
	"TDC","i","2","1",
	"JMP","(a,x)","6","3",
	"ADC","a,x","4","3",
	"ROR","a,x","7","3",
	"ADC","al,x","5","4",

	"BRA","r","2","2",
	"STA","(d,x)","6","2",
	"BRL","rl","4","3",
	"STA","d,s","4","2",
	"STY","d","3","2",
	"STA","d","3","2",
	"STX","d","3","2",
	"STA","[d]","2","2",
	"DEY","i","2","1",
	"BIT","#","2","2",
	"TXA","i","2","1",
	"PHB","s","3","1",
	"STY","a","4","3",
	"STA","a","4","3",
	"STX","a","4","3",
	"STA","al","5","4",

	"BCC","r","2","2",
	"STA","(d),y","6","2",
	"STA","(d)","5","2",
	"STA","(d,s),y","7","2",
	"STY","d,x","4","2",
	"STA","d,x","4","2",
	"STX","d,y","4","2",
	"STA","[d],y","6","2",
	"TYA","i","2","1",
	"STA","a,y","5","3",
	"TXS","i","2","1",
	"TXY","i","2","1",
	"STZ","a","4","3",
	"STA","a,x","5","3",
	"STZ","a,x","5","3",
	"STA","al,x","5","4",
	
	"LDY","#","2","2",
	"LDA","(d,x)","6","2",
	"LDX","#","2","2",
	"LDA","d,s","4","2",
	"LDY","d","3","2",
	"LDA","d","3","2",
	"LDX","d","3","2",
	"LDA","[d]","6","2",
	"TAY","i","2","1",
	"LDA","#","2","2",
	"TAX","i","2","1",
	"PLB","s","4","1",
	"LDY","a","4","3",
	"LDA","a","4","3",
	"LDX","a","4","3",
	"LDA","al","5","4",

	"BCS","r","2","2",
	"LDA","(d),y","5","2",
	"LDA","(d)","5","2",
	"LDA","(d,s),y","7","2",
	"LDY","d,x","4","2",
	"LDA","d,x","4","2",
	"LDX","d,y","4","2",
	"LDA","[d],y","6","2",
	"CLV","i","2","1",
	"LDA","a,y","4","3",
	"TSX","i","2","1",
	"TYX","i","2","1",
	"LDY","a,x","4","3",
	"LDA","a,x","4","3",
	"LDX","a,y","4","3",
	"LDA","al,x","5","4",

	"CPY","#","2","2",
	"CMP","(d,x)","6","2",
	"REP","#","3","2",
	"CMP","d,s","4","2",
	"CPY","d","3","2",
	"CMP","d","3","2",
	"DEC","d","5","2",
	"CMP","[d]","6","2",
	"INY","i","2","1",
	"CMP","#","2","2",
	"DEX","i","2","1",
	"WAI","i","3","1",
	"CPY","a","4","3",
	"CMP","a","4","3",
	"DEC","a","6","3",
	"CMP","al","5","4",

	"BNE","r","2","2",
	"CMP","(d),y","5","2",
	"CMP","(d)","5","2",
	"CMP","(d,s),y","7","2",
	"PEI","s","6","2",
	"CMP","d,x","4","2",
	"DEC","d,x","6","2",
	"CMP","[d],y","6","2",
	"CLD","i","2","1",
	"CMP","a,y","4","3",
	"PHX","s","3","1",
	"STP","i","3","1",
	"JML","(a)","6","3",
	"CMP","a,x","4","3",
	"DEC","a,x","7","3",
	"CMP","al,x","5","4",

	"CPX","#","2","2",
	"SBC","(d,x)","6","2",
	"SEP","#","3","2",
	"SBC","d,s","4","2",
	"CPX","d","3","2",
	"SBC","d","3","2",
	"INC","d","5","2",
	"SBC","[d]","6","2",
	"INX","i","2","1",
	"SBC","#","2","2",
	"NOP","i","2","1",
	"XBA","i","3","1",
	"CPX","a","4","3",
	"SBC","a","4","3",
	"INC","a","6","3",
	"SBC","al","5","4",

	"BEQ","r","2","2",
	"SBC","(d),y","5","2",
	"SBC","(d)","5","2",
	"SBC","(d,s),y","7","2",
	"PEA","s","5","3",
	"SBC","d,x","4","2",
	"INC","d,x","6","2",
	"SBC","[d],y","6","2",
	"SED","i","2","1",
	"SBC","a,y","4","3",
	"PLX","s","4","1",
	"XCE","i","2","1",
	"JSR","(a,x)","8","3",
	"SBC","a,x","4","3",
	"INC","a,x","7","3",
	"SBC","al,x","5","4",
	
};

int read_value(FILE *input, int pass, unsigned long &value, int &count, unsigned char &mods)
{
	value = 0;
	count = 0;
	mods = 0x00;

	int bytes = 1;
	char buffer = 0;

	char name[256];

	for (int i=0; i<256; i++) name[i] = 0;

	int total = 0;

	int dist = 0;

	unsigned char changes = 0x00;
	
	bool comment = false;

	while (bytes > 0 && buffer != '\n')
	{
		bytes = fscanf(input, "%c", &buffer);
		
		if (bytes > 0 && buffer != '\n')
		{
			if (buffer >= '0' && buffer <= '9' && comment == false)
			{
				value *= 16;
				value += (unsigned long)(buffer - '0');

				count++;
			}
			else if (buffer >= 'A' && buffer <= 'F' && comment == false)
			{
				value *= 16;
				value += (unsigned long)(buffer - 'A' + 10);

				count++;
			}
			else if (((buffer >= 'a' && buffer <= 'z') || buffer == '_') && comment == false)
			{
				name[total] = buffer;
				total++;
			}
			else if (buffer == ';')
			{
				comment = true;
			}
			else if (comment == false)
			{
				if (buffer == '#')
				{
					mods = mods | 0x01;
				}
				else if (buffer == '(' || buffer == ')')
				{
					mods = mods | 0x02;
				}
				else if (buffer == '[' || buffer == ']')
				{
					mods = mods | 0x04;
				}
				else if (buffer == 'X')
				{
					mods = mods | 0x08;
				}
				else if (buffer == 'Y')
				{
					mods = mods | 0x10;
				}
				else if (buffer == 'S')
				{
					mods = mods | 0x20;
				}
				else if (buffer == '<')
				{
					changes = changes | 0x01;
				}
				else if (buffer == '>')
				{
					changes = changes | 0x02;
				}
				else if (buffer == '=')
				{
					changes = changes | 0x04;
				}
				else if (buffer == '^')
				{
					changes = changes | 0x08;
				}
				else if (buffer == '+')
				{
					dist++;
				}
				else if (buffer == '-')
				{
					dist--;
				}
			}
		}
	}

	if (total > 0)
	{
		comment = false;

		for (unsigned long i=0; i<equal_total; i++)
		{
			if (strcmp(name, equal_name[i]) == 0)
			{
				value = equal_location[i];
				count = 6;

				comment = true;
			
				break;
			}
		}

		if (comment == false)
		{
			for (unsigned long i=0; i<label_total; i++)
			{
				if (strcmp(name, label_name[i]) == 0)
				{
					value = label_location[i];
					count = 4;
	
					comment = true;
				
					break;
				}
			}

			if (comment == false) 
			{
				if (pass > 1) // not on first or second passes
				{
					printf("Line %lu: Unknown equal/label %s\n", current_line, name);
				}
			}
		}
	}

	if (dist != 0)
	{
		if (dist < 0) dist++;

		comment = false;

		for (unsigned long i=0; i<star_total-1; i++)
		{
			if (star_location[i] <= current_location &&
				star_location[i+1] > current_location)
			{
				comment = true;	

				if (i+dist >= 0 && i+dist < star_total)
				{
					value = star_location[i+dist];
					count = 4;
				}
				else
				{
					if (pass > 1) // not on first or second passes
					{
						printf("Line %lu: Unknown star location\n", current_line);
					}
				}

				break;
			}
		}

		if (comment == false)
		{
			if (pass > 1) // not on first or second passes
			{
				printf("Line %lu: Unknown star location\n", current_line);
			}
		}
	}

	if ((changes & 0x01) > 0) // <
	{
		value = (unsigned long)(value % 256);
		count = 2;
	}
	else if ((changes & 0x02) > 0) // <
	{
		value = (unsigned long)(value / 256) % 256;
		count = 2;
	}
	else if ((changes & 0x04) > 0) // =
	{
		value = (unsigned long)(value % 65536);
		count = 4;
	}
	else if ((changes & 0x08) > 0) // ^
	{
		value = (unsigned long)(value / 65536) % 256;
		count = 2;
	}

	if (count >= 6)
	{
		value = value;
	}
	else if (count >= 4)
	{
		value = (value % 65536);
	}
	else if (count >= 2)
	{
		value = (value % 256);
	}
	else
	{
		value = 0;
	}

	return bytes;
};

int read_line(FILE *input, int pass)
{
	int bytes = 1;
	char buffer = 0;
	int count;
	int tally;
	char name[256];
	unsigned long value;
	bool comment;
	bool quotes;
	
	unsigned char mods;
	signed long dist;
	
	bool loop = true;

	while (bytes > 0 && loop == true)
	{
		bytes = fscanf(input, "%c", &buffer);

		if (bytes > 0)
		{
			if (buffer == '\n') // new line
			{
				loop = false;
			}
			else if (buffer == ';') // comment
			{
				while (bytes > 0 && buffer != '\n')
				{
					bytes = fscanf(input, "%c", &buffer);
				}

				loop = false;
			}
			else if (buffer == ':') // label
			{
				count = 0;

				comment = false;

				while (bytes > 0 && buffer != '\n')
				{
					bytes = fscanf(input, "%c", &buffer);

					if (bytes > 0 && ((buffer >= 'a' && buffer <= 'z') || buffer == '_') && comment == false)
					{
						label_name[label_count][count] = buffer;
					
						count++;
					}
					else if (bytes > 0 && buffer == ';')
					{
						comment = true;
					}
				}

				label_location[label_count] = current_location;

				label_count++;

				if (pass == 0) label_total++;
				
				loop = false;
			}
			else if (buffer == '*') // star
			{
				while (bytes > 0 && buffer != '\n')
				{
					bytes = fscanf(input, "%c", &buffer);
				}

				star_location[star_count] = current_location;
		
				star_count++;

				if (pass == 0) star_total++;

				loop = false;
			}
			else if (buffer == '.') // command
			{
				for (int i=0; i<256; i++) name[i] = 0;
				
				count = 0;

				comment = false;

				while (bytes > 0 && (buffer != ' ' && buffer != '\n'))
				{
					bytes = fscanf(input, "%c", &buffer);
		
					if (bytes > 0 && (buffer >= 'A' && buffer <= 'Z') && comment == false)
					{
						name[count] = buffer;

						count++;
					}
					else if (bytes > 0 && buffer == ';')
					{
						comment = true;
					}
				}

				if (strcmp(name, "EQU") == 0)
				{
					count = 0;
	
					value = 0;

					comment = false;

					while (bytes > 0 && buffer != '\n')
					{
						bytes = fscanf(input, "%c", &buffer);

						if (bytes > 0 && ((buffer >= 'a' && buffer <= 'z') || buffer == '_') && comment == false)
						{
							equal_name[equal_count][count] = buffer;
						
							count++;
						}
						else if (bytes > 0 && (buffer >= '0' && buffer <= '9') && comment == false)
						{
							value *= 16;
							value += (unsigned long)(buffer - '0');
						}
						else if (bytes > 0 && (buffer >= 'A' && buffer <= 'F') && comment == false)
						{
							value *= 16;
							value += (unsigned long)(buffer - 'A' + 10);
						}
						else if (bytes > 0 && buffer == ';')
						{
							comment = true;
						}
					}

					equal_location[equal_count] = value;
	
					equal_count++;

					if (pass == 0) equal_total++;
				}
				else if (strcmp(name, "DAT") == 0)
				{
					value = 0;

					tally = 0;

					for (int i=0; i<256; i++) name[i] = 0;
				
					count = 0;

					comment = false;

					quotes = false;

					while (bytes > 0 && buffer != '\n')
					{
						bytes = fscanf(input, "%c", &buffer);

						if (bytes > 0 && buffer != '\n' && quotes == true)
						{
							if (buffer == '"') quotes = false;
							else
							{
								binary_code[current_location] = (unsigned char)(buffer);

								current_location += 1;
							}
						}
						else
						{
							if (bytes > 0 && ((buffer >= 'a' && buffer <= 'z') || buffer == '_') && comment == false)
							{
								name[count] = buffer;
							
								count++;
							}
							else if (bytes > 0 && (buffer >= '0' && buffer <= '9') && comment == false)
							{
								value *= 16;
								value += (unsigned long)(buffer - '0');
	
								tally++;
							}
							else if (bytes > 0 && (buffer >= 'A' && buffer <= 'F') && comment == false)
							{
								value *= 16;
								value += (unsigned long)(buffer - 'A' + 10);
	
								tally++;
							}
							else if (bytes > 0 && buffer == '"' && comment == false)
							{
								quotes = true;
							}
							else if (bytes > 0 && buffer == ',' && comment == false)
							{
								if (tally > 0)
								{
									if (tally >= 6)
									{
										binary_code[current_location] = (unsigned char)(value % 256);
										binary_code[current_location+1] = (unsigned char)((value / 256) % 256);
										binary_code[current_location+2] = (unsigned char)((value / 65536) % 256);

										current_location += 3;
									}
									else if (tally >= 4)
									{
										binary_code[current_location] = (unsigned char)(value % 256);
										binary_code[current_location+1] = (unsigned char)((value / 256) % 256);

										current_location += 2;
									}
									else if (tally >= 2)
									{
										binary_code[current_location] = (unsigned char)(value % 256);

										current_location += 1;
									}
								}
								
								if (count > 0)
								{
									comment = false;

									for (unsigned long i=0; i<label_total; i++)
									{
										if (strcmp(name, label_name[i]) == 0)
										{
											binary_code[current_location] = (unsigned char)(label_location[i] % 256);
											binary_code[current_location+1] = (unsigned char)((label_location[i] / 256) % 256);

											current_location += 2;
								
											comment = true;
											
											break;
										}
									}

									if (comment == false) 
									{
										if (pass > 1) // not on first or second passes
										{
											printf("Line %lu: Unknown label %s\n", current_line, name);
										}
									}

									comment = false;
								}

								value = 0;

								tally = 0;
	
								for (int i=0; i<256; i++) name[i] = 0;
					
								count = 0;
							}
							else if (bytes > 0 && buffer == ';')
							{
								comment = true;
							}
						}
					}

					if (tally > 0)
					{
						if (tally >= 6)
						{
							binary_code[current_location] = (unsigned char)(value % 256);
							binary_code[current_location+1] = (unsigned char)((value / 256) % 256);
							binary_code[current_location+2] = (unsigned char)((value / 65536) % 256);

							current_location += 3;
						}
						else if (tally >= 4)
						{
							binary_code[current_location] = (unsigned char)(value % 256);
							binary_code[current_location+1] = (unsigned char)((value / 256) % 256);

							current_location += 2;
						}
						else if (tally >= 2)
						{
							binary_code[current_location] = (unsigned char)(value % 256);

							current_location += 1;
						}
					}
					
					if (count > 0)
					{
						comment = false;

						for (unsigned long i=0; i<label_total; i++)
						{
							if (strcmp(name, label_name[i]) == 0)
							{
								binary_code[current_location] = (unsigned char)(label_location[i] % 256);
								binary_code[current_location+1] = (unsigned char)((label_location[i] / 256) % 256);

								current_location += 2;
					
								comment = true;
								
								break;
							}
						}

						if (comment == false) 
						{
							if (pass > 1) // not on first or second passes
							{
								printf("Line %lu: Unknown label %s\n", current_line, name);
							}
						}
					}
				}
				else if (strcmp(name, "ORG") == 0)
				{
					value = 0;

					comment = false;

					while (bytes > 0 && buffer != '\n')
					{
						bytes = fscanf(input, "%c", &buffer);

						if (bytes > 0 && (buffer >= '0' && buffer <= '9') && comment == false)
						{
							value *= 16;
							value += (unsigned long)(buffer - '0');
						}
						else if (bytes > 0 && (buffer >= 'A' && buffer <= 'F') && comment == false)
						{
							value *= 16;
							value += (unsigned long)(buffer - 'A' + 10);
						}
						else if (bytes > 0 && buffer == ';')
						{
							comment = true;
						}
					}

					current_location = value;
				}
				else if (strcmp(name, "LOC") == 0)
				{
					count = 0;

					for (int i=0; i<256; i++) name[i] = 0;

					while (bytes > 0 && buffer != '\n')
					{
						bytes = fscanf(input, "%c", &buffer);

						if (bytes > 0 && buffer != '\n')
						{
							name[count] = buffer;

							count++;
						}
					}

					if (pass > 1) // not on first or second passes
					{
						printf("Line %lu: $%06lx %s\n", current_line, current_location, name);
					}
				}
				else
				{
					printf("Line %lu: Unknown command %s\n", current_line, name);
				}

				loop = false;
			}
			else if (buffer == '\t' && pass > 0) // instruction
			{
				for (int i=0; i<256; i++) name[i] = 0;
				
				count = 0;

				comment = false;

				buffer = 0;

				while (bytes > 0 && (buffer != ' ' && buffer != '\t' && buffer != ';' && buffer != '\n'))
				{
					bytes = fscanf(input, "%c", &buffer);
		
					if (bytes > 0 && buffer == '\n')
					{
						comment = true;
					}
					else if (bytes > 0 && buffer == ';')
					{
						comment = true;
					}
					else if (bytes > 0 && (buffer >= 'A' && buffer <= 'Z') && comment == false)
					{
						name[count] = buffer;

						count++;
					}
				}

				count = 0;
				value = 0;
				mods = 0;

				if (comment == false)
				{
					bytes = read_value(input, pass, value, count, mods);
				}
				else
				{
					while (bytes > 0 && buffer != '\n')
					{
						bytes = fscanf(input, "%c", &buffer);
					}
				}

				comment = false;

				for (int i=0; i<1024; i+=4)
				{
					if (strcmp(name, instruction_list[i]) == 0 &&
						atoi(instruction_list[i+3]) == (int)(count / 2) + 1)
					{
						if (mods == 0x00 && 
							strcmp(instruction_list[i+1], "a") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)(value / 256);

							current_location += 3;

							break;
						}
						else if ((mods & 0x0A) == 0x0A && (mods & 0xF5) == 0x00 &&
							strcmp(instruction_list[i+1], "(a,x)") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)(value / 256);

							current_location += 3;

							break;
						}
						else if ((mods & 0x08) == 0x08 && (mods & 0xF7) == 0x00 &&
							strcmp(instruction_list[i+1], "a,x") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)(value / 256);

							current_location += 3;

							break;
						}
						else if ((mods & 0x10) == 0x10 && (mods & 0xEF) == 0x00 &&
							strcmp(instruction_list[i+1], "a,y") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)(value / 256);

							current_location += 3;

							break;
						}
						else if ((mods & 0x02) == 0x02 && (mods & 0xFD) == 0x00 &&
							strcmp(instruction_list[i+1], "(a)") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)(value / 256);

							current_location += 3;

							break;
						}
						else if ((mods & 0x08) == 0x08 && (mods & 0xF7) == 0x00 &&
							strcmp(instruction_list[i+1], "al,x") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)((value / 256) % 256);
							binary_code[current_location+3] = (unsigned char)((value / 65536) % 256);

							current_location += 4;

							break;
						}
						else if (mods == 0x00 && 
							strcmp(instruction_list[i+1], "al") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)((value / 256) % 256);
							binary_code[current_location+3] = (unsigned char)((value / 65536) % 256);

							current_location += 4;

							break;
						}
						else if (mods == 0x00 && 
							strcmp(instruction_list[i+1], "A") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
					
							current_location += 1;

							break;
						}
						else if (mods == 0x00 && 
							strcmp(instruction_list[i+1], "xyc") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;

							break;
						}
						else if ((mods & 0x0A) == 0x0A && (mods & 0xF5) == 0x00 &&
							strcmp(instruction_list[i+1], "(d,x)") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;

							break;
						}
						else if ((mods & 0x08) == 0x08 && (mods & 0xF7) == 0x00 &&
							strcmp(instruction_list[i+1], "d,x") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;

							break;
						}
						else if ((mods & 0x10) == 0x10 && (mods & 0xEF) == 0x00 &&
							strcmp(instruction_list[i+1], "d,y") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;

							break;
						}
						else if ((mods & 0x12) == 0x12 && (mods & 0xED) == 0x00 &&
							strcmp(instruction_list[i+1], "(d),y") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;
						
							break;
						}
						else if ((mods & 0x14) == 0x14 && (mods & 0xEB) == 0x00 &&
							strcmp(instruction_list[i+1], "[d],y") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;

							break;
						}
						else if ((mods & 0x04) == 0x04 && (mods & 0xFB) == 0x00 &&
							strcmp(instruction_list[i+1], "[d]") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;

							break;
						}
						else if ((mods & 0x02) == 0x02 && (mods & 0xFD) == 0x00 &&
							strcmp(instruction_list[i+1], "(d)") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;

							break;
						}
						else if (mods == 0x00 &&
							strcmp(instruction_list[i+1], "d") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;
					
							break;
						}
						else if ((mods & 0x01) == 0x01 && (mods & 0xFE) == 0x00 &&
							strcmp(instruction_list[i+1], "#") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)value;

							current_location += 2;

							break;
						}
						else if (mods == 0x00 &&
							strcmp(instruction_list[i+1], "i") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);

							current_location += 1;

							break;
						}
						else if (mods == 0x00 &&
							strcmp(instruction_list[i+1], "rl") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);
							binary_code[current_location+2] = (unsigned char)((value / 256) % 256);

							current_location += 3;

							break;
						}
						else if (mods == 0x00 &&
							strcmp(instruction_list[i+1], "r") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);

							current_location += 2;

							break;
						}
						else if (mods == 0x00 &&
							strcmp(instruction_list[i+1], "s") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);

							current_location += 1;

							break;
						}
						else if ((mods & 0x20) == 0x20 && (mods & 0xDF) == 0x00 &&
							strcmp(instruction_list[i+1], "d,s") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;
					
							break;
						}
						else if ((mods & 0x32) == 0x32 && (mods & 0xCD) == 0x00 &&
							strcmp(instruction_list[i+1], "(d,s),y") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);
							binary_code[current_location+1] = (unsigned char)(value % 256);	

							current_location += 2;
					
							break;
						}
					}
					else if (strcmp(name, instruction_list[i]) == 0 &&
						atoi(instruction_list[i+3]) == (int)(count / 2) + 0)
					{
						if (mods == 0x00 &&
							strcmp(instruction_list[i+1], "rl") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);

							if (value < current_location + 3)
							{
								dist = value - (current_location + 3);
							}
							else
							{
								dist = value - (current_location + 3);
							}

							if (dist < -32768 || dist > 32767)
							{
								binary_code[current_location+1] = 0x00;
								binary_code[current_location+2] = 0x00;

								if (pass > 1) printf("Line %lu: Relative branch too far, dist %ld\n", current_line, dist);
							}
							else
							{
								binary_code[current_location+1] = (unsigned char)((unsigned int)(dist) % 256);
								binary_code[current_location+2] = (unsigned char)(((unsigned int)(dist) / 256) % 256);
							}

							current_location += 3;

							break;
						}
						else if (mods == 0x00 &&
							strcmp(instruction_list[i+1], "r") == 0)
						{
							comment = true;

							binary_code[current_location] = (unsigned char)(i / 4);

							if (value < current_location + 2)
							{
								dist = value - (current_location + 2);
							}
							else
							{
								dist = value - (current_location + 2);
							}

							if (dist < -128 || dist > 127)
							{
								binary_code[current_location+1] = 0x00;

								if (pass > 1) printf("Line %lu: Relative branch too far, dist %ld\n", current_line, dist);
							}
							else
							{
								binary_code[current_location+1] = (unsigned char)(dist);
							}

							current_location += 2;

							break;
						}
					}
				}

				if (comment == false)
				{
					if (pass > 1) // not on first or second passes
					{
						printf("Line %lu: Unknown instruction %s\n", current_line, name);
					}
				}

				loop = false;
			}
		}
	}

	current_line++;

	return bytes;
};

int main(const int argc, const char **argv)
{
	printf("Assembler for the W65C816S 8/16-bit CPU\n");
	printf("Remember to enter native mode: CLC, then XCE\n");
	printf("Currently this does NOT implement REP #$30 and SEP #$30\n");
	printf("\n");

	for (unsigned long i=0; i<equal_max; i++)
	{
		for (int j=0; j<256; j++) equal_name[i][j] = 0;
		
		equal_location[i] = 0;
	}

	for (unsigned long i=0; i<label_max; i++)
	{
		for (int j=0; j<256; j++) label_name[i][j] = 0;
		
		label_location[i] = 0;
	}

	for (unsigned long i=0; i<star_max; i++)
	{	
		star_location[i] = 0;
	}

	if (argc < 6)
	{
		printf("Arguments: <input.asm> <output.bin> <output.hex> <output_start_bytes> <output_end_bytes>\n");
		
		return 0;
	}

	FILE *input;
	int bytes;

	for (int i=0; i<3; i++) // number of passes
	{
		printf("Pass %d\n", i+1);

		input = NULL;

		input = fopen(argv[1], "rt");
		if (!input)
		{
			printf("Error: Cannot open input file\n");
		
			return 0;
		}

		bytes = 1;

		current_location = 0; // starts at $000000

		current_line = 1; // starts at Line 1

		equal_count = 0;
		label_count = 0;
		star_count = 0;

		if (i == 0) // only on first pass
		{
			star_location[star_total] = 0x000000;
			star_total++;
		}

		star_count++;

		while (bytes > 0)
		{
			bytes = read_line(input, i);
		}

		if (i == 0) // only on first pass
		{
			star_location[star_total] = 0xFFFFFF;
			star_total++;
		}
	
		fclose(input);
	}

	unsigned long start_bytes = atoi(argv[4]);
	unsigned long end_bytes = atoi(argv[5]);

	if (start_bytes < 0) start_bytes = 0;
	if (end_bytes > 16777216) end_bytes = 16777216;

	FILE *output = NULL;
	
	output = fopen(argv[2], "wb");
	if (!output)
	{
		printf("Error: Cannot open binary write file\n");
	
		return 0;
	}

	for (unsigned long i=start_bytes; i<end_bytes; i++)
	{
		fprintf(output, "%c", binary_code[i]);
	}

	fclose(output);

	output = NULL;
	
	output = fopen(argv[3], "wt");
	if (!output)
	{
		printf("Error: Cannot open hex write file\n");
	
		return 0;
	}

	for (unsigned long i=start_bytes; i<end_bytes; i++)
	{
		fprintf(output, "0x%02X,", binary_code[i]);

		if (i % 16 == 15)
		{
			fprintf(output, "\n");
		}
	}

	fclose(output);

	printf("Complete\n\n");
	
/*
	// optional code below, comment out if need be
	char command[256];
	
	for (int i=0; i<256; i++) command[i] = 0;

	sprintf(command, "hexdump -C %s", argv[2]);

	system(command);
*/

	return 1;
}









