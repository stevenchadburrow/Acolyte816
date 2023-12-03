// LastAcolyte6 C code for PIC16F886
// Bootloader and I/O device

/*

cd ~/Projects/PIC16 ; ~/Projects/sdcc/bin/sdcc --use-non-free -mpic14 -p16f886 LastAcolyte6.c ; cd ~/Projects/minipro ; ./minipro -p "pic16f886" -w ~/Projects/PIC16/LastAcolyte6.hex ; ./minipro -p "pic16f886" -c config -w ~/Projects/PIC16/LastAcolyte6.fuses -e

cd ~/Projects/PIC16 ; ~/Projects/sdcc/bin/sdcc --use-non-free -mpic14 -p16f887 LastAcolyte6.c ; cd ~/Projects/minipro ; ./minipro -p "pic16f887@DIP40" -w ~/Projects/PIC16/LastAcolyte6.hex ; ./minipro -p "pic16f887@DIP40" -c config -w ~/Projects/PIC16/LastAcolyte6.fuses -e

*/

/* 
The config words in sdcc don't work with the TL866II+ programmer,
so you need to make sure to have the LastAcolyte6.fuses file look like this:

word1 = 0x2cf4
word2 = 0x3eff
user_id0 = 0x3fff
user_id1 = 0x3fff
user_id2 = 0x3fff
user_id3 = 0x3fff
*/

/*
RA0-RA7 = DATA-BUS

RB0 = RDY, input, used in booting, later is active on PIC when low, used to interrupt
RB1 = PHI2-3V3, inout, controlled by PIC in boot, floating when not
RB2 = /RW, input, used to sense CPU status in boot
RB3 = /VP, input, used to sense CPU status in boot
RB4 = VPA, input, used to sense CPU status in boot
RB5 = VDA, input, used to sense CPU status in boot
RB6 = /NMI, inout, grounded on regular timed intervals, floating (pulled high) when not
RB7 = /RES, inout, grounded upon start of boot, then grounded to exit boot

RC0 = SHIFT-CLOCK-PIC-3V3, output, used to talk to VIA
RC1 = SHIFT-DATA-3V3, inout, used to talk to VIA
RC2 = SPI-CS, output, connects to SDcard
RC3 = SPI-CLK, output, connects to SDcard
RC4 = SPI-MISO, input, connects to SDcard
RC5 = SPI-MOSI, output, connects to SDcard
RC6 = TX/CK, output, for possible FT232RL
RC7 = RX/DT, input, for possible FT232RL

RE3 = /MCLR, master clear
*/

/*
The boot sequence is as follows:
	Bring /RES low and gain control of PHI2.
	Release /RES to be pulled high, start single stepping CPU.
	When /VP goes low, send $0000 reset vector (arbitrary).
	Send LDA# and STAa instructions, filling $FF00-$FFFF with boot code.
	When done, send WAI instruction to bring RDY low.
	Release PHI2, bring /RES low again.
	Initialize SDcard and setup PIC interrupts.
	Then release /RES to start the CPU running!
*/

/*
The SDcard (and other) command sequence is as follows:
	VIA brings SHIFT-DATA (CB2) low, turning off T0 interrupts on PIC.
	CPU turns off /IRQ interrupts entirely. 
	CPU sets up command data in RAM somewhere.
	CPU uses the WAI command, bringing RDY low.
	PIC interrupts on falling edge of RDY.
	PIC brings /NMI low and keeps it low.
	PIC waits for VIA to bring SHIFT-DATA (CB2) high, then starts shifting in command data.
	...
	Eventually PIC brings /NMI high.
	CPU turns /IRQ interrupts back on.
*/
	


// must have the library file locally in the same folder
#include "pic16f886.h"
//#include "pic16f887.h"


// these are useless here, but keeping them here for 'posterity' sake
unsigned int __at _CONFIG1 config1 = 0x2CF4;
unsigned int __at _CONFIG2 config2 = 0x3EFF;

// blocks for SDcard
unsigned char block_high = 0x00;
unsigned char block_mid = 0x00;
unsigned char block_low = 0x00;

// a short delay
void delay(void)
{
	unsigned char value = 0x00;

	while (value < 0x10) // arbitrary wait time
	{
		value = value + 1;
	}
}

// toggles PHI2 from low to high to low again
void toggle(void)
{
	// assumes PHI2 is low already
	RB1 = 1; // bring PHI2 high
	// delay needed?
	RB1 = 0; // bring PHI2 low
}

