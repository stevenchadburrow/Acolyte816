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
RB2 = /BOOT, output, tells CPLD and transceiver to stay in bootloader mode
RB3 = /VP, input, used to sense CPU status in boot
RB4 = VPA, input, used to sense CPU status in boot
RB5 = VDA, input, used to sense CPU status in boot
RB6 = /NMI, inout, grounded on regular timed intervals, floating (pulled high) when not
RB7 = /RES, inout, grounded upon start of boot, then grounded to exit boot

RC0 = SPI-CLK, output, connects to SDcard
RC1 = SPI-MOSI, output, connects to SDcard
RC2 = SPI-CS, input, connects to SDcard
RC3 = SPI-MISO, output, connects to SDcard
RC4 = DSR, input, for possible FT232RL
RC5 = RTS, output, for possible FT232RL
RC6 = TX/CK, output, for possible FT232RL
RC7 = RX/DT, input, for possible FT232RL

RE3 = /MCLR, master clear
*/

/*
The boot sequence is as follows:
	Bring /RES low and gain control of PHI2.
	Release /RES to be pulled high, start single stepping CPU.
	When /VP goes low, send $0000 reset vector (arbitrary).
	Send LDA# and STAal instructions, filling $FE00-$FFFF with boot code.
	If SDcard is ready, send 128KB of data as well.
	When done, send WAI instruction to bring RDY low.
	Release PHI2, bring /RES low again.
	Setup PIC interrupts.
	Then release /RES to start the CPU running!
*/

/*
The SDcard command sequence is as follows:
	CPU turns off /IRQ interrupts entirely. 
	VIA puts command on PA, and enters special copy sub-routine.
	CPU uses the WAI command, bringing RDY low.
	PIC interrupts on falling edge of RDY.
	PIC reads command then toggles /NMI, bringing RDY high.
	VIA has more command data, PIC waits for RDY to fall, repeat three more times.
	VIA then make PA as input and issues WAI command.
	PIC puts "SDcard Ready" on PA and toggles /NMI.
	If SDcard was not ready, exit sub-routine, else, expect 512 bytes of data (incoming or outgoing).
	PIC waits for RDY to fall, then either sends or receives data on PA, then toggles /NMI, repeat.
	...
	CPU exits sub-routine.
	CPU turns /IRQ interrupts back on.
*/
	


// must have the library file locally in the same folder
#include "pic16f886.h"
//#include "pic16f887.h"


// these are useless here, but keeping them here for 'posterity' sake
unsigned int __at _CONFIG1 config1 = 0x2CF4;
unsigned int __at _CONFIG2 config2 = 0x3EFF;


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

void rdy_wait(void)
{
	while (RB0 == 1) { } // wait for RDY to fall from WAI instruction
}

void nmi_ground(void)
{
	while (RB6 == 0) { } // wait for /NMI to be high
	RB6 = 0; // get /NMI ready for low
	TRISB = TRISB & 0xBF; // /NMI is output
}

void nmi_float(void)
{
	TRISB = TRISB | 0x60; // release /NMI, pulled high
}

// tells if the initialization process worked or not
unsigned char sdcard_ready = 0x00;

// blocks for SDcard
unsigned char sdcard_block_high = 0x00;
unsigned char sdcard_block_mid = 0x00;
unsigned char sdcard_block_low = 0x00;

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
	RC0 = 1; // CLK is high
	// delay?
	RC0 = 0; // CLK is low
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
			RC1 = 1; // MOSI is high
		}
		else
		{
			RC1 = 0; // MOSI is low
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

		if (RC3) // if MISO is high...
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
	RC1 = 1; // MOSI is high, AND leave mosi high!!!

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
	RC1 = 0; // MOSI is low
	RC0 = 0; // CLK is low

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

unsigned char sdcard_readinit(unsigned char high, unsigned char mid, unsigned char low)
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
	
	return 1;
}

unsigned char sdcard_readfinal(void)
{
	unsigned char temp_value = 0x00;

	temp_value = sdcard_receivebyte(); // data packet ends with 0x55 then 0xAA
	temp_value = sdcard_receivebyte(); // ignore here
	sdcard_disable();

	return 1;
}

