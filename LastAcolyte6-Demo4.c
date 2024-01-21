// LastAcolyte6 C code for PIC16F886
// Bootloader and I/O device

/*

cd ~/Projects/PIC16 ; ~/Projects/sdcc-4.3.0/bin/sdcc --use-non-free -mpic14 -p16f886 LastAcolyte6-Mini.c ; cd ~/Projects/minipro ; ./minipro -p "pic16f886@DIP28" -w ~/Projects/PIC16/LastAcolyte6-Mini.hex ; ./minipro -p "pic16f886@DIP28" -c config -w ~/Projects/PIC16/LastAcolyte6-Mini.fuses -e

cd ~/Projects/PIC16 ; ~/Projects/sdcc-4.3.0/bin/sdcc --use-non-free -mpic14 -p16f887 LastAcolyte6.c ; cd ~/Projects/minipro ; ./minipro -p "pic16f887@DIP40" -w ~/Projects/PIC16/LastAcolyte6.hex ; ./minipro -p "pic16f887@DIP40" -c config -w ~/Projects/PIC16/LastAcolyte6.fuses -e

Use 'picocom' for the FTDI port:

sudo picocom /dev/ttyUSB0

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
unsigned int __at _CONFIG1 config1 = 0x20F4; // 0x2CF4
unsigned int __at _CONFIG2 config2 = 0x0000; // 0x3EFF


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
	// delay needed?
	//for (unsigned int i=0; i<1000; i++) { }

	// assumes PHI2 is low already
	RB1 = 1; // bring PHI2 high

	// delay needed?
	//for (unsigned int i=0; i<1000; i++) { }

	RB1 = 0; // bring PHI2 low
}

void serial_initialize(void)
{
	unsigned char value = 0x00;

	TRISC = TRISC | 0x80; // make sure RX is input
	TRISC = TRISC & 0x40; // make sure TX is output
		
	SPBRG = 51; // 8000000 / (64 * (9600 + 1)); // formula set to baud rate at 9600 with 8 MHz clock
	//SPBRG = 25; // 4000000 / (64 * (9600 + 1)); // formula to set baud rate at 9600 with 4 MHz clock
	//SPBRGH = 0;
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

void serial_transmithex(unsigned char data)
{
	if ((data & 0xF0) > 0x90)
	{
		serial_transmitchar((char)('A' - 0x0A + (data >> 4)));
	}
	else
	{
		serial_transmitchar((char)('0' + (data >> 4)));
	}
	
	if ((data & 0x0F) > 0x09)
	{
		serial_transmitchar((char)('A' - 0x0A + (data & 0x0F)));
	}
	else
	{
		serial_transmitchar((char)('0' + (data & 0x0F)));
	}
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



// bootloader code that goes at 0x00FD00 to 0x00FDFF
const unsigned char codeA[256] = {
0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0x80,0x01,0x40,0xA9,0x00,0x8F,0x02,
0x00,0x01,0x8F,0x0B,0x00,0x01,0x8F,0x0C,0x00,0x01,0xA9,0x82,0x8F,0x0E,0x00,0x01,
0x58,0xA9,0x00,0x20,0x48,0xFF,0xEE,0x49,0xFF,0xD0,0xF8,0xAE,0x4B,0xFF,0xE0,0x03,
0xD0,0x0C,0xAE,0x4A,0xFF,0xE0,0xE0,0xD0,0x05,0xCE,0x4B,0xFF,0x80,0x0A,0xEE,0x4A,
0xFF,0xD0,0xE0,0xEE,0x4B,0xFF,0x80,0xDB,0x20,0xB7,0xFE,0xAE,0xDD,0xFF,0xEC,0xDC,
0xFF,0xF0,0xF8,0xEE,0xDD,0xFF,0xAD,0xE5,0xFE,0x9C,0xE5,0xFE,0xD0,0xED,0xBD,0x00,
0xFD,0xC9,0xF0,0xD0,0x05,0xEE,0xE5,0xFE,0x80,0xE1,0xC9,0x29,0xD0,0x05,0xEE,0xE3,
0xFE,0x80,0xD5,0xC9,0x5A,0xD0,0x0B,0xAD,0xE3,0xFE,0x18,0x69,0x10,0x8D,0xE3,0xFE,
0x80,0xC6,0xC9,0x66,0xD0,0x05,0xCE,0xE3,0xFE,0x80,0xBD,0xC9,0x76,0xF0,0x1D,0xA2,
0x0F,0xDD,0x8D,0xFF,0xF0,0x03,0xCA,0xD0,0xF8,0xDA,0x20,0xE2,0xFE,0xB5,0x00,0x0A,
0x0A,0x0A,0x0A,0x95,0x00,0x68,0x15,0x00,0x95,0x00,0x80,0x9C,0xAD,0xE3,0xFE,0x8D,
0xB3,0xFE,0x20,0x00,0x00,0x80,0x91,0xA0,0x00,0xA2,0x00,0xA5,0x00,0xDA,0x9C,0x45,
0xFF,0x20,0xE2,0xFE,0xEC,0xBC,0xFE,0xD0,0x03,0xCE,0x45,0xFF,0xFA,0x20,0xE6,0xFE,
0xEE,0xBC,0xFE,0x8A,0x29,0x0F,0xD0,0xE3,0xA2,0x00,0xC8,0xC8,0xAD,0xBC,0xFE,0xD0,
0xDA,0x60,0xA2,0x00,0x60,0x00,0x48,0x48,0x29,0xF0,0x18,0x6A,0x6A,0x6A,0x6A,0x20,
0xFD,0xFE,0xE8,0x68,0x29,0x0F,0x20,0xFD,0xFE,0xE8,0xE8,0x68,0x60,0x48,0xDA,0x5A
};

// bootloader code that goes at 0x00FE00 to 0x00FEFF
const unsigned char codeB[256] = {
0x48,0x8A,0x0A,0x8D,0x49,0xFF,0x98,0x0A,0x0A,0x0A,0x8D,0x4A,0xFF,0x68,0x0A,0x0A,
0xAA,0xA0,0x04,0xBD,0x4D,0xFF,0x20,0x2A,0xFF,0x20,0x33,0xFF,0x20,0x2A,0xFF,0x20,
0x33,0xFF,0xE8,0x88,0xD0,0xED,0x7A,0xFA,0x68,0x60,0x48,0x20,0x44,0xFF,0xEE,0x49,
0xFF,0x80,0x0D,0x48,0x20,0x44,0xFF,0xCE,0x49,0xFF,0xEE,0x4A,0xFF,0xEE,0x4A,0xFF,
0x68,0x0A,0x0A,0x60,0x49,0x00,0x29,0xC0,0x8F,0x00,0x00,0x02,0x60,0xEA,0xAA,0xE0,
0x00,0x22,0x22,0x20,0x00,0xE2,0xE8,0xE0,0x00,0xE2,0xE2,0xE0,0x00,0xAA,0xE2,0x20,
0x00,0xE8,0xE2,0xE0,0x00,0xE8,0xEA,0xE0,0x00,0xE2,0x22,0x20,0x00,0xEA,0xEA,0xE0,
0x00,0xEA,0xE2,0xE0,0x00,0xEA,0xEA,0xA0,0x00,0x88,0xEA,0xE0,0x00,0xE8,0x88,0xE0,
0x00,0x22,0xEA,0xE0,0x00,0xE8,0xE8,0xE0,0x00,0xE8,0xE8,0x80,0x00,0x45,0x16,0x1E,
0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,0x48,0xA9,0x7F,
0x8F,0x0D,0x00,0x01,0x18,0x6E,0xDE,0xFF,0xAF,0x00,0x00,0x01,0x29,0x20,0x0A,0x0A,
0x18,0x6D,0xDE,0xFF,0x8D,0xDE,0xFF,0xEE,0xDF,0xFF,0xAD,0xDF,0xFF,0xC9,0x09,0xD0,
0x10,0xDA,0xAE,0xDC,0xFF,0xAD,0xDE,0xFF,0x9D,0x00,0xFD,0xEE,0xDC,0xFF,0xFA,0x68,
0x40,0xC9,0x0B,0xF0,0x02,0x68,0x40,0x9C,0xDF,0xFF,0x68,0x40,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x0B,0xFE,0x0B,0xFE,0x0B,0xFE,0x0B,0xFE,0x00,0x00,0x9D,0xFF,
0x00,0x00,0x00,0x00,0x0B,0xFE,0x00,0x00,0x0B,0xFE,0x0B,0xFE,0x00,0xFE,0x9D,0xFF
};

// tells if the initialization process worked or not
unsigned char sdcard_ready = 0x00;

unsigned char sdcard_data = 0x00;

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

void sdcard_longdelay(void)
{
	unsigned int tally = 0x0000;

	while (tally < 0x0400) // arbitrary amount of time to delay, should be around 10ms
	{
		tally = tally + 1;
	}
}

void sdcard_toggle(void)
{
	RC0 = 1; // CLK goes high then low again
	RC0 = 0; // make as fast as possible, use asm{ ... } ?
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

	TRISC = (TRISC & 0xF0) | 0x08;

	RC2 = 1; // CS is high
	RC1 = 1; // MOSI is high
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
	//sdcard_disable(); // do not disable, was a bug in gfoot's code
	if (temp_value != 0x01) { return 0; } // expecting 0x01
	//sdcard_enable(); // so thus don't have to re-enable also
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
		if (temp_value != 0x01) { break; } // expecting 0x01, but if not it might already be 'initialized'?
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

	return 1;
}

unsigned char sdcard_readinit(unsigned char high, unsigned char mid, unsigned char low)
{
	unsigned char temp_value = 0x00;

	sdcard_disable();
	//sdcard_pump(); // gfoot says I don't need these past initialization
	//sdcard_longdelay();
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
	unsigned char temp_value1 = 0x00, temp_value2 = 0x00;

	temp_value1 = sdcard_receivebyte(); // data packet ends with 0x55 then 0xAA
	temp_value2 = sdcard_receivebyte(); // ignore here
	
	sdcard_disable();

//	if (!(temp_value1 == 0x55 && temp_value2 == 0xAA)) { return 0; }

	return 1;	
}

unsigned char sdcard_writeinit(unsigned char high, unsigned char mid, unsigned char low)
{
	unsigned char temp_value = 0x00;

	sdcard_disable();
	//sdcard_pump(); // gfoot says I don't need these past initialization
	//sdcard_longdelay();
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

void sdcard_writefinal(void)
{
	unsigned char temp_value = 0x00;
	
	sdcard_sendbyte(0x55); // data packet ends with 0x55 then 0xAA
	sdcard_sendbyte(0xAA);
	temp_value = sdcard_receivebyte(); // toggle clock 8 times
	sdcard_disable();
}

void feed_data(unsigned char data, unsigned char low, unsigned char high, unsigned char bank)
{
	PORTA = 0xA9; // LDA#
	toggle();

	PORTA = data; // data
	toggle();

	PORTA = 0x8F; // STAal
	toggle();

	PORTA = low; // low address
	toggle();

	PORTA = high; // high addr
	toggle();

	PORTA = bank; // bank addr
	toggle();

	RC5 = 1; // shut down transceiver

	//TRISA = 0xFF; // data

	toggle();

	RC5 = 0; // start up transceiver

	//TRISA = 0x00;
}

void feed_jump(void)
{
	PORTA = 0x4C; // JMPa
	toggle();

	PORTA = 0x00; // low address
	toggle();

	PORTA = 0x80; // high address
	toggle();
}

void feed_native(void)
{
	PORTA = 0x18; // CLC
	toggle();

	PORTA = 0xFB; // XCE
	toggle();
}

void check_init(void)
{
	if (sdcard_ready == 0x00)
	{
		for (unsigned char i=0; i<5; i++)
		{
			if (sdcard_initialize())
			{
				sdcard_ready = 0xFF;
				break;
			}
		}
	}
}

void check_read(unsigned char high, unsigned char mid, unsigned char low)
{
	if (sdcard_ready != 0x00)
	{
		if (!sdcard_readinit(high, mid, low))
		{
			sdcard_ready = 0x00;
		}
	}
}

// main program, send interrupts on /NMI periodically and controls audio
void main(void)
{
	unsigned char addr_low = 0x00;
	unsigned char addr_high = 0x00;
	unsigned char addr_bank = 0x02;

	unsigned char temp_low = 0x00;
	unsigned char temp_high = 0x00;
	unsigned char temp_bank = 0x02;

	unsigned char pos = 0x00;
	unsigned char first = 0x00;

	// initial setup (if unused, set as output and drive low)
	// PHI2 is low, /BOOT is low, /RES is low, /NMI is input, TX/CK is high and RX/DT is input
	TRISA = 0x00;
	PORTA = 0x00;
	TRISB = 0x79;
	PORTB = 0x00;
	TRISC = 0x88;
	PORTC = 0x54;

	// you MUST set ANSELH to 0x00 to read PORTB inputs as digital!
	ANSEL = 0x00;
	ANSELH = 0x00;

	ADCON0 = 0x00;
	ADCON1 = 0x00;

	PCON = 0x03;

	CM1CON0 = 0x00;
	CM2CON0 = 0x00;
	CM2CON1 = 0x00;

	SSPCON = 0x00;
	SSPCON2 = 0x00;

	VRCON = 0x00;
	SRCON = 0x00;

	OPTION_REG = 0x87; // sets up INT edge
	INTCON = 0x00; //0xD0; // sets up INT interrupt and clears flags
	PIE1 = 0x00; // no other interrupts
	PIE2 = 0x00; // no other interrupts
	PIR2 = 0x00;

	//OSCTUNE = 0x00; // or 0x0F?
	//OSCCON = 0x60; // 4 MHz

	OSCTUNE = 0x00; // or 0x0F?
	OSCCON = 0x70; // 8 MHz

	while ((OSCCON & 0x04) == 0x00) { } // wait to stablize

	serial_initialize();

	RCIF = 0; // clear flag
	TXIF = 0; // clear flag

	for (unsigned char i=0; i<32; i++) // cycle phi2 32 times while /RES is low
	{
		toggle(); // toggle phi2
	}

	TRISB = 0xF9; // release /RES, pulled high

	PORTA = 0xEA; // NOP and/or vector location

	while (RB3)
	{
		toggle(); // toggle phi2 until /VP goes low
	}

	for (unsigned char i=0; i<32; i++) // cycle phi2 32 times while it settles from reset
	{
		toggle();
	}

	if (!(RB4 && RB5)) toggle(); // if in the middle of a NOP, toggle again

	//feed_native();

	feed_jump();

	serial_transmitchar('.');

	pos = 0x00;

	do
	{
		feed_data(codeA[pos], pos, 0xFE, 0x00);

		pos = pos + 0x01;
		
		//serial_transmitchar('.');
	} 
	while (pos != 0x00);

	feed_jump();

	serial_transmitchar('.');

	do
	{
		feed_data(codeB[pos], pos, 0xFF, 0x00);

		pos = pos + 0x01;
		
		//serial_transmitchar('.');
	} 
	while (pos != 0x00);

	feed_jump();

	serial_transmitchar('?');

	sdcard_block_high = 0x00; // start in block 0 on SDcard
	sdcard_block_mid = 0x00;
	sdcard_block_low = 0x00;
	sdcard_data = 0x00;
	sdcard_ready = 0x00;

	addr_low = 0x00;
	addr_high = 0x00;
	addr_bank = 0x02;

	pos = 0x00;
	first = 0x00;

	do
	{
		if (pos == 0x01 && first == 0x00)
		{
			first = 0x01;
			pos = 0x00;

			addr_low = temp_low;
			addr_high = temp_high;
			addr_bank = temp_bank;

			sdcard_block_low = 0x00;
		}

		check_init();

		check_read(sdcard_block_high, sdcard_block_mid, sdcard_block_low);
			
		if (sdcard_ready != 0x00)
		{
			temp_low = addr_low;
			temp_high = addr_high;
			temp_bank = addr_bank;

			for (unsigned int i=0; i<512; i++)
			{
				sdcard_data = sdcard_receivebyte();

				feed_data(sdcard_data, addr_low, addr_high, addr_bank);

				addr_low = addr_low + 0x01;
				
				if (addr_low == 0x00)
				{
					addr_high = addr_high + 0x01;

					if (addr_high == 0x00)
					{
						addr_bank = addr_bank + 0x01;
					}
				}
			}

			feed_jump();

			if (sdcard_readfinal())
			{
				//serial_transmitchar('.');

				sdcard_block_low = sdcard_block_low + 0x02;

				if (sdcard_block_low == 0x00)
				{
					sdcard_block_mid = sdcard_block_mid + 0x01;

					if (sdcard_block_mid == 0x00)
					{
						sdcard_block_high = sdcard_block_high + 0x01;
					}
				}

				pos = pos + 0x01;

				if (pos == 0x00)
				{
					feed_data(0x5C, 0x00, 0xFE, 0x00);
					feed_data(0x00, 0x01, 0xFE, 0x00);
					feed_data(0xE0, 0x02, 0xFE, 0x00);
					feed_data(0x03, 0x03, 0xFE, 0x00); // jump to new code!
				}
			}
			else
			{
				//serial_transmitchar('x');

				addr_low = temp_low;
				addr_high = temp_high;
				addr_bank = temp_bank;
			}
		}
	}
	while (pos != 0x00);

	//TRISA = 0xFF; // release data bus

	RC5 = 1; // shut down transceiver

	RB7 = 0; // get /RES ready for low
	TRISB = 0x7B; // /RES is output

	for (unsigned char i=0; i<32; i++) // cycle phi2 32 times while /RES is low
	{
		toggle(); // toggle phi2
	}

	TRISB = 0xF9; // release /RES, pulled high

	TRISB = 0xFB; // release PHI2

	RB2 = 1; // have /BOOT go high, letting CPLD run normally

	serial_transmitchar('*');

	while (1) { }
}










