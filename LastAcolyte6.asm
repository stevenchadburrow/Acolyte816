; Bootloader Code for the (upcoming) '816 SBC
; 512 bytes hard coded into the PIC16F886
; Includes SDcard loading command and mini monitor

; g++ -o AcolyteSimulator.o AcolyteSimulator.cpp -lglfw -lGL -lGLU ; g++ -o Assembler.o Assembler.cpp

; ./Assembler.o LastAcolyte6.asm LastAcolyte6.bin LastAcolyte6.hex 0 65536 ; ./AcolyteSimulator.o LastAcolyte6.bin

; ./Assembler.o LastAcolyte6.asm LastAcolyte6.bin LastAcolyte6.hex 65024 65536


; VIA location
.EQU via		$010000
.EQU via_pb		$010000
.EQU via_pa		$010001
.EQU via_db		$010002
.EQU via_da		$010003
.EQU via_t1cl		$010004
.EQU via_t1ch		$010005
.EQU via_t1ll		$010006
.EQU via_t1lh		$010007
.EQU via_t2cl		$010008
.EQU via_t2ch		$010009
.EQU via_sr		$01000A
.EQU via_acr		$01000B
.EQU via_pcr		$01000C
.EQU via_ifr		$01000D
.EQU via_ier		$01000E
.EQU via_pah		$01000F

; keyboard array is 256 byte page in RAM
.EQU key_array		$000200

; location of keyboard variables in RAM
.EQU key_write		$00FFE0
.EQU key_read		$00FFE1
.EQU key_data		$00FFE2
.EQU key_counter	$00FFE4


; code
.ORG $FE00
.LOC start of code
:reset				; upon reset...
	WAI			; bring RDY low, wait for PIC to interrupt and bring /NMI low, not coming back

:dummy				; dummy used for native mode, can replace later
	RTI