unsigned char sdcard_readblock(unsigned char high, unsigned char mid, unsigned char low)
{
	unsigned char temp_value = 0x00;

	if (sdcard_readinit(high, mid, low) == 0) { sdcard_ready = 0x00; }

	TRISA = 0x00; // output on databus

	for (unsigned int i=0; i<512; i++) // packet of 512 bytes
	{
		// get value from SDcard
		temp_value = sdcard_receivebyte();
		
		// send temp_value to VIA
		PORTA = temp_value;

		// wait for RDY to fall from WAI
		rdy_wait();

		// toggle /NMI
		nmi_ground();
		nmi_float();
	}

	TRISA = 0xFF; // input on databus

	if (sdcard_readfinal() == 0) { sdcard_ready = 0x00; }

	if (sdcard_ready == 0x00) { return 0; }
	else { return 1; }
}

unsigned char sdcard_writeinit(unsigned char high, unsigned char mid, unsigned char low)
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

	return 1;
}

unsigned char sdcard_writefinal(void)
{
	unsigned char temp_value = 0x00;
	
	sdcard_sendbyte(0x55); // data packet ends with 0x55 then 0xAA
	sdcard_sendbyte(0xAA);
	temp_value = sdcard_receivebyte(); // toggle clock 8 times
	sdcard_disable();

	return 1;
}

unsigned char sdcard_writeblock(unsigned char high, unsigned char mid, unsigned char low)
{
	unsigned char temp_value = 0x00;

	if (sdcard_writeinit(high, mid, low) == 0) { sdcard_ready = 0x00; }

	// assumes TRISA = 0xFF already
	
	for (unsigned int i=0; i<512; i++) // packet of 512 bytes
	{
		// receive temp_value from VIA
		temp_value = PORTA;

		// send to SDcard
		sdcard_sendbyte(temp_value);

		// wait for RDY to fall from WAI
		rdy_wait();

		// toggle /NMI
		nmi_ground();
		nmi_float();
	}

	if (sdcard_writefinal() == 0) { sdcard_ready = 0x00; }

	if (sdcard_ready == 0x00) { return 0; }
	else { return 1; }
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
		// assumes TRISA = 0xFF already

		command = PORTA; // shift in command from VIA

		nmi_ground(); // interrupt while RDY is low by toggling /NMI
		nmi_float();
	
		if (command == 0x00) // empty command
		{
			// do nothing
		}
		else if (command == 0x01) // if command is for serial connection, loading 64KB...
		{
			serial_transmitchar('R');
			serial_transmitchar('d');
			serial_transmitchar('y');
			serial_transmitchar('\n');
			serial_transmitchar('\r'); // send message to computer

			// VIA needs to make databus input here

			rdy_wait(); // wait for RDY to fall from WAI
			
			nmi_ground(); // toggle /NMI
			nmi_float();

			value = 0x00;

			counter = 0x0000;

			TRISA = 0x00; // output on databus

			do
			{
				value = serial_receivechar(); // receive byte of data from computer
	
				PORTA = value; // send to VIA
	
				rdy_wait(); // wait for RDY to fall from WAI

				nmi_ground(); // toggle /NMI
				nmi_float();

				counter = counter + 1; // increment counter
			}
			while (counter != 0x0000); // do this for 64KB of data

			TRISA = 0xFF; // input on databus
	
			serial_transmitchar('C');
			serial_transmitchar('l');
			serial_transmitchar('r');
			serial_transmitchar('\n');
			serial_transmitchar('\r'); // send message to computer
		}
		else if (command == 0x02 || command == 0x03) // if command is for SD card...
		{
			rdy_wait(); // wait for RDY to fall from WAI
			
			value = PORTA; // get value from VIA on databus
			sdcard_block_high = value; // store as highest block value

			nmi_ground(); // toggle /NMI
			nmi_float();
	
			rdy_wait(); // wait for RDY to fall from WAI
			
			value = PORTA; // get value from VIA on databus
			sdcard_block_mid = value; // store as mid block value

			nmi_ground(); // toggle /NMI
			nmi_float();			

			rdy_wait(); // wait for RDY to fall from WAI
			
			value = PORTA; // get value from VIA on databus
			sdcard_block_low = value; // store as low block value

			nmi_ground(); // toggle /NMI
			nmi_float();

			// VIA needs to make databus input here

			if (sdcard_ready == 0x00) // if SDcard is not ready
			{
				for (unsigned char i=0; i<5; i++) // try 5 times to initialize
				{
					if (sdcard_initialize()) break; // break if it's initialized
				}
			}

			if (sdcard_ready == 0x00) // if SDcard is still not ready
			{
				rdy_wait(); // wait for RDY to fall from WAI
			
				PORTA = 0x00; // tell VIA that it's not going to work

				nmi_ground(); // toggle /NMI
				nmi_float();
			}
			else
			{
				rdy_wait(); // wait for RDY to fall from WAI
			
				PORTA = 0xFF; // tell VIA that it's ok

				nmi_ground(); // toggle /NMI
				nmi_float();

				TRISA = 0xFF; // input on databus
	
				if (command == 0x02) // if reading 512 byte block...
				{
					sdcard_readblock(sdcard_block_high, sdcard_block_mid, sdcard_block_low); // return success?
				}
				else if (command == 0x03) // if writing 512 byte block...
				{
					sdcard_writeblock(sdcard_block_high, sdcard_block_mid, sdcard_block_low); // return success?
				}
			}
		}

		INTF = 0; // clear flag
	}
	// if there were other interrupts, they would go here
}