void serial_initialize(void)
{
	unsigned char value = 0x00;

	TRISC = TRISC | 0x80; // make sure RX is input
	TRISC = TRISC & 0x40; // make sure TX is output
	
	SPBRG = 25; // 4000000 / (64 * (9600 + 1)); // formula to set baud rate at 9600
	TXSTA = 0x24; // 'high speed' transmit enabled
	RCSTA = 0x90; // serial port enabled, continuous receives

	do
	{
		value = value + 1;
	}
	while (value != 0x00); // short delay
}

char serial_receivechar(void)
{
	while (RCIF == 0) { } // wait to receive character
	RCIF = 0; // clear flag

	return RCREG;
}

void serial_transmitchar(char value)
{
	while (TXIF == 0) { } // wait to transmit character
	TXIF = 0; // clear flag

	TXREG = value;
}

void serial_error(void)
{
	serial_transmitchar('E');
	serial_transmitchar('r');
	serial_transmitchar('r');
	serial_transmitchar('\n');
	serial_transmitchar('\r'); // send message to computer

	while (1) { } // loop forever
}

void nmi_ground(void)
{
	RB6 = 0; // get /NMI ready for low
	TRISB = TRISB & 0xBF; // /NMI is output
}

void nmi_float(void)
{
	TRISB = TRISB | 0x60; // release /NMI, pulled high
}

// shift in 8 bits from VIA
unsigned char shift_in(void)
{
	unsigned char value = 0x00;

	RC0 = 1; // set SHIFT-CLOCK high
	TRISC = TRISC & 0xFE; // make SHIFT-CLOCK output

	// shift register on VIA is already loaded with 8 bits in mode 111
	for (unsigned char i=0x00; i<0x08; i++)
	{
		RC0 = 0; // bring clock low
			
		value = value << 1; // shift buffer left, fill with zero
			
		if (RC1) // if data is high...
		{
			value = value + 0x01;
		}
	
		RC0 = 1; // bring clock high
	}

	TRISC = TRISC | 0x01; // release SHIFT-CLOCK, pulled high 

	return value;
}

// shift out 8 bits to VIA
void shift_out(unsigned char value)
{
	RC0 = 1; // set SHIFT-CLOCK high
	RC1 = 1; // set SHIFT-DATA high
	TRISC = TRISC & 0xFC; // make SHIFT-CLOCK and SHIFT-DATA output

	// shift register on VIA is ready in mode 011
	// and assumes SHIFT-SAFE-CLOCK is already high
	for (unsigned char i=0x00; i<0x08; i++)
	{
		RC0 = 0; // bring clock low
			
		if ((value & 0x7F)) // if data is high...
		{
			RC1 = 1; // set SHIFT-DATA high
		}
		else // if data is low...
		{
			RC1 = 0; // set SHIFT-DATA low
		}

		value = value << 1; // shift buffer left, fill with zero
	
		RC0 = 1; // bring clock high
	}

	TRISC = TRISC | 0x03; // release SHIFT-CLOCK and SHIFT-DATA, pulled high
}


// tells if the initialization process worked or not
unsigned char sdcard_ready = 0x00;

// SDcard commands below
// This was used for the Arduino, but has been modified to work here.
void sdcard_enable(void)
{
	RC2 = 0; // CS is low
}

void sdcard_disable(void)
{
	RC2 = 1; // CS is high
}

void sdcard_toggle(void)
{
	RC3 = 1; // CLK is high
	// delay?
	RC3 = 0; // CLK is low
}

void sdcard_longdelay(void)
{
	unsigned int tally = 0x0000;

	while (tally < 0x0200) // arbitrary amount of time to delay, should be around 10ms
	{
		tally = tally + 1;
	}
}

void sdcard_sendbyte(unsigned char value)
{
	unsigned char temp_value = value;

	for (unsigned char i=0; i<8; i++)
	{
		if (temp_value >= 0x80)
		{
			RC5 = 1; // MOSI is high
		}
		else
		{
			RC5 = 0; // MOSI is low
		}

		temp_value = temp_value << 1;

		sdcard_toggle();
	}
}

unsigned char sdcard_receivebyte(void)
{
	unsigned char temp_value = 0x00;

	for (unsigned char i=0; i<8; i++)
	{
		temp_value = temp_value << 1;

		if (RC4) // if MISO is high...
		{
			temp_value += 0x01;
		}

		sdcard_toggle();
	}

	return temp_value;
}

