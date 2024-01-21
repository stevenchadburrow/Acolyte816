; Bootloader Code for the (upcoming) Acolyte '816 SBC
; 512 bytes hard coded into the PIC16F886
; Includes SDcard loading command and mini monitor

; g++ -o AcolyteSimulator.o AcolyteSimulator.cpp -lglfw -lGL -lGLU ; g++ -o AcolyteAssembler.o AcolyteAssembler.cpp

; cd ~/Projects/Assembler ; ./AcolyteAssembler.o LastAcolyte6-Bootloader.asm LastAcolyte6-Bootloader.bin LastAcolyte6-Bootloader.hex 64768 65536 ; hexdump -C LastAcolyte6-Bootloader.bin

; You can run this in the simulator (given the hex was preloaded) by doing this
; ./AcolyteSimulator.o null

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
.EQU key_array		$00FD00

; location of keyboard variables in RAM
.EQU key_write		$00FFDC
.EQU key_read		$00FFDD
.EQU key_data		$00FFDE
.EQU key_counter	$00FFDF



; code
.ORG $FE00
.LOC start of code
:reset				; upon reset...
	NOP
	NOP
	NOP
	NOP			; 4 NOP codes, will be replaced later by PIC?
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP			; and some extra because it will fit


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
	STA via_db		; PB is inputs
	STA via_acr		; no latching
	STA via_pcr		; set up CA1 falling edge interrupts
	LDA #$82
	STA via_ier		; set up /IRQ interrupts for CA1
;	STZ =key_write
;	STZ =key_read
;	STZ =key_counter
	CLI			; accept /IRQ interupts	

; clears the screen
:monitor_clear
	LDA #$00
*
	JSR hex_pixel_store
	INC hex_pixel_low
	BNE -
	LDX hex_pixel_bank
	CPX #$03
	BNE +
	LDX hex_pixel_high
	CPX #$E0
	BNE +
	DEC hex_pixel_bank
	BRA monitor_display
*
	INC hex_pixel_high
	BNE --
	INC hex_pixel_bank
	BRA --

; displays the zero page
:monitor_display
	JSR monitor_page

; looks for a keyboard press
:monitor_key
	LDX =key_read	; compare key_read with key_write
	CPX =key_write
	BEQ monitor_key		; if they are the same, loop back
	INC =key_read ; increment key_read to match key_write
	LDA monitor_skip
	STZ monitor_skip
	BNE monitor_key
	LDA =key_array,X	; load latest keyboard press

	CMP #$F0 ; release	; if release, then increment key_read again (to skip)
	BNE +
	INC monitor_skip
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
	ADC #$10		; increase location by 16
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
	LDA monitor_location
	STA monitor_jump
.DAT $20 ; JSRa			; run code from $0000XX
:monitor_jump
.DAT $00 ; low addr
.DAT $00 ; high addr
	BRA monitor_display	; can return to monitor if using RTS at some point


; Draws all of zero page
; Y = starting y-coordinate on screen
:monitor_page
	LDY #$00		; location on screen, should start at zero
	LDX #$00

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
	AND #$0F		; allow 16 bytes per line, thus 16 lines total
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

:monitor_skip
.DAT $00


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
	INX			; increment two x-coordinates
	INX
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
	BRA +

:hex_down			; draws second half of row, and goes down a line
	PHA
	JSR hex_pixel		; send pixel to screen
	DEC hex_pixel_low
	INC hex_pixel_high
	INC hex_pixel_high	; go back one but go down as well
*
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

:hex_pixel_store
.DAT $8F 			; $8F for STAal to use with 65816
:hex_pixel_low
.DAT $00
:hex_pixel_high
.DAT $00
:hex_pixel_bank
.DAT $02
.DAT $60 ; RTS

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
	LDA #$7F
	STA via_ifr			; clear interrupts
	CLC
	ROR =key_data 			; shift key_code
	LDA via_pb
	AND #$20			; read PB5
	ASL
	ASL				; shift into bit 7
	CLC
	ADC =key_data			; add the bit 7 into key_code
	STA =key_data
	INC =key_counter
	LDA =key_counter
	CMP #$09 ; data ready		; 1 start bit, 8 data bits = 9 bits until real data ready
	BNE +	
	PHX
	LDX =key_write
	LDA =key_data
	STA =key_array,X		; put the key code into key_array	
	INC =key_write
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
:dummy				; dummy for unused interrupts, can replace later
	RTI


.LOC end of code, do not pass $FFDC


; this left over space contains variables

.ORG $00FFDC
.DAT $00,$00,$00,$00 ; [ key_write, key_read, key_data, key_counter ]

; vectors

.ORG $00FFE0
.DAT $0000 	; reserved
.DAT $0000 	; reserved
.DAT dummy 	; COP (native)
.DAT dummy	; BRK (native)
.DAT dummy 	; ABORTB (native)
.DAT dummy 	; NMIB (native)
.DAT $0000 	; reserved
.DAT key 	; IRQB (native)

.ORG $00FFF0
.DAT $0000	; reserved 
.DAT $0000	; reserved
.DAT dummy	; COP (emulation)
.DAT $0000	; reserved
.DAT dummy	; ABORTB (emulation)
.DAT dummy 	; NMIB (emulation)
.DAT reset	; RESETB (emulation)
.DAT key	; IRQB/BRK (emulation)