// bootloader code that goes at 0x00FE00 to 0x00FEFF
const unsigned char codeA[256] = {
	0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
	0x80,0x01,0x40,0xA9,0x00,0x8F,0x03,0x00,0x01,0xA9,0x02,0x8F,0x0C,0x00,0x01,0x8F,
	0x0E,0x00,0x01,0x58,0x20,0x85,0xFE,0xAE,0xE1,0xFF,0xEC,0xE0,0xFF,0xF0,0xF8,0xEE,
	0xE1,0xFF,0xBD,0x00,0x02,0xC9,0xF0,0xD0,0x05,0xEE,0xE1,0xFF,0x80,0xE9,0xC9,0x29,
	0xD0,0x05,0xEE,0xB1,0xFE,0x80,0xDD,0xC9,0x5A,0xD0,0x0B,0xAD,0xB1,0xFE,0x18,0x69,
	0x20,0x8D,0xB1,0xFE,0x80,0xCE,0xC9,0x66,0xD0,0x05,0xCE,0xB1,0xFE,0x80,0xC5,0xC9,
	0x76,0xF0,0x1D,0xA2,0x0F,0xDD,0x5B,0xFF,0xF0,0x03,0xCA,0xD0,0xF8,0xDA,0x20,0xB0,
	0xFE,0xB5,0x00,0x0A,0x0A,0x0A,0x0A,0x95,0x00,0x68,0x15,0x00,0x95,0x00,0x80,0xA4,
	0x20,0x00,0x00,0x80,0x9F,0xA0,0x04,0xA2,0x00,0xA5,0x00,0xDA,0x9C,0x13,0xFF,0x20,
	0xB0,0xFE,0xEC,0x8A,0xFE,0xD0,0x03,0xCE,0x13,0xFF,0xFA,0x20,0xB3,0xFE,0xEE,0x8A,
	0xFE,0x8A,0x29,0x3F,0xD0,0xE3,0xA2,0x00,0xC8,0xC8,0xAD,0x8A,0xFE,0xD0,0xDA,0x60,
	0xA2,0x00,0x60,0x48,0x48,0x29,0xF0,0x18,0x6A,0x6A,0x6A,0x6A,0x20,0xC9,0xFE,0xE8,
	0x68,0x29,0x0F,0x20,0xC9,0xFE,0xE8,0x68,0x60,0x48,0xDA,0x5A,0x48,0x8A,0x0A,0x8D,
	0x17,0xFF,0x98,0x0A,0x0A,0x0A,0x8D,0x18,0xFF,0x68,0x0A,0x0A,0xAA,0xA0,0x04,0xBD,
	0x1B,0xFF,0x20,0xF6,0xFE,0x20,0x01,0xFF,0x20,0xF6,0xFE,0x20,0x01,0xFF,0xE8,0x88,
	0xD0,0xED,0x7A,0xFA,0x68,0x60,0x48,0x20,0x12,0xFF,0xEE,0x17,0xFF,0x68,0x0A,0x0A
};