unsigned char sdcard_waitresult(void)
{
	unsigned char temp_value = 0xFF;

	for (unsigned int i=0; i<2048; i++) // arbitrary wait time
	{
		temp_value = sdcard_receivebyte();

		if (temp_value != 0xFF)
		{
			return temp_value;
		}
	}

	return 0xFF;
}

void sdcard_pump(void)
{
	RC2 = 1; // CS is high, must disable the device
	RC5 = 1; // MOSI is high, AND leave mosi high!!!

	sdcard_longdelay();

	for (unsigned char i=0; i<80; i++)
	{
		sdcard_toggle();
	}
}

unsigned char sdcard_initialize(void)
{
	unsigned char temp_value = 0x00;

	RC2 = 1; // CS is high
	RC5 = 0; // MOSI is low
	RC3 = 0; // CLK is low

	sdcard_disable();
	sdcard_pump();
	sdcard_longdelay();
	sdcard_enable();
	sdcard_sendbyte(0x40); // CMD0 = 0x40 + 0x00 (0 in hex)
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x95); // CRC for CMD0
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	sdcard_disable();
	if (temp_value != 0x01) { return 0; } // expecting 0x01
	sdcard_longdelay();
	sdcard_pump();
	sdcard_enable();
	sdcard_sendbyte(0x48); // CMD8 = 0x40 + 0x08 (8 in hex)
	sdcard_sendbyte(0x00); // CMD8 needs 0x000001AA argument
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x01);
	sdcard_sendbyte(0xAA); 
	sdcard_sendbyte(0x87); // CRC for CMD8
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	sdcard_disable();
	if (temp_value != 0x01) { return 0; } // expecting 0x01
	sdcard_enable();
	temp_value = sdcard_receivebyte(); // 32-bit return value, ignore
	temp_value = sdcard_receivebyte();
	temp_value = sdcard_receivebyte();
	temp_value = sdcard_receivebyte();
	sdcard_disable();
	do {
		sdcard_pump();
		sdcard_longdelay();
		sdcard_enable();
		sdcard_sendbyte(0x77); // CMD55 = 0x40 + 0x37 (55 in hex)
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x01); // CRC (general)
		temp_value = sdcard_waitresult(); // command response
		if (temp_value == 0xFF) { return 0; }
		sdcard_disable();
		if (temp_value != 0x01) { return 0; } // expecting 0x01
		sdcard_pump();
		sdcard_longdelay();
		sdcard_enable();
		sdcard_sendbyte(0x69); // CMD41 = 0x40 + 0x29 (41 in hex)
		sdcard_sendbyte(0x40); // needed for CMD41?
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x01); // CRC (general)
		temp_value = sdcard_waitresult(); // command response
		if (temp_value == 0xFF) { return 0; }
		sdcard_disable();
		if (temp_value != 0x00 && temp_value != 0x01) { return 0; } // expecting 0x00, if 0x01 try again
		sdcard_longdelay();
	} while (temp_value == 0x01);

	sdcard_ready = 0x01;

	return 1;
}

unsigned char sdcard_readblock(unsigned char high, unsigned char mid, unsigned char low)
{
	unsigned char temp_value = 0x00;

	sdcard_disable();
	sdcard_pump();
	sdcard_longdelay();
	sdcard_enable();
	sdcard_sendbyte(0x51); // CMD17 = 0x40 + 0x11 (17 in hex)
	sdcard_sendbyte(high);
	sdcard_sendbyte(mid);
	sdcard_sendbyte((low&0xFE)); // only blocks of 512 bytes
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x01); // CRC (general)
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	else if (temp_value != 0x00) { return 0; } // expecting 0x00
	temp_value = sdcard_waitresult(); // data packet starts with 0xFE
	if (temp_value == 0xFF) { return 0; }
	else if (temp_value != 0xFE) { return 0; }
	for (unsigned int i=0; i<512; i++) // packet of 512 bytes
	{
		temp_value = sdcard_receivebyte();
		
		// send temp_value to VIA
		shift_out(temp_value);
	}
	temp_value = sdcard_receivebyte(); // data packet ends with 0x55 then 0xAA
	temp_value = sdcard_receivebyte(); // ignore here
	sdcard_disable();

	return 1;
}

