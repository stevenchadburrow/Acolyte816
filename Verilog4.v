
`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    16:41:04 11/28/2023 
// Design Name: 
// Module Name:    Verilog4 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////


/*

Make sure to run fitter like in command line, it allows you to change -optimize to "density" which makes things fit!

cd ~/Xilinx/LastAcolyte6 ; ~/Xilinx/14.7/ISE_DS/ISE/bin/lin64/cpldfit -intstyle ise -p xc9572xl-5-VQ44 -ofmt vhdl -optimize density -htmlrpt -loc on -slew fast -init low -inputs 54 -pterms 25 -unused float -power std -terminate keeper Verilog4.ngd ; ~/Xilinx/14.7/ISE_DS/ISE/bin/lin64/hprep6 -s IEEE1149 -n Verilog4 -i Verilog4

cd /usr/share ; sudo xc3sprog -c xpc ~/Xilinx/LastAcolyte6/Verilog4.jed

Use 'xc3sprog' to program the .jed file into the CPLD, but you need to follow some instructions:

Notes for the USB cable
=======================

To use the device as an ordinary user, put the following line in a new
file "libusb-driver.rules" in /etc/udev/rules.d/ and restart udev:
ACTION=="add", SUBSYSTEMS=="usb", ATTRS{idVendor}=="03fd", MODE="666"


If your cable does not have the ID 03fd:0008 in the output of lsusb,
the initial firmware has not been loaded (loading it changes the
product-ID from another value to 8). To load the firmware follow
these steps:

1. Run ./setup_pcusb in this directory, this should set up everything
   correctly:
   - When $XILINX is set correctly:
     $ ./setup_pcusb
   - When $XILINX is not set, and ISE is installed in /opt/Xilinx/13.1:
     $ ./setup_pcusb /opt/Xilinx/13.1/ISE_DS/ISE

Old instructions, use only when the above script did not work:

1. If you have no /etc/udev/rules.d/xusbdfwu.rules file, copy it from
   /path/to/ISE/bin/lin/xusbdfwu.rules to /etc/udev/rules.d/xusbdfwu.rules

2. If you are running a newer version of udev (as in Debian Squeeze and
   Ubuntu 9.10), you need to adapt the rules-file to the new udev-version:
   sed -i -e 's/TEMPNODE/tempnode/' -e 's/SYSFS/ATTRS/g' -e 's/BUS/SUBSYSTEMS/' /etc/udev/rules.d/xusbdfwu.rules

3. Install the package containing /sbin/fxload from your linux distribution.
   It is usually called "fxload"

4. copy the files /path/to/ISE/bin/lin/xusb*.hex to /usr/share/

5. restart udev and re-plug the cable with 'service udev restart'

That should do it???

*/

// This is for W65C816 at 12.5 MHz with 1 MB of RAM
// 64-colors at 256x240 or 2-colors at 512x240
// keyboard/mouse, 2x joysticks, and 1-voice audio through VIA
// bootloader and SDcard through PIC

// Memory Map:
// $000000-$00FFFF = Zero Bank, used for Interrupts and Vectors
// $010000-$01FFFF = VIA Bank, where VIA resides (16 bytes duplicated throughout)
// $020000-$03FFFF = Video Banks, where color video data is, 512 bytes per line, overscanned
// $040000-$07FFFF = 256KB on RAM-A chip
// $080000-$0FFFFF = 512KB on RAM-B chip
// $100000-$FFFFFF = 15MB unused, ready for expansion

module Verilog4(
	input emu, // Emulation pin from CPU
	inout nmi, // /NMI controlled by PIC or CPLD
	input master_clock, // 25.175 MHz
	inout phi2, // 12.5 MHz
	output not_phi2, // inverted clock
	inout shift_clock, //SR clock from VIA
	inout shift_data, // SR data from VIA
	input boot, // from PIC, tells if we are in boot mode
	output visible, // turns color off
	output reg hsync, // video sync signals
	output reg vsync,
	input zero, // has pull-down resistor, bring high to disable first 1MB, used for memory expansion
	input rw, // /RW from CPU
	output rd, // /RD (or /OE) sent to RAM
	output wr, // /WR (or /WE) sent to RAM
	input [3:0] bank, // bank registers from CPU during phi2 low
	output [8:0] address, // video addresses during phi2 low
	output [2:0] top, // video addrs during phi2 low, and CPU addresses during phi2 high
	output via, // /CS signals for VIA and RAM
	output ram_a,
	output ram_b
    );

reg half; // true phi2 value
reg [18:0] video_addr; // video addresses
reg [7:0] video_scroll; // vertical scrolling
reg hblank; // blanking values
reg vblank;
reg latch_via; // latches for VIA and RAM on rising phi2
reg latch_ram_a;
reg latch_ram_b;
reg [3:0] select; // bank values A16-A18 during phi2 low
reg interrupt; // periodic interrupt using v-sync
reg prev; // previous value on shift_clock
reg [4:0] combined_addr;

// during boot the PIC controls phi2, otherwise it's half the 25.175 MHz
assign phi2 = (~boot) ? 1'bz : half;
assign not_phi2 = (~boot) ? ~phi2 : (~via ? 1'b1 : ~half);

// these are A0-A8, changed each pixel, inactive during phi2 high or booting,
// these supply A9-A15 during h-sync for external latch
assign address[0] = (~boot) ? 1'bz :
	((~hsync && ~phi2) ? video_addr[10] :
	((~phi2) ? video_addr[0] : 1'bz));
assign address[2:1] = (~boot) ? 2'bzz :
	((~hsync && ~phi2) ? video_addr[12:11] :
	((~phi2) ? video_addr[2:1] : 2'bzz));
assign address[6:3] = (~boot) ? 4'bzzzz :
	((~hsync && ~phi2) ? combined_addr[3:0] : //video_addr[16:13] :
	((~phi2) ? video_addr[6:3] : 4'bzzzz));
assign address[7] = (~boot) ? 1'bz :
	((~hsync && ~phi2) ? 1'b0 : // change here to whatever value you like
	((~phi2) ? video_addr[7] : 1'bz));
assign address[8] = (~boot) ? 1'bz :
	((~hsync && ~phi2) ? 1'bz :
	((~phi2) ? video_addr[8] : 1'bz));

// these are A16, A17, and A18, connected to RAM for 512KB each
assign top[0] = (~boot) ? select[0] :
	((~phi2) ? combined_addr[4] : //video_addr[17] :
	select[0]);
assign top[1] = (~boot) ? select[1] :
	((~phi2) ? 1'b1 : select[1]);
assign top[2] = (~boot) ? select[2] :
	((~phi2) ? 1'b0 : select[2]);


// always reading while phi2 is low, normal otherwise
// /WR assumes a 'register' type SRAM, where data is written only on rising /WR
assign rd = (~phi2) ? 1'b0 : ~rw;
assign wr = (~phi2 || ~master_clock || latch_via) ? 1'b1 : rw;

// VIA is in RAM-A space, and needs it's /CS line activated before phi2 rises
assign via = (~boot) ? 1'b1 :
	((~phi2 && ~zero && ~bank[3] && ~bank[2] && ~bank[1] && bank[0]) ? 1'b0 : 
	((phi2 && latch_via) ? 1'b0 : 1'b1));

// RAM-A is active when writing while booting, otherwise runs normal
assign ram_a = (~boot) ? (~rw ? 1'b0 : 1'b1) :
	((~phi2) ? 1'b0 :
	((phi2 && ~latch_via) ? 1'b0 : 1'b1));

// RAM-B is inactive while booting, otherwise runs normal
assign ram_b = 1'b1;
//	(~boot) ? 1'b1 :
//	((~phi2) ? 1'b1 :
//	((latch_ram_b) ? 1'b0 : 1'b1));
	
// black screen while booting, otherwise use blanking values
assign visible = (~boot) ? 1'b0 :
	((~hblank || ~vblank) ? 1'b0 : 1'b1);
	
// ground if v-sync (and PIC not active)
assign nmi = 1'b1;
//	(~interrupt) ? 1'b0 : 1'bz;

always @(negedge master_clock) begin
	
	if (~boot) begin
		video_scroll[7:0] <= 8'b00000000; // zero scroll
	end
	
	if (shift_clock && ~prev) begin
		video_scroll[7:1] <= video_scroll[6:0];
		video_scroll[0] <= shift_data;
	end

	prev <= shift_clock;
	
	if (half) begin // if falling phi2...
		latch_via <= 1'b0;
		
		combined_addr[4:0] <= video_addr[17:13] - video_scroll[7:3];
	end

	if (~half) begin // if rising phi2...
		
		latch_via <= ~zero && ~bank[3] && ~bank[2] && ~bank[1] && bank[0]; // VIA located at $010000		
		
		if (boot || (~boot && ~phi2)) begin
			select[3:0] <= bank[3:0]; // store for high addresses
		end
		
		// video sync signals
		if (video_addr[8:0] == 9'b101000000) begin // 320 pixels
			hblank <= 1'b0; // turn off h-blank
		end
		
		if (video_addr[8:0] == 9'b101001000) begin // 328 pixels
			hsync <= 1'b0; // turn on h-sync
			
			if (video_addr[18:9] == 10'b1000001101) begin // 525 lines
				video_addr[18:9] <= 10'b0000000000; // 0 lines
				
				vblank <= 1'b1; // turn on v-blank
			end
			else begin
				video_addr[18:9] <= video_addr[18:9] + 1; // increment lines during h-sync
			end
		end
				
		if (video_addr[8:0] == 9'b101111000) begin // 376 pixels
			hsync <= 1'b1; // turn off h-sync
		end
		
		if (video_addr[8:0] == 9'b110010000) begin // 400 pixels
			video_addr[8:0] <= 9'b000000000; // 0 pixels
		
			hblank <= 1'b1; // turn on h-blank
		
			if (video_addr[18:9] == 10'b0111100000) begin // 480 lines
				vblank <= 1'b0; // turn off v-blank
			end
		
			if (video_addr[18:9] == 10'b0111101010) begin // 490 lines
				vsync <= 1'b0; // turn on v-sync
				
				if (~emu) begin
					interrupt <= 1'b0; // periodic interrupts using /NMI if Emulation pin is low
				end
			end
					
			if (video_addr[18:9] == 10'b0111101100) begin // 492 lines
				vsync <= 1'b1; // turn off v-sync
				
				interrupt <= 1'b1; // release /NMI, pulled high
			end
		end
		else begin
			video_addr[8:0] <= video_addr[8:0] + 1; // increment pixels
		end
	end
	
	half <= ~half; // alternate phi2 clock
end

	
endmodule