// bootloader code that goes at 0x00FF00 to 0x00FFFF
const unsigned char codeB[256] = {
	0x60,0x48,0x20,0x12,0xFF,0xCE,0x17,0xFF,0xEE,0x18,0xFF,0xEE,0x18,0xFF,0x68,0x0A,
	0x0A,0x60,0x49,0x00,0x29,0xC0,0x8F,0x00,0x00,0x02,0x60,0xEA,0xAA,0xE0,0x00,0x22,
	0x22,0x20,0x00,0xE2,0xE8,0xE0,0x00,0xE2,0xE2,0xE0,0x00,0xAA,0xE2,0x20,0x00,0xE8,
	0xE2,0xE0,0x00,0xE8,0xEA,0xE0,0x00,0xE2,0x22,0x20,0x00,0xEA,0xEA,0xE0,0x00,0xEA,
	0xE2,0xE0,0x00,0xEA,0xEA,0xA0,0x00,0x88,0xEA,0xE0,0x00,0xE8,0x88,0xE0,0x00,0x22,
	0xEA,0xE0,0x00,0xE8,0xE8,0xE0,0x00,0xE8,0xE8,0x80,0x00,0x45,0x16,0x1E,0x26,0x25,
	0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,0x48,0xAF,0x01,0x00,0x01,
	0x29,0x80,0x18,0x6E,0xE2,0xFF,0x18,0x6D,0xE2,0xFF,0x8D,0xE2,0xFF,0xEE,0xE4,0xFF,
	0xAD,0xE4,0xFF,0xC9,0x09,0xD0,0x12,0xAD,0xE2,0xFF,0xDA,0xAE,0xE0,0xFF,0x9D,0x00,
	0x02,0x8A,0x1A,0x8D,0xE0,0xFF,0xFA,0x68,0x40,0xC9,0x0B,0xF0,0x02,0x68,0x40,0x9C,
	0xE4,0xFF,0x68,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x12,0xFE,0x12,0xFE,0x12,0xFE,0x12,0xFE,0x00,0x00,0x12,0xFE,
	0x00,0x00,0x00,0x00,0x12,0xFE,0x00,0x00,0x12,0xFE,0x12,0xFE,0x00,0xFE,0x6B,0xFF
};