unsigned char sdcard_writeblock(unsigned char high, unsigned char mid, unsigned char low)
{
	unsigned char temp_value = 0x00;

	sdcard_disable();
	sdcard_pump();
	sdcard_longdelay();
	sdcard_enable();
	sdcard_sendbyte(0x58); // CMD24 = 0x40 + 0x18 (24 in hex)
	sdcard_sendbyte(high);
	sdcard_sendbyte(mid);
	sdcard_sendbyte((low&0xFE)); // only blocks of 512 bytes
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x01); // CRC (general)
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	else if (temp_value != 0x00) { return 0; } // expecting 0x00
	sdcard_sendbyte(0xFE); // data packet starts with 0xFE
	for (unsigned int i=0; i<512; i++) // packet of 512 bytes
	{
		// receive temp_value from VIA
		temp_value = shift_in();

		sdcard_sendbyte(temp_value);
	}
	sdcard_sendbyte(0x55); // data packet ends with 0x55 then 0xAA
	sdcard_sendbyte(0xAA);
	temp_value = sdcard_receivebyte(); // toggle clock 8 times
	sdcard_disable();

	return 1;
}

// Interrupt on falling edge of RDY (CPU has turned off all sources of /IRQ interrupts),
// in response the PIC interrupts on /NMI. 
// PIC will control SHIFT-CLOCK and receive on SHIFT-DATA
// and then send back on SHIFT-DATA
// UNFORTUNATELY, the VIA has a bug in mode's 011 and 111,
// which are exactly what is needed for this to work!  
// http://forum.6502.org/viewtopic.php?p=2310#p2310
// If CB1 edge is close to the PHI2 falling edge, it will be ignored.
// To get around this, the CPLD will make sure the edge is on PHI2 rising or something.
// Thus SHIFT-CLOCK-PIC is always an output from PIC into CPLD only, and CPLD re-routes it to SHIFT-CLOCK-VIA.
// When SDcard addresses are sent, the 1 bytes will be sent back to VIA, 0x00 for "no sdcard", else is good to go
// If good to go, reading requires 512 bytes sent to VIA, or writing requires 512 bytes sent from VIA.
void isr(void) __interrupt
{
	unsigned char command = 0x00;
	unsigned char value = 0x00;
	unsigned int counter = 0x00;

	if (INTF) // command ready
	{
		while (!RB6) // wait for /NMI to be high to begin
		{
			// do nothing
		}

		nmi_ground(); // interrupt while RDY is low

		command = 0xFF; // non-zero value

		command = shift_in(); // shift in command from VIA
	
		if (command == 0x00 || command == 0x02 || command == 0x03) // if command is for SD card...
		{
			delay();
	
			if (command == 0x00) // we know to look at the first 256KB of memory
			{
				block_high = 0x00;
				block_mid = 0x00;
				block_low = 0x00;
			}
			else if (command == 0x02 || command == 0x03) // read in location from SD card
			{
				value = shift_in(); // shift in command from VIA
				block_high = value; // store as highest block value
			
				delay();
	
				value = shift_in(); // shift in command from VIA
				block_mid = value; // store as mid block value
		
				delay();
		
				value = shift_in(); // shift in command from VIA
				block_low = value; // store as low block value
		
				delay();
			}

			if (sdcard_ready == 0x00)
			{
				// tell VIA that it's not going to work
				shift_out(0x00);

				command = 0x03; // automatically switch to serial connection
			}
			else
			{
				// tell the VIA that it's ok
				shift_out(0xFF);
	
				if (command == 0x00) // if loading 256KB...
				{
					value = 0x00;

					do
					{
						do
						{
							sdcard_readblock(block_high,	
								block_mid, block_low); // return success?
						
							block_low = block_low + 2; // increment block low
						
							value = value + 1;
						}
						while (value != 0x00);

						command = command + 1;
					}
					while (command < 2); // load 512 blocks of 512 bytes each = 256KB total
				}
				else if (command == 0x02) // if reading 512 byte block...
				{
					sdcard_readblock(block_high, block_mid, block_low); // return success?
				}
				else if (command == 0x03) // if writing 512 byte block...
				{
					sdcard_writeblock(block_high, block_mid, block_low); // return success?
				}

				delay();

				command = 0x00;
			}
		}

		// do not put an 'else if' here!!!
		if (command == 0x01) // if command is for serial connection, loading 64KB...
		{
			serial_transmitchar('R');
			serial_transmitchar('d');
			serial_transmitchar('y');
			serial_transmitchar('\n');
			serial_transmitchar('\r'); // send message to computer

			delay();

			value = 0x00;

			counter = 0x0000;

			do
			{
				value = serial_receivechar(); // receive byte of data from computer
	
				shift_out(value); // send to VIA

				counter = counter + 1; // increment counter
			}
			while (counter != 0x0000); // do this for 64KB of data

			delay();
		}

		serial_transmitchar('C');
		serial_transmitchar('l');
		serial_transmitchar('r');
		serial_transmitchar('\n');
		serial_transmitchar('\r'); // send message to computer

		nmi_float(); // release /NMI to be pulled high

		INTF = 0; // clear flag
	}
	// if there were other interrupts, they would go here
}