:load				; used in emulation mode, copy code in for bootloader
	LDA #$04
	STA via_ier		; set up /IRQ interrupts for SR (but don't use CLI!)
; first access of non-zero bank, at this point /NMI interrupts on V-SYNC, but not while /NMI is already low
	LDA #$1C
	STA via_acr		; set up SR in mode 111
	LDA #$00		; command for loading 256KB of data
	STA via_sr		; store accumulator in SR
	WAI			; wait for PIC

	LDA #$0C
	STA via_acr		; set up SR in mode 011
	WAI			; wait for PIC
	LDA via_sr		; get return value from SR
	BEQ monitor		; if $00, SDcard error, go to monitor

:copy_sdcard_loop
	WAI			; wait for PIC
	LDA via_sr		; get SR data

; store data from SR to RAM
.DAT $8F ; STAal
:copy_sdcard_low
.DAT $00 ; low address, will be modified
:copy_sdcard_high
.DAT $00 ; high address, will be modified
:copy_sdcard_bank
.DAT $02 ; bank address at $020000, will be modified 

	INC copy_sdcard_low	; increment low address
	BNE copy_sdcard_loop	; if not zero, loop
	INC copy_sdcard_high	; increment high address
	BNE copy_sdcard_loop	; if not zero, loop
	INC copy_sdcard_bank	; increment bank address when high address is zero
	LDA copy_sdcard_bank
	CMP #$08
	BEQ copy_sdcard_loop	; keep going until $040000, which would make 128KB
	CLC
	XCE			; exit emulation mode, enter native mode
	JMP $03FF00	 	; jump to end of the newly loaded code!


; mini monitor in case SD card is not present
; only allows displays zero page
; Controls:
; Space = skip forward one byte
; Backspace = skip backward one byte
; Enter = skip forward 32 bytes
; Escape = run code starting at $000000
; 0-9 or A-F = change current byte value
:monitor
	LDA #$00
	STA via_da		; PA is inputs
	LDA #$02
	STA via_pcr		; set up CA1 falling edge interrupts
	STA via_ier		; set up /IRQ interrupts for CA1
	CLI			; accept /IRQ interupts	

; displays the zero page
:monitor_display
	JSR monitor_page

; looks for a keyboard press
:monitor_key
	LDX =key_read		; compare key_read with key_write
	CPX =key_write
	BEQ monitor_key		; if they are the same, loop back
	INC =key_read		; increment key_read to match key_write
	LDA =key_array,X	; load latest keyboard press
	CMP #$F0 ; release	; if release, then increment key_read again (to skip)
	BNE +
	INC =key_read
	BRA monitor_key		; go back to waiting for keyboard presses
*
	CMP #$29 		; space to skip byte
	BNE +
	INC monitor_location	; increase location by 1
	BRA monitor_display
*
	CMP #$5A 		; return to skip line
	BNE +
	LDA monitor_location
	CLC
	ADC #$20		; increase location by 32
	STA monitor_location
	BRA monitor_display
*
	CMP #$66 		; backspace to back up
	BNE +
	DEC monitor_location	; increase location by -1
	BRA monitor_display
*
	CMP #$76 		; escape to run at $000000
	BEQ monitor_run
	LDX #$0F		; otherwise, it might be a hex character
*
	CMP hex_lookup,X	; look up which one, if none defaults to 0
	BEQ +
	DEX
	BNE -
*
	PHX
	JSR monitor_load	; load in location into X
	LDA $00,X		; load value in zero page
	ASL			; shift 4 times left
	ASL
	ASL
	ASL
	STA $00,X		; store back into zero page
	PLA
	ORA $00,X		; bitwise-or with key press hex value
	STA $00,X		; store back into zero page
	BRA monitor_display	; redraw zero page

:monitor_run
	JSR $0000		; run code from $000000
	BRA monitor_display	; can return to monitor if using RTS at some point


; Draws all of zero page
; Y = starting y-coordinate on screen
:monitor_page
	LDY #$04		; location of bitmap graphics, can change as needed
	LDX #$00		; should start at zero here though

:monitor_loop			; loop through all of zero page
.DAT $A5 ; LDAz
:monitor_byte
.DAT $00
	PHX
	STZ hex_pixel_inverse	; default is no inverse
	JSR monitor_load
	CPX monitor_byte
	BNE +
	DEC hex_pixel_inverse	; but invert current location
*
	PLX
	JSR hex_byte		; draw the full byte (2 characters)
	INC monitor_byte	; move to the next byte
	TXA
	AND #$3F		; allow 32 bytes per line, thus 8 lines total
	BNE monitor_loop
	LDX #$00		; start back at zero horizontally
	INY			; increment y-coordinate twice
	INY
	LDA monitor_byte
	BNE monitor_loop	; if end of zero page, exit
	RTS

:monitor_load			; used to grab value in zero page and store in X
.DAT $A2 ; LDX#
:monitor_location
.DAT $00
	RTS



; Draws two hex characters
; A = hex value to be printed
; X = x-coordinate on screen
; Y = y-coordinate on screen
:hex_byte
	PHA			; retain A
	PHA
	AND #$F0		; shift higher nibble down 4 places
	CLC
	ROR
	ROR
	ROR
	ROR
	JSR hex_char		; draw the character
	INX			; increment x-coordinate
	PLA
	AND #$0F		; no shifting needed for lower nibble
	JSR hex_char		; draw the character
	INX			; increment x-coordinate
	PLA			; get A back before leaving
	RTS

:hex_char
	PHA			; retain A, X, and Y
	PHX
	PHY
	PHA
	TXA
	ASL
	STA hex_pixel_low	; store multiplied x-coordinate in lower addr
	TYA
	ASL
	ASL
	ASL
	STA hex_pixel_high	; store multiplied y-coordinate in upper addr
	PLA
	ASL
	ASL
	TAX			; multiply A by 4 because 4 bytes per character
	LDY #$04
*
	LDA hex_data,X		; get bitmap data
	JSR hex_left		; draw first half
	JSR hex_down		; draw second half and go down
	JSR hex_left		; repeat
	JSR hex_down
	INX			; increment to next bitmap byte
	DEY
	BNE -			; do this 4 times
	PLY
	PLX
	PLA			; get A, X, and Y back before leaving
	RTS

:hex_left			; draws first half of the row
	PHA
	JSR hex_pixel		; send pixel to screen
	INC hex_pixel_low	; go to next location
	PLA
	ASL			; shift over by two slots (two pixels per byte)
	ASL
	RTS

:hex_down			; draws second half of row, and goes down a line
	PHA
	JSR hex_pixel		; send pixel to screen
	DEC hex_pixel_low
	INC hex_pixel_high
	INC hex_pixel_high	; go back one but go down as well
	PLA
	ASL			; shift over by two slots (two pixels per byte)
	ASL
	RTS


:hex_pixel			; sends pixel to screen, changes per memory map
.DAT $49 ; EOR#
:hex_pixel_inverse
.DAT $00
.DAT $29 ; AND#
.DAT $C0
.DAT $8F 			; $8F for STAal to use with 65816
:hex_pixel_low
.DAT $00
:hex_pixel_high
.DAT $00
:hex_pixel_bank
.DAT $02
	RTS

:hex_data ; bitmap data
.DAT $EA,$AA,$E0,$00 ; 0
.DAT $22,$22,$20,$00 ; 1
.DAT $E2,$E8,$E0,$00 ; 2
.DAT $E2,$E2,$E0,$00 ; 3
.DAT $AA,$E2,$20,$00 ; 4
.DAT $E8,$E2,$E0,$00 ; 5
.DAT $E8,$EA,$E0,$00 ; 6
.DAT $E2,$22,$20,$00 ; 7
.DAT $EA,$EA,$E0,$00 ; 8
.DAT $EA,$E2,$E0,$00 ; 9
.DAT $EA,$EA,$A0,$00 ; A
.DAT $88,$EA,$E0,$00 ; B
.DAT $E8,$88,$E0,$00 ; C
.DAT $22,$EA,$E0,$00 ; D
.DAT $E8,$E8,$E0,$00 ; E
.DAT $E8,$E8,$80,$00 ; F

:hex_lookup ; PS/2 keyboard codes
.DAT $45,$16,$1E,$26 ; 0,1,2,3
.DAT $25,$2E,$36,$3D ; 4,5,6,7
.DAT $3E,$46,$1C,$32 ; 8,9,A,B
.DAT $21,$23,$24,$2B ; C,D,E,F


; IRQ-ISR for keyboard input
:key
	PHA
	LDA via_pa
	AND #$80			; read PA7
	CLC	
	ROR =key_data			; shift key_code
	CLC
	ADC =key_data			; add the PA7 bit into key_code
	STA =key_data
	INC =key_counter		; increment key_counter
	LDA =key_counter
	CMP #$09 ; data ready		; 1 start bit, 8 data bits = 9 bits until real data ready
	BNE +
	LDA =key_data
	PHX
	LDX =key_write
	STA =key_array,X		; put the key code into key_array
	TXA
	INC
	STA =key_write
	PLX
	PLA
	RTI				; and exit
*
	CMP #$0B ; reset counter	; 1 start bit, 8 data bits, 1 parity bit, 1 stop bit = 11 bits to complete a full signal
	BEQ +
	PLA
	RTI				; and exit
*
	STZ =key_counter		; reset the counter
	PLA
	RTI

.LOC end of code

; vectors
.ORG $00FFE0
.DAT $0000 	; reserved [ used in monitor ]
.DAT $0000 	; reserved [ used in monitor ]
.DAT dummy 	; COP (native)
.DAT dummy	; BRK (native)
.DAT dummy 	; ABORTB (native)
.DAT dummy 	; NMIB (native)
.DAT $0000 	; reserved
.DAT dummy 	; IRQB (native)
.DAT $0000	; reserved
.DAT $0000	; reserved
.DAT dummy	; COP (emulation)
.DAT $0000	; reserved
.DAT dummy	; ABORTB (emulation)
.DAT load 	; NMIB (emulation)
.DAT reset	; RESETB (emulation)
.DAT key	; IRQB/BRK (emulation)