// spoon feed 16 bytes directly into CPU using LDA# and STAal commands
// if exit != 0x00, then the WAI command is issued
void load(unsigned char packet[16], unsigned char addr_bank, unsigned char addr_high, unsigned char addr_low, unsigned char exit)
{
	unsigned char wait = 0x00; // ready to send WAI command
	unsigned char count = 0x00; // low addr and cycle counter
	unsigned char loop = 0x01; // loop flag
	unsigned char state = 0x00; // grabs state of PORTB
	unsigned char pull = 0x00; // only /VP from state
	unsigned char read = 0x00; // changes between LDA# and STAa statements, start 0x00 for write
	unsigned char loc = 0x03; // changes between low, high, and bank addresses, start 0x03 for low

	while (loop == 0x01) // loop until told to stop
	{
		state = PORTB & 0x38; // get state of /VP, VDA, and VPA

		pull = state & 0x08; // separate /VP
		
		if (pull == 0x00) // if pulling reset vectors
		{
			PORTA = 0x00; // load fake reset vectors at $0000
			toggle(); // toggle phi2
		}
		else if (wait == 0x00) // if not sending WAI yet
		{
			if (state == 0x38) // if VDA is high, and VPA is high...
			{
				read = 0x01 - read; // change next instruction type
	
				if (read == 0x01) // reading, thus LDA#
				{
					PORTA = 0xA9; // LDA# on data bus
					toggle(); // toggle phi2
				}
				else // writing, thus STAal
				{
					PORTA = 0x8F; // STAal on data bus
					toggle(); // toggle phi2
				}
			}
			else if (state == 0x28) // if VDA is low, and VPA is high...
			{
				if (read == 0x01) // reading, thus LDA#
				{
					PORTA = packet[count]; // code on data bus
					toggle(); // toggle phi2
				}
				else // writing, thus STAa
				{
					loc = loc - 0x01; // change next addr type
	
					if (loc == 0x02) // low addr
					{
						PORTA = addr_low + count; // low addr on data bus
						toggle(); // toggle phi2
					}
					else if (loc == 0x01) // high addr
					{
						PORTA = addr_high; // high addr on data bus
						toggle(); // toggle phi2
					}
					else if (loc == 0x00) // bank addr
					{
						PORTA = addr_bank; // high addr on data bus
						toggle(); // toggle phi2

						loc = 0x03; // go back to beginning
					}
				}
			}
			else if (state == 0x18) // if VDA is high, and VPA is low...
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
	
					if (count == 0x10) // if 16 bytes written...
					{
						wait = exit; // get ready to send WAI command

						if (exit == 0x00) // not done loading
						{
							loop = 0x00; // but exit loop for next 16 bytes
						}
					}
				}
			}	
			else // could be reset vectors taking a while to work, or could be an error!
			{
				PORTA = 0x00; // load $00
				toggle(); // toggle phi2
			}
		}
		else if (wait != 0x00) // if ready to send WAI command...
		{
			if (state == 0x38) // if VDA is high, and VPA is high...
			{
				PORTA = 0xCB; // WAI on data bus
				toggle(); // toggle phi2
			}
			else if (state == 0x08) // if VDA is low, and VPA is low...
			{
				PORTA = 0x00; // arbitrary value on data bus
				toggle(); // toggle phi2
			}
			else // if in an unexpected state...
			{
				serial_error(); // error msg and infinite loop
			}

			count = count + 1; // increment cycle count

			if (count > 0x05) // after about five cycles...
			{
				loop = 0x00; // end loop
			}
		}
	}
}