// bootloader code that goes at 0x00FE00 to 0x00FEFF
const unsigned char codeA[256] = {
	0xCB,0x40,0xA9,0x04,0x8F,0x0E,0x00,0x01,0xA9,0x1C,0x8F,0x0B,0x00,0x01,0xA9,0x00,
	0x8F,0x0A,0x00,0x01,0xCB,0xA9,0x0C,0x8F,0x0B,0x00,0x01,0xCB,0xAF,0x0A,0x00,0x01,
	0xF0,0x23,0xCB,0xAF,0x0A,0x00,0x01,0x8F,0x00,0x00,0x04,0xEE,0x28,0xFE,0xD0,0xF2,
	0xEE,0x29,0xFE,0xD0,0xED,0xEE,0x2A,0xFE,0xAD,0x2A,0xFE,0xC9,0x08,0xF0,0xE3,0x18,
	0xFB,0x5C,0x00,0x00,0x04,0xA9,0x00,0x8F,0x03,0x00,0x01,0xA9,0x02,0x8F,0x0C,0x00,
	0x01,0x8F,0x0E,0x00,0x01,0x58,0x20,0xB7,0xFE,0xAE,0xE1,0xFF,0xEC,0xE0,0xFF,0xF0,
	0xF8,0xEE,0xE1,0xFF,0xBD,0x00,0x02,0xC9,0xF0,0xD0,0x05,0xEE,0xE1,0xFF,0x80,0xE9,
	0xC9,0x29,0xD0,0x05,0xEE,0xE3,0xFE,0x80,0xDD,0xC9,0x5A,0xD0,0x0B,0xAD,0xE3,0xFE,
	0x18,0x69,0x20,0x8D,0xE3,0xFE,0x80,0xCE,0xC9,0x66,0xD0,0x05,0xCE,0xE3,0xFE,0x80,
	0xC5,0xC9,0x76,0xF0,0x1D,0xA2,0x0F,0xDD,0x8D,0xFF,0xF0,0x03,0xCA,0xD0,0xF8,0xDA,
	0x20,0xE2,0xFE,0xB5,0x00,0x0A,0x0A,0x0A,0x0A,0x95,0x00,0x68,0x15,0x00,0x95,0x00,
	0x80,0xA4,0x20,0x00,0x00,0x80,0x9F,0xA0,0x04,0xA2,0x00,0xA5,0x00,0xDA,0x9C,0x45,
	0xFF,0x20,0xE2,0xFE,0xEC,0xBC,0xFE,0xD0,0x03,0xCE,0x45,0xFF,0xFA,0x20,0xE5,0xFE,
	0xEE,0xBC,0xFE,0x8A,0x29,0x3F,0xD0,0xE3,0xA2,0x00,0xC8,0xC8,0xAD,0xBC,0xFE,0xD0,
	0xDA,0x60,0xA2,0x00,0x60,0x48,0x48,0x29,0xF0,0x18,0x6A,0x6A,0x6A,0x6A,0x20,0xFB,
	0xFE,0xE8,0x68,0x29,0x0F,0x20,0xFB,0xFE,0xE8,0x68,0x60,0x48,0xDA,0x5A,0x48,0x8A
};

// bootloader code that goes at 0x00FF00 to 0x00FFFF
const unsigned char codeB[256] = {
	0x0A,0x8D,0x49,0xFF,0x98,0x0A,0x0A,0x0A,0x8D,0x4A,0xFF,0x68,0x0A,0x0A,0xAA,0xA0,
	0x04,0xBD,0x4D,0xFF,0x20,0x28,0xFF,0x20,0x33,0xFF,0x20,0x28,0xFF,0x20,0x33,0xFF,
	0xE8,0x88,0xD0,0xED,0x7A,0xFA,0x68,0x60,0x48,0x20,0x44,0xFF,0xEE,0x49,0xFF,0x68,
	0x0A,0x0A,0x60,0x48,0x20,0x44,0xFF,0xCE,0x49,0xFF,0xEE,0x4A,0xFF,0xEE,0x4A,0xFF,
	0x68,0x0A,0x0A,0x60,0x49,0x00,0x29,0xC0,0x8F,0x00,0x00,0x02,0x60,0xEA,0xAA,0xE0,
	0x00,0x22,0x22,0x20,0x00,0xE2,0xE8,0xE0,0x00,0xE2,0xE2,0xE0,0x00,0xAA,0xE2,0x20,
	0x00,0xE8,0xE2,0xE0,0x00,0xE8,0xEA,0xE0,0x00,0xE2,0x22,0x20,0x00,0xEA,0xEA,0xE0,
	0x00,0xEA,0xE2,0xE0,0x00,0xEA,0xEA,0xA0,0x00,0x88,0xEA,0xE0,0x00,0xE8,0x88,0xE0,
	0x00,0x22,0xEA,0xE0,0x00,0xE8,0xE8,0xE0,0x00,0xE8,0xE8,0x80,0x00,0x45,0x16,0x1E,
	0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,0x48,0xAF,0x01,
	0x00,0x01,0x29,0x80,0x18,0x6E,0xE2,0xFF,0x18,0x6D,0xE2,0xFF,0x8D,0xE2,0xFF,0xEE,
	0xE4,0xFF,0xAD,0xE4,0xFF,0xC9,0x09,0xD0,0x12,0xAD,0xE2,0xFF,0xDA,0xAE,0xE0,0xFF,
	0x9D,0x00,0x02,0x8A,0x1A,0x8D,0xE0,0xFF,0xFA,0x68,0x40,0xC9,0x0B,0xF0,0x02,0x68,
	0x40,0x9C,0xE4,0xFF,0x68,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x01,0xFE,0x01,0xFE,0x01,0xFE,0x01,0xFE,0x00,0x00,0x01,0xFE,
	0x00,0x00,0x00,0x00,0x01,0xFE,0x00,0x00,0x01,0xFE,0x02,0xFE,0x00,0xFE,0x9D,0xFF
};