// initial boot sequence, only happens when /MCLR is low (power on or reset button pressed)
unsigned char boot(unsigned char jump)
{
	unsigned char addr_low = 0x00;
	unsigned char addr_high = 0xFE;
	unsigned char addr_bank = 0x00;

	unsigned char packet[16];
	unsigned char packet_loc = 0x00;

	// initial setup (if unused, set as output and drive low)
	// PHI2 is low, /BOOT is low, /RES is low, /NMI is input, TX/CK is high and RX/DT is input
	TRISA = 0x00;
	PORTA = 0x00;
	TRISB = 0x79;
	PORTB = 0x00;
	TRISC = 0x98;
	PORTC = 0x64;

	// only needed for PIC16F887, comment out otherwise
	//TRISD = 0x00;
	//PORTD = 0x00;
	//TRISE = 0x00;
	//PORTE = 0x00;

	serial_initialize(); // initialize serial registers

	sdcard_ready = 0x00; // SDcard not yet ready
	for (unsigned char i=0; i<5; i++) // try 5 times to initialize
	{
		if (sdcard_initialize()) break; // break if it's initialized
	}

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

	for (unsigned char i=0; i<2; i++) // 2 sets of 16 packets = 512 bytes
	{
		for (unsigned char j=0; j<16; j++) // 16 packets of 16 bytes each
		{
			if (i == 0 && j == 0) // first 16 byte packet
			{
				if (jump != 0x00) // if jumping, load jump instruction
				{
					packet[0] = 0x5C; // JMPal
					packet[1] = 0x00;
					packet[2] = 0x00;
					packet[3] = 0x04; // jump location of $040000
					
					for (unsigned char k=4; k<16; k++) // 12 bytes remaining
					{
						packet[k] = 0xEA; // fill with NOP
					}
				}
				else // if not jumping, fill packet with NOP
				{
					for (unsigned char k=0; k<16; k++) // 16 bytes per packet
					{
						packet[k] = 0xEA; // if not jumping, fill first packet with NOP
					}
				}
			}
			else
			{
				for (unsigned char k=0; k<16; k++) // 16 bytes per packet
				{
					if (i == 0)
					{
						packet[k] = codeA[packet_loc]; // first 256 bytes of bootloader code
					}
					else
					{
						packet[k] = codeB[packet_loc]; // second 256 bytes of bootloader code
					}
				
					packet_loc = packet_loc + 0x01; // increment packet location
				}
			}	

			if (i == 1 && j == 15 && sdcard_ready == 0x00) // if last packet and sdcard is not ready...
			{
				load(packet, addr_bank, addr_high, addr_low + packet_loc, 0x01); // exit loading process with WAI command
			}
			else
			{
				load(packet, addr_bank, addr_high, addr_low + packet_loc, 0x00); // else, keep loading
			}

			addr_low = addr_low + 16; // increment low address
		}

		addr_high = addr_high + 1; // increment high address
	}

	if (sdcard_ready != 0x00 && jump != 0x00) // if SDcard is ready and we are jumping...
	{
		serial_transmitchar('C');
		serial_transmitchar('a');
		serial_transmitchar('r');
		serial_transmitchar('d');
		serial_transmitchar('\n');
		serial_transmitchar('\r'); // send message to computer

		addr_low = 0x00; // start in bank $02 for video display
		addr_high = 0x00;
		addr_bank = 0x02;

		sdcard_block_high = 0x00; // start in block 0 on SDcard
		sdcard_block_mid = 0x00;
		sdcard_block_low = 0x00;

		while (addr_bank < 0x04) // go until bank $04 which fills all of the screen, and a bit extra
		{
			for (unsigned char i=0; i<128; i++) // 128 sets of 32 packets = 64KB
			{
				if (sdcard_readinit(sdcard_block_high, sdcard_block_mid, sdcard_block_low) == 0) // try to read from SDcard
				{
					sdcard_ready = 0x00;

					return 0; // exit with an error
				}

				sdcard_block_low = sdcard_block_low + 1; // increment block on SDcard
				
				if (sdcard_block_low == 0x00)
				{
					sdcard_block_high = sdcard_block_high + 1; // and increment higher block locations when need be
				}	

				for (unsigned char j=0; j<2; j++) // 2 pages per block on SDcard
				{
					for (unsigned char k=0; k<16; k++) // 16 packets of 16 bytes each
					{
						for (unsigned char l=0; l<16; l++) // 16 bytes per packet
						{
							packet[l] = sdcard_receivebyte(); // get byte from SDcard
						
							packet_loc = packet_loc + 0x01; // increment packet location
						}
		
						if (addr_bank == 0x07 && addr_high == 0xFF && k == 15) // if last packet and sdcard is not ready...
						{
							load(packet, addr_low + packet_loc, addr_high, addr_bank, 0x01); // exit loading process with WAI command
						}
						else
						{
							load(packet, addr_low + packet_loc, addr_high, addr_bank, 0x00); // else, keep loading
						}
	
						addr_low = addr_low + 16; // increment low address
					}
		
					addr_high = addr_high + 1; // increment high address
				}

				if (sdcard_readfinal() == 0) // try to finish reading
				{
					sdcard_ready = 0x00;

					return 0; // exit with an error
				}
			}
			
			addr_bank = addr_bank + 1; // increment bank address
		}
	}
	
	TRISA = 0xFF; // release data bus
	TRISB = TRISB | 0x02; // release PHI2
	RB7 = 0; // get /RES ready for low
	TRISB = TRISB & 0x7F; // /RES is output
	RB2 = 1; // have /BOOT go high, letting CPLD run normally

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

	return 1;
}

// main program, send interrupts on /NMI periodically and controls audio
void main(void)
{
	if (boot(0x01) == 0) // try to boot from SDcard
	{
		boot(0x00); // if that doesn't work, just do it normally
	}

	while (1) // loop forever
	{
		// wait for interrupts?
	}
}