// initial boot sequence, only happens when /MCLR is low (power on or reset button pressed)
void boot(void)
{
	unsigned char wait = 0x00; // ready to send WAI command
	unsigned char count = 0x00; // low addr and cycle counter
	unsigned char loop = 0x01; // loop flag
	unsigned char state = 0x00; // grabs state of PORTB
	unsigned char pull = 0x00; // only /VP from state
	unsigned char read = 0x00; // changes between LDA# and STAa statements, start 0x00 for write
	unsigned char low = 0x00; // changes between low and high addresses, start 0x00 for high
	unsigned char second = 0x00; // switches between which 256 byte code is used

	// initial setup (if unused, set as output and drive low)
	// PHI2 is low, /RES is low, /NMI is low, SHIFT-CLOCK is input, SHIFT-DATA is input, TX/CK is high and RX/DT is input
	TRISA = 0x00;
	PORTA = 0x00;
	TRISB = 0x3D;
	PORTB = 0x00;
	TRISC = 0x93;
	PORTC = 0x64;

	// only needed for PIC16F887, comment out otherwise
	//TRISD = 0x00;
	//PORTD = 0x00;
	//TRISE = 0x00;
	//PORTE = 0x00;

	serial_initialize(); // initialize serial registers

	serial_transmitchar('B');
	serial_transmitchar('o');
	serial_transmitchar('o');
	serial_transmitchar('t');
	serial_transmitchar('\n');
	serial_transmitchar('\r'); // send message to computer

	for (unsigned char i=0; i<10; i++) // cycle phi2 10 times while /RES is low
	{
		toggle(); // toggle phi2
	}

	TRISB = TRISB | 0x80; // release /RES, pulled high

	while (loop == 0x01) // loop until told to stop
	{
		state = PORTB & 0x3C; // get state of /RW, /VP, VDA, and VPA

		pull = state & 0x08; // separate /VP
		
		if (pull == 0x00) // if pulling reset vectors
		{
			PORTA = 0x00; // load fake reset vectors at $0000
			toggle(); // toggle phi2
		}
		else if (wait == 0x00) // if not sending WAI yet
		{
			if (state == 0x3C) // if /RW is high, VDA is high, and VPA is high...
			{
				read = 0x01 - read; // change next instruction type
	
				if (read == 0x01) // reading, thus LDA#
				{
					PORTA = 0xA9; // LDA# on data bus
					toggle(); // toggle phi2
				}
				else // writing, thus STAa
				{
					PORTA = 0x8D; // STAa on data bus
					toggle(); // toggle phi2
				}
			}
			else if (state == 0x2C) // if /RW is high, VDA is low, and VPA is high...
			{
				if (read == 0x01) // reading, thus LDA#
				{
					if (second == 0x00) // use first 256 byte set of code
					{
						PORTA = codeA[count]; // code on data bus
					}
					else // use second 256 byte set of code
					{
						PORTA = codeB[count]; // code on data bus
					}
					toggle(); // toggle phi2
				}
				else // writing, thus STAa
				{
					low = 0x01 - low; // change next addr type
	
					if (low == 0x01) // low addr
					{
						PORTA = count; // low addr on data bus
						toggle(); // toggle phi2
					}
					else // high addr
					{
						PORTA = 0xFF; // high addr on data bus
						toggle(); // toggle phi2
					}
				}
			}
			else if (state == 0x18) // if /RW is low, VDA is high, and VPA is low...
			{
				if (read == 0x01) // reading, thus LDA#
				{
					serial_error(); // error msg and infinite loop
				}
				else // writing, thus STAa
				{
					TRISA = 0xFF; // release data bus
					toggle(); // toggle phi2
					TRISA = 0x00; // drive data bus
	
					count = count + 1; // increment low addr
	
					if (count == 0x00) // if 256 bytes written...
					{
						second = second + 1; // switch to next code set
		
						if (second == 0x02) // only two banks, when done...
						{
							wait = 0x01; // get ready to send WAI command
						}
					}
				}
			}	
			else // if in an unexpected state...
			{
				serial_error(); // error msg and infinite loop
			}
		}
		else if (wait == 0x01) // if ready to send WAI command...
		{
			if (state == 0x3C) // if /RW is high, VDA is high, and VPA is high...
			{
				PORTA = 0xCB; // WAI on data bus
				toggle(); // toggle phi2
			}
			else if (state == 0x0C) // if /RW is high, VDA is low, and VPA is low...
			{
				PORTA = 0x00; // arbitrary value on data bus
				toggle(); // toggle phi2
			}
			else // if in an unexpected state...
			{
				serial_error(); // error msg and infinite loop
			}

			count = count + 1; // increment cycle count

			if (count > 0x03) // after about three cycles...
			{
				loop = 0x00; // end loop
			}
		}
		else // if extra cycle for some reason...
		{
			serial_error(); // error msg and infinite loop
		}
	}
	
	TRISA = 0xFF; // release data bus
	TRISB = TRISB | 0x02; // release PHI2
	RB7 = 0; // get /RES ready for low
	TRISB = TRISB & 0x7F; // /RES is output
	TRISB = TRISB | 0x40; // release /NMI, pulled high, CPLD goes to normal operation

	serial_transmitchar('I');
	serial_transmitchar('n');
	serial_transmitchar('i');
	serial_transmitchar('t');
	serial_transmitchar('\n');
	serial_transmitchar('\r'); // send message to computer

	sdcard_ready = 0x00; // SDcard not yet ready
	for (unsigned char i=0; i<5; i++) // try 5 times to initialize
	{
		if (sdcard_initialize()) break; // break if it's initialized
	}

	OPTION_REG = 0x87; // sets up INT edge
	INTCON = 0xD0; // sets up INT interrupt and clears flags
	PIE1 = 0x00; // no other interrupts
	PIE2 = 0x00; // no other interrupts

	TRISB = TRISB | 0x80; // release /RES, pulled high, ready to go!

	serial_transmitchar('D');
	serial_transmitchar('o');
	serial_transmitchar('n');
	serial_transmitchar('e');
	serial_transmitchar('\n');
	serial_transmitchar('\r'); // send message to computer
}

// main program, send interrupts on /NMI periodically and controls audio
void main(void)
{
	boot(); // boot sequence

	while (1) // loop forever
	{
		// wait for interrupts?
	}
}










