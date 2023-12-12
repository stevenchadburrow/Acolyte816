//============================================================================
// Name        : mos6502
// Author      : Gianluca Ghettini
// Version     : 1.0
// Copyright   :
// Description : A MOS 6502 CPU emulator written in C++
//============================================================================

// This has been altered by Agumander (Clyde Shaffer) and by Steven Chad Burrow

#include "mos6502-Edit816.h"

mos6502::mos6502(BusRead r, BusWrite w, CPUEvent stp, BusRead sync)
{
	Write = (BusWrite)w;
	Read = (BusRead)r;
	Stopped = (CPUEvent)stp;
	Sync = (BusRead)sync;
	Instr instr;
	irq_timer = 0;

	// fill jump table with ILLEGALs
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_ILLEGAL;
	for(int i = 0; i < 256; i++)
	{
		InstrTable[i] = instr;
	}

	// insert opcodes

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 2;
	InstrTable[0x69] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 4;
	InstrTable[0x6D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 3;
	InstrTable[0x65] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 6;
	InstrTable[0x61] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 6;
	InstrTable[0x71] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 4;
	InstrTable[0x75] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 4;
	InstrTable[0x7D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 4;
	InstrTable[0x79] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 6;
	InstrTable[0x72] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 2;
	InstrTable[0x29] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 4;
	InstrTable[0x2D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 3;
	InstrTable[0x25] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 6;
	InstrTable[0x21] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 5;
	InstrTable[0x31] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 4;
	InstrTable[0x35] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 4;
	InstrTable[0x3D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 4;
	InstrTable[0x39] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 5;
	InstrTable[0x32] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ASL;
	instr.cycles = 6;
	InstrTable[0x0E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ASL;
	instr.cycles = 5;
	InstrTable[0x06] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_ASL_ACC;
	instr.cycles = 2;
	InstrTable[0x0A] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ASL;
	instr.cycles = 6;
	InstrTable[0x16] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ASL;
	instr.cycles = 7;
	InstrTable[0x1E] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BCC;
	instr.cycles = 2;
	InstrTable[0x90] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BCS;
	instr.cycles = 2;
	InstrTable[0xB0] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BEQ;
	instr.cycles = 2;
	InstrTable[0xF0] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BRA;
	instr.cycles = 3;
	InstrTable[0x80] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_BIT;
	instr.cycles = 4;
	InstrTable[0x2C] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_BIT;
	instr.cycles = 3;
	InstrTable[0x24] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BMI;
	instr.cycles = 2;
	InstrTable[0x30] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BNE;
	instr.cycles = 2;
	InstrTable[0xD0] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BPL;
	instr.cycles = 2;
	InstrTable[0x10] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_BRK;
	instr.cycles = 7;
	InstrTable[0x00] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BVC;
	instr.cycles = 2;
	InstrTable[0x50] = instr;

	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BVS;
	instr.cycles = 2;
	InstrTable[0x70] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLC;
	instr.cycles = 2;
	InstrTable[0x18] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLD;
	instr.cycles = 2;
	InstrTable[0xD8] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLI;
	instr.cycles = 2;
	InstrTable[0x58] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLV;
	instr.cycles = 2;
	InstrTable[0xB8] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 2;
	InstrTable[0xC9] = instr;
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_WAI;
	instr.cycles = 3;
	InstrTable[0xCB] = instr;
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_STP;
	instr.cycles = 3;
	InstrTable[0xDB] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 4;
	InstrTable[0xCD] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 3;
	InstrTable[0xC5] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 6;
	InstrTable[0xC1] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 3;
	InstrTable[0xD1] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 4;
	InstrTable[0xD5] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 4;
	InstrTable[0xDD] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 4;
	InstrTable[0xD9] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 5;
	InstrTable[0xD2] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_CPX;
	instr.cycles = 2;
	InstrTable[0xE0] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_CPX;
	instr.cycles = 4;
	InstrTable[0xEC] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_CPX;
	instr.cycles = 3;
	InstrTable[0xE4] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_CPY;
	instr.cycles = 2;
	InstrTable[0xC0] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_CPY;
	instr.cycles = 4;
	InstrTable[0xCC] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_CPY;
	instr.cycles = 3;
	InstrTable[0xC4] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_DEC_ACC;
	instr.cycles = 2;
	InstrTable[0x3A] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_DEC;
	instr.cycles = 6;
	InstrTable[0xCE] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_DEC;
	instr.cycles = 5;
	InstrTable[0xC6] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_DEC;
	instr.cycles = 6;
	InstrTable[0xD6] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_DEC;
	instr.cycles = 7;
	InstrTable[0xDE] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_DEX;
	instr.cycles = 2;
	InstrTable[0xCA] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_DEY;
	instr.cycles = 2;
	InstrTable[0x88] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 2;
	InstrTable[0x49] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 4;
	InstrTable[0x4D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 3;
	InstrTable[0x45] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 6;
	InstrTable[0x41] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 5;
	InstrTable[0x51] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 4;
	InstrTable[0x55] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 4;
	InstrTable[0x5D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 4;
	InstrTable[0x59] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 5;
	InstrTable[0x52] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_INC_ACC;
	instr.cycles = 2;
	InstrTable[0x1A] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_INC;
	instr.cycles = 6;
	InstrTable[0xEE] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_INC;
	instr.cycles = 5;
	InstrTable[0xE6] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_INC;
	instr.cycles = 6;
	InstrTable[0xF6] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_INC;
	instr.cycles = 7;
	InstrTable[0xFE] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_INX;
	instr.cycles = 2;
	InstrTable[0xE8] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_INY;
	instr.cycles = 2;
	InstrTable[0xC8] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_JMP;
	instr.cycles = 3;
	InstrTable[0x4C] = instr;
	instr.addr = &mos6502::Addr_ABI;
	instr.code = &mos6502::Op_JMP;
	instr.cycles = 5;
	InstrTable[0x6C] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_JSR;
	instr.cycles = 6;
	InstrTable[0x20] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 2;
	InstrTable[0xA9] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 4;
	InstrTable[0xAD] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 3;
	InstrTable[0xA5] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 6;
	InstrTable[0xA1] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 5;
	InstrTable[0xB1] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 4;
	InstrTable[0xB5] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 4;
	InstrTable[0xBD] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 4;
	InstrTable[0xB9] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 5;
	InstrTable[0xB2] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_LDX;
	instr.cycles = 2;
	InstrTable[0xA2] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LDX;
	instr.cycles = 4;
	InstrTable[0xAE] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LDX;
	instr.cycles = 3;
	InstrTable[0xA6] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_LDX;
	instr.cycles = 4;
	InstrTable[0xBE] = instr;
	instr.addr = &mos6502::Addr_ZEY;
	instr.code = &mos6502::Op_LDX;
	instr.cycles = 4;
	InstrTable[0xB6] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_LDY;
	instr.cycles = 2;
	InstrTable[0xA0] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LDY;
	instr.cycles = 4;
	InstrTable[0xAC] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LDY;
	instr.cycles = 3;
	InstrTable[0xA4] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_LDY;
	instr.cycles = 4;
	InstrTable[0xB4] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_LDY;
	instr.cycles = 4;
	InstrTable[0xBC] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LSR;
	instr.cycles = 6;
	InstrTable[0x4E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LSR;
	instr.cycles = 5;
	InstrTable[0x46] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_LSR_ACC;
	instr.cycles = 2;
	InstrTable[0x4A] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_LSR;
	instr.cycles = 6;
	InstrTable[0x56] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_LSR;
	instr.cycles = 7;
	InstrTable[0x5E] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_NOP;
	instr.cycles = 2;
	InstrTable[0xEA] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 2;
	InstrTable[0x09] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 4;
	InstrTable[0x0D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 3;
	InstrTable[0x05] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 6;
	InstrTable[0x01] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 5;
	InstrTable[0x11] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 4;
	InstrTable[0x15] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 4;
	InstrTable[0x1D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 4;
	InstrTable[0x19] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 5;
	InstrTable[0x12] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PHA;
	instr.cycles = 3;
	InstrTable[0x48] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PHP;
	instr.cycles = 3;
	InstrTable[0x08] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PHX;
	instr.cycles = 3;
	InstrTable[0xDA] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PHY;
	instr.cycles = 3;
	InstrTable[0x5A] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PLA;
	instr.cycles = 4;
	InstrTable[0x68] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PLP;
	instr.cycles = 4;
	InstrTable[0x28] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PLX;
	instr.cycles = 4;
	InstrTable[0xFA] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PLY;
	instr.cycles = 4;
	InstrTable[0x7A] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ROL;
	instr.cycles = 6;
	InstrTable[0x2E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ROL;
	instr.cycles = 5;
	InstrTable[0x26] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_ROL_ACC;
	instr.cycles = 2;
	InstrTable[0x2A] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ROL;
	instr.cycles = 6;
	InstrTable[0x36] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ROL;
	instr.cycles = 7;
	InstrTable[0x3E] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ROR;
	instr.cycles = 6;
	InstrTable[0x6E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ROR;
	instr.cycles = 5;
	InstrTable[0x66] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_ROR_ACC;
	instr.cycles = 2;
	InstrTable[0x6A] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ROR;
	instr.cycles = 6;
	InstrTable[0x76] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ROR;
	instr.cycles = 7;
	InstrTable[0x7E] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_RTI;
	instr.cycles = 6;
	InstrTable[0x40] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_RTS;
	instr.cycles = 6;
	InstrTable[0x60] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 2;
	InstrTable[0xE9] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 4;
	InstrTable[0xED] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 3;
	InstrTable[0xE5] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 6;
	InstrTable[0xE1] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 5;
	InstrTable[0xF1] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 4;
	InstrTable[0xF5] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 4;
	InstrTable[0xFD] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 4;
	InstrTable[0xF9] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 5;
	InstrTable[0xF2] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_SEC;
	instr.cycles = 2;
	InstrTable[0x38] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_SED;
	instr.cycles = 2;
	InstrTable[0xF8] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_SEI;
	instr.cycles = 2;
	InstrTable[0x78] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 4;
	InstrTable[0x8D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 3;
	InstrTable[0x85] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 6;
	InstrTable[0x81] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 6;
	InstrTable[0x91] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 4;
	InstrTable[0x95] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 5;
	InstrTable[0x9D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 5;
	InstrTable[0x99] = instr;
	instr.addr = &mos6502::Addr_ZPI;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 5;
	InstrTable[0x92] = instr;

	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_STZ;
	instr.cycles = 3;
	InstrTable[0x64] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_STZ;
	instr.cycles = 4;
	InstrTable[0x74] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_STZ;
	instr.cycles = 4;
	InstrTable[0x9C] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_STZ;
	instr.cycles = 5;
	InstrTable[0x9E] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_STX;
	instr.cycles = 4;
	InstrTable[0x8E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_STX;
	instr.cycles = 3;
	InstrTable[0x86] = instr;
	instr.addr = &mos6502::Addr_ZEY;
	instr.code = &mos6502::Op_STX;
	instr.cycles = 4;
	InstrTable[0x96] = instr;

	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_STY;
	instr.cycles = 4;
	InstrTable[0x8C] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_STY;
	instr.cycles = 3;
	InstrTable[0x84] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_STY;
	instr.cycles = 4;
	InstrTable[0x94] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TAX;
	instr.cycles = 2;
	InstrTable[0xAA] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TAY;
	instr.cycles = 2;
	InstrTable[0xA8] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TSX;
	instr.cycles = 2;
	InstrTable[0xBA] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TXA;
	instr.cycles = 2;
	InstrTable[0x8A] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TXS;
	instr.cycles = 2;
	InstrTable[0x9A] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TYA;
	instr.cycles = 2;
	InstrTable[0x98] = instr;



	// new ones
	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_JSL;
	instr.cycles = 8;
	InstrTable[0x22] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_RTL;
	instr.cycles = 6;
	InstrTable[0x6B] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_REP;
	instr.cycles = 3;
	InstrTable[0xC2] = instr;

	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_SEP;
	instr.cycles = 3;
	InstrTable[0xE2] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_JMP;
	instr.cycles = 4;
	InstrTable[0x5C] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TXY;
	instr.cycles = 2;
	InstrTable[0x9B] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TYX;
	instr.cycles = 2;
	InstrTable[0xBB] = instr;

	instr.addr = &mos6502::Addr_ABI;
	instr.code = &mos6502::Op_JML;
	instr.cycles = 6;
	InstrTable[0xDC] = instr;

	instr.addr = &mos6502::Addr_AII;
	instr.code = &mos6502::Op_JMP;
	instr.cycles = 6;
	InstrTable[0x7C] = instr;

	instr.addr = &mos6502::Addr_AII;
	instr.code = &mos6502::Op_JSR;
	instr.cycles = 8;
	InstrTable[0xFC] = instr;

	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_XBA;
	instr.cycles = 3;
	InstrTable[0xEB] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_XCE;
	instr.cycles = 2;
	InstrTable[0xFB] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 5;
	InstrTable[0x0F] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_ORA;
	instr.cycles = 5;
	InstrTable[0x1F] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 5;
	InstrTable[0x2F] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_AND;
	instr.cycles = 5;
	InstrTable[0x3F] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 5;
	InstrTable[0x4F] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_EOR;
	instr.cycles = 5;
	InstrTable[0x5F] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 5;
	InstrTable[0x6F] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_ADC;
	instr.cycles = 5;
	InstrTable[0x7F] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 5;
	InstrTable[0x8F] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_STA;
	instr.cycles = 5;
	InstrTable[0x9F] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 5;
	InstrTable[0xAF] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_LDA;
	instr.cycles = 5;
	InstrTable[0xBF] = instr;
	
	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 5;
	InstrTable[0xCF] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_CMP;
	instr.cycles = 5;
	InstrTable[0xDF] = instr;

	instr.addr = &mos6502::Addr_ABL;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 5;
	InstrTable[0xEF] = instr;
	instr.addr = &mos6502::Addr_ALI;
	instr.code = &mos6502::Op_SBC;
	instr.cycles = 5;
	InstrTable[0xFF] = instr;


	Reset();

	return;
}

uint32_t mos6502::Addr_ACC()
{
	return 0; // not used
}

uint32_t mos6502::Addr_IMM()
{
	return pc++;
}

uint32_t mos6502::Addr_ABS()
{
	uint32_t addrL;
	uint32_t addrH;
	uint32_t addr;

	addrL = Read(pc++);
	addrH = Read(pc++);

	addr = addrL + (addrH << 8) + (pc & 0x00FF0000);

	return addr;
}

uint32_t mos6502::Addr_ZER()
{
	return Read(pc++);
}

uint32_t mos6502::Addr_IMP()
{
	return 0; // not used
}

uint32_t mos6502::Addr_REL()
{
	uint32_t offset;
	uint32_t addr;

	offset = (uint32_t)Read(pc++);
	if (offset & 0x80) offset |= 0xFF00;
	addr = pc + (int16_t)offset;
	return addr;
}

uint32_t mos6502::Addr_ABI()
{
	uint32_t addrL;
	uint32_t addrH;
	uint32_t effL;
	uint32_t effH;
	uint32_t abs;
	uint32_t addr;

	addrL = Read(pc++);
	addrH = Read(pc++);

	abs = (addrH << 8) + addrL + (pc & 0x00FF0000);

	effL = Read(abs);

#ifndef CMOS_INDIRECT_JMP_FIX
	effH = Read((abs & 0xFF00) + ((abs + 1) & 0x00FF) );
#else
	effH = Read(abs + 1);
#endif

	addr = effL + 0x100 * effH;

	return addr;
}

uint32_t mos6502::Addr_ZEX()
{
	uint32_t addr = (Read(pc++) + X) % 256;
	return addr;
}

uint32_t mos6502::Addr_ZEY()
{
	uint32_t addr = (Read(pc++) + Y) % 256;
	return addr;
}

uint32_t mos6502::Addr_ABX()
{
	uint32_t addr;
	uint32_t addrL;
	uint32_t addrH;

	addrL = Read(pc++);
	addrH = Read(pc++);

	addr = addrL + (addrH << 8) + X + (pc & 0x00FF0000);
	return addr;
}

uint32_t mos6502::Addr_ABY()
{
	uint32_t addr;
	uint32_t addrL;
	uint32_t addrH;

	addrL = Read(pc++);
	addrH = Read(pc++);

	addr = addrL + (addrH << 8) + Y + (pc & 0x00FF0000);
	return addr;
}


uint32_t mos6502::Addr_INX()
{
	uint32_t zeroL;
	uint32_t zeroH;
	uint32_t addr;

	zeroL = (Read(pc++) + X) % 256;
	zeroH = (zeroL + 1) % 256;
	addr = Read(zeroL) + (Read(zeroH) << 8);

	return addr;
}

uint32_t mos6502::Addr_INY()
{
	uint32_t zeroL;
	uint32_t zeroH;
	uint32_t addr;

	zeroL = Read(pc++);
	zeroH = (zeroL + 1) % 256;
	addr = Read(zeroL) + (Read(zeroH) << 8) + Y;

	return addr;
}

uint32_t mos6502::Addr_ZPI()
{
	uint32_t zeroL;
	uint32_t zeroH;
	uint32_t addr;

	zeroL = Read(pc++);
	zeroH = (zeroL + 1) % 256;
	addr = Read(zeroL) + (Read(zeroH) << 8);

	return addr;
}


// new ones
uint32_t mos6502::Addr_AII()
{
	uint32_t addrL;
	uint32_t addrH;
	uint32_t effL;
	uint32_t effH;
	uint32_t abs;
	uint32_t addr;

	addrL = Read(pc++);
	addrH = Read(pc++);

	abs = addrL + (addrH << 8) + X + (pc & 0x00FF0000);

	effL = Read(abs);

#ifndef CMOS_INDIRECT_JMP_FIX
	effH = Read((abs & 0xFF00) + ((abs + 1) & 0x00FF) );
#else
	effH = Read(abs + 1);
#endif

	addr = effL + 0x100 * effH;

	return addr;
}

uint32_t mos6502::Addr_ABL()
{
	uint32_t addrL;
	uint32_t addrH;
	uint32_t addrB;
	uint32_t addr;

	addrL = Read(pc++);
	addrH = Read(pc++);
	addrB = Read(pc++);

	addr = addrL + (addrH << 8) + (addrB << 16);

	return addr;
}

uint32_t mos6502::Addr_ALI()
{
	uint32_t addrL;
	uint32_t addrH;
	uint32_t addrB;
	uint32_t addr;

	addrL = Read(pc++);
	addrH = Read(pc++);
	addrB = Read(pc++);

	addr = addrL + (addrH << 8) + (addrB << 16) + X;

	return addr;
}



void mos6502::Reset()
{
	A = 0x00;
	Y = 0x00;
	X = 0x00;

	if (emulation_mode != 0x00)
	{
		pc = (Read(rstVectorEH) << 8) + Read(rstVectorEL); // load PC from reset vector
	}
	else 
	{
		pc = (Read(rstVectorNH) << 8) + Read(rstVectorNL); // load PC from reset vector
	}

	sp = 0xFD;

	status |= CONSTANT;

	illegalOpcode = false;
	waiting = false;

	return;
}

void mos6502::StackPush(uint8_t byte)
{
	Write(0x0100 + sp, byte);
	if(sp == 0x00) sp = 0xFF;
	else sp--;
}

uint8_t mos6502::StackPop()
{
	if(sp == 0xFF) sp = 0x00;
	else sp++;
	return Read(0x0100 + sp);
}

void mos6502::IRQ()
{
	irq_line = true;
	waiting = false;
	if(!IF_INTERRUPT())
	{
		SET_BREAK(0);
		StackPush((pc >> 8) & 0xFF);
		StackPush(pc & 0xFF);
		StackPush(status);
		SET_INTERRUPT(1);
		if (emulation_mode != 0x00)
		{
			pc = (Read(irqVectorEH) << 8) + Read(irqVectorEL);
		}
		else
		{
			pc = (Read(irqVectorNH) << 8) + Read(irqVectorNL);
		}
	}
	return;
}

void mos6502::ScheduleIRQ(uint32_t cycles) {
	irq_timer = cycles;
	if(cycles == 0) {
		IRQ();
	}
}

void mos6502::ClearIRQ() {
	irq_line = false;
	irq_timer = 0;
}

void mos6502::NMI()
{
	waiting = false;
	SET_BREAK(0);
	StackPush((pc >> 8) & 0xFF);
	StackPush(pc & 0xFF);
	StackPush(status);
	SET_INTERRUPT(1);
	if (emulation_mode != 0x00)
	{
		pc = (Read(nmiVectorEH) << 8) + Read(nmiVectorEL);
	}
	else
	{
		pc = (Read(nmiVectorNH) << 8) + Read(nmiVectorNL);
	}
	return;
}

void mos6502::Run(
	int32_t cyclesRemaining,
	uint64_t& cycleCount,
	CycleMethod cycleMethod
) {
	uint8_t opcode;
	Instr instr;

	while(cyclesRemaining > 0 && !illegalOpcode)
	{
		if(waiting) {
			if(irq_line) {
				waiting = false;
				IRQ();
			} else if(irq_timer > 0) {
				cycleCount += irq_timer;
				cyclesRemaining -= irq_timer;
				irq_timer = 0;
				irq_line = true;
				IRQ();
			} else {
				break;
			}
		} else if(irq_line) {
			IRQ();
		}
		// fetch
		if(Sync == NULL) {
			opcode = Read(pc++);
		} else {
			opcode = Sync(pc++);
		}
		

		// decode
		instr = InstrTable[opcode];

		// execute
		Exec(instr);
		if(illegalOpcode) {
			illegalOpcodeSrc = opcode;
		}
		cycleCount += instr.cycles;
		cyclesRemaining -=
			cycleMethod == CYCLE_COUNT        ? instr.cycles
			/* cycleMethod == INST_COUNT */   : 1;
		if(irq_timer > 0) {
			if(irq_timer < instr.cycles) {
				irq_timer = 0;
				IRQ();
			} else {
				irq_timer -= instr.cycles;
			}
			if(irq_timer == 0) {
				IRQ();
			}
		}
	}
}

void mos6502::Exec(Instr i)
{
	uint32_t src = (this->*i.addr)();
	(this->*i.code)(src);
}

void mos6502::Op_ILLEGAL(uint32_t src)
{
	illegalOpcode = true;
}


void mos6502::Op_ADC(uint32_t src)
{
	uint8_t m = Read(src);
	unsigned int tmp = m + A + (IF_CARRY() ? 1 : 0);
	SET_ZERO(!(tmp & 0xFF));
	if (IF_DECIMAL())
	{
		if (((A & 0xF) + (m & 0xF) + (IF_CARRY() ? 1 : 0)) > 9) tmp += 6;
		SET_NEGATIVE(tmp & 0x80);
		SET_OVERFLOW(!((A ^ m) & 0x80) && ((A ^ tmp) & 0x80));
		if (tmp > 0x99)
		{
			tmp += 96;
		}
		SET_CARRY(tmp > 0x99);
	}
	else
	{
		SET_NEGATIVE(tmp & 0x80);
		SET_OVERFLOW(!((A ^ m) & 0x80) && ((A ^ tmp) & 0x80));
		SET_CARRY(tmp > 0xFF);
	}

	A = tmp & 0xFF;
	return;
}



void mos6502::Op_AND(uint32_t src)
{
	uint8_t m = Read(src);
	uint8_t res = m & A;
	SET_NEGATIVE(res & 0x80);
	SET_ZERO(!res);
	A = res;
	return;
}


void mos6502::Op_ASL(uint32_t src)
{
	uint8_t m = Read(src);
	SET_CARRY(m & 0x80);
	m <<= 1;
	m &= 0xFF;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Write(src, m);
	return;
}

void mos6502::Op_ASL_ACC(uint32_t src)
{
	uint8_t m = A;
	SET_CARRY(m & 0x80);
	m <<= 1;
	m &= 0xFF;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
	return;
}

void mos6502::Op_BCC(uint32_t src)
{
	if (!IF_CARRY())
	{
		pc = src;
	}
	return;
}


void mos6502::Op_BCS(uint32_t src)
{
	if (IF_CARRY())
	{
		pc = src;
	}
	return;
}

void mos6502::Op_BEQ(uint32_t src)
{
	if (IF_ZERO())
	{
		pc = src;
	}
	return;
}

void mos6502::Op_BIT(uint32_t src)
{
	uint8_t m = Read(src);
	uint8_t res = m & A;
	SET_NEGATIVE(res & 0x80);
	status = (status & 0x3F) | (uint8_t)(m & 0xC0);
	SET_ZERO(!res);
	return;
}

void mos6502::Op_BMI(uint32_t src)
{
	if (IF_NEGATIVE())
	{
		pc = src;
	}
	return;
}

void mos6502::Op_BNE(uint32_t src)
{
	if (!IF_ZERO())
	{
		pc = src;
	}
	return;
}

void mos6502::Op_BPL(uint32_t src)
{
	if (!IF_NEGATIVE())
	{
		pc = src;
	}
	return;
}

void mos6502::Op_BRK(uint32_t src)
{
	pc++;
	if (emulation_mode == 0x00)
	{
		StackPush((pc >> 16) & 0xFF);
	}
	StackPush((pc >> 8) & 0xFF);
	StackPush(pc & 0xFF);
	StackPush(status | BREAK);
	SET_INTERRUPT(1);
	if (emulation_mode != 0x00)
	{
		pc = (Read(irqVectorEH) << 8) + Read(irqVectorEL);
	}
	else
	{
		pc = (Read(irqVectorNH) << 8) + Read(irqVectorNL);
	}
	return;
}

void mos6502::Op_WAI(uint32_t src)
{
	waiting = true;
}

void mos6502::Op_STP(uint32_t src)
{
	illegalOpcode = true;
	Stopped();
}

void mos6502::Op_BVC(uint32_t src)
{
	if (!IF_OVERFLOW())
	{
		pc = src;
	}
	return;
}

void mos6502::Op_BVS(uint32_t src)
{
	if (IF_OVERFLOW())
	{
		pc = src;
	}
	return;
}

void mos6502::Op_CLC(uint32_t src)
{
	SET_CARRY(0);
	return;
}

void mos6502::Op_CLD(uint32_t src)
{
	SET_DECIMAL(0);
	return;
}

void mos6502::Op_CLI(uint32_t src)
{
	SET_INTERRUPT(0);
	return;
}

void mos6502::Op_CLV(uint32_t src)
{
	SET_OVERFLOW(0);
	return;
}

void mos6502::Op_CMP(uint32_t src)
{
	unsigned int tmp = A - Read(src);
	SET_CARRY(tmp < 0x100);
	SET_NEGATIVE(tmp & 0x80);
	SET_ZERO(!(tmp & 0xFF));
	return;
}

void mos6502::Op_CPX(uint32_t src)
{
	unsigned int tmp = X - Read(src);
	SET_CARRY(tmp < 0x100);
	SET_NEGATIVE(tmp & 0x80);
	SET_ZERO(!(tmp & 0xFF));
	return;
}

void mos6502::Op_CPY(uint32_t src)
{
	unsigned int tmp = Y - Read(src);
	SET_CARRY(tmp < 0x100);
	SET_NEGATIVE(tmp & 0x80);
	SET_ZERO(!(tmp & 0xFF));
	return;
}

void mos6502::Op_DEC(uint32_t src)
{
	uint8_t m = Read(src);
	m = (m - 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Write(src, m);
	return;
}

void mos6502::Op_DEC_ACC(uint32_t src)
{
	uint8_t m = A;
	m = (m - 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
	return;
}

void mos6502::Op_DEX(uint32_t src)
{
	uint8_t m = X;
	m = (m - 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	X = m;
	return;
}

void mos6502::Op_DEY(uint32_t src)
{
	uint8_t m = Y;
	m = (m - 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Y = m;
	return;
}

void mos6502::Op_EOR(uint32_t src)
{
	uint8_t m = Read(src);
	m = A ^ m;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
}

void mos6502::Op_INC(uint32_t src)
{
	uint8_t m = Read(src);
	m = (m + 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Write(src, m);
}

void mos6502::Op_INC_ACC(uint32_t src)
{
	uint8_t m = A;
	m = (m + 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
}

void mos6502::Op_INX(uint32_t src)
{
	uint8_t m = X;
	m = (m + 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	X = m;
}

void mos6502::Op_INY(uint32_t src)
{
	uint8_t m = Y;
	m = (m + 1) % 256;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Y = m;
}

void mos6502::Op_JMP(uint32_t src)
{
	pc = src;
}

void mos6502::Op_JSR(uint32_t src)
{
	pc--;
	StackPush((pc >> 8) & 0xFF);
	StackPush(pc & 0xFF);
	pc = (pc & 0xFFFF0000) + (src & 0x0000FFFF);
}

void mos6502::Op_LDA(uint32_t src)
{
	uint8_t m = Read(src);
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
}

void mos6502::Op_LDX(uint32_t src)
{
	uint8_t m = Read(src);
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	X = m;
}

void mos6502::Op_LDY(uint32_t src)
{
	uint8_t m = Read(src);
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Y = m;
}

void mos6502::Op_LSR(uint32_t src)
{
	uint8_t m = Read(src);
	SET_CARRY(m & 0x01);
	m >>= 1;
	SET_NEGATIVE(0);
	SET_ZERO(!m);
	Write(src, m);
}

void mos6502::Op_LSR_ACC(uint32_t src)
{
	uint8_t m = A;
	SET_CARRY(m & 0x01);
	m >>= 1;
	SET_NEGATIVE(0);
	SET_ZERO(!m);
	A = m;
}

void mos6502::Op_NOP(uint32_t src)
{
	return;
}

void mos6502::Op_ORA(uint32_t src)
{
	uint8_t m = Read(src);
	m = A | m;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
}

void mos6502::Op_PHA(uint32_t src)
{
	StackPush(A);
	return;
}

void mos6502::Op_PHP(uint32_t src)
{
	StackPush(status | BREAK);
	return;
}

void mos6502::Op_PHX(uint32_t src)
{
	StackPush(X);
	return;
}

void mos6502::Op_PHY(uint32_t src)
{
	StackPush(Y);
	return;
}

void mos6502::Op_PLA(uint32_t src)
{
	A = StackPop();
	SET_NEGATIVE(A & 0x80);
	SET_ZERO(!A);
	return;
}

void mos6502::Op_PLP(uint32_t src)
{
	status = StackPop();
	SET_CONSTANT(1);
	return;
}

void mos6502::Op_PLX(uint32_t src)
{
	X = StackPop();
	SET_NEGATIVE(X & 0x80);
	SET_ZERO(!X);
	return;
}

void mos6502::Op_PLY(uint32_t src)
{
	Y = StackPop();
	SET_NEGATIVE(Y & 0x80);
	SET_ZERO(!Y);
	return;
}

void mos6502::Op_ROL(uint32_t src)
{
	uint32_t m = Read(src);
	m <<= 1;
	if (IF_CARRY()) m |= 0x01;
	SET_CARRY(m > 0xFF);
	m &= 0xFF;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Write(src, m);
	return;
}

void mos6502::Op_ROL_ACC(uint32_t src)
{
	uint32_t m = A;
	m <<= 1;
	if (IF_CARRY()) m |= 0x01;
	SET_CARRY(m > 0xFF);
	m &= 0xFF;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
	return;
}

void mos6502::Op_ROR(uint32_t src)
{
	uint32_t m = Read(src);
	if (IF_CARRY()) m |= 0x100;
	SET_CARRY(m & 0x01);
	m >>= 1;
	m &= 0xFF;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Write(src, m);
	return;
}

void mos6502::Op_ROR_ACC(uint32_t src)
{
	uint32_t m = A;
	if (IF_CARRY()) m |= 0x100;
	SET_CARRY(m & 0x01);
	m >>= 1;
	m &= 0xFF;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
	return;
}

void mos6502::Op_RTI(uint32_t src)
{
	uint8_t lo, hi, bk;

	status = StackPop();

	lo = StackPop();
	hi = StackPop();
	if (emulation_mode == 0x00)
	{
		bk = StackPop();
	}

	pc = (bk << 16) + (hi << 8) + lo;
	return;
}

void mos6502::Op_RTS(uint32_t src)
{
	uint8_t lo, hi;

	lo = StackPop();
	hi = StackPop();

	pc = (pc & 0xFFFF0000) + (((hi << 8) | lo) & 0x0000FFFF) + 1;
	return;
}

void mos6502::Op_SBC(uint32_t src)
{
	uint8_t m = Read(src);
	unsigned int tmp = A - m - (IF_CARRY() ? 0 : 1);
	SET_NEGATIVE(tmp & 0x80);
	SET_ZERO(!(tmp & 0xFF));
	SET_OVERFLOW(((A ^ tmp) & 0x80) && ((A ^ m) & 0x80));

	if (IF_DECIMAL())
	{
		if ( ((A & 0x0F) - (IF_CARRY() ? 0 : 1)) < (m & 0x0F)) tmp -= 6;
		if (tmp > 0x99)
		{
			tmp -= 0x60;
		}
	}
	SET_CARRY(tmp < 0x100);
	A = (tmp & 0xFF);
	return;
}

void mos6502::Op_SEC(uint32_t src)
{
	SET_CARRY(1);
	return;
}

void mos6502::Op_SED(uint32_t src)
{
	SET_DECIMAL(1);
	return;
}

void mos6502::Op_SEI(uint32_t src)
{
	SET_INTERRUPT(1);
	return;
}

void mos6502::Op_STA(uint32_t src)
{
	Write(src, A);
	return;
}

void mos6502::Op_STZ(uint32_t src)
{
	Write(src, 0);
	return;
}

void mos6502::Op_STX(uint32_t src)
{
	Write(src, X);
	return;
}

void mos6502::Op_STY(uint32_t src)
{
	Write(src, Y);
	return;
}

void mos6502::Op_TAX(uint32_t src)
{
	uint8_t m = A;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	X = m;
	return;
}

void mos6502::Op_TAY(uint32_t src)
{
	uint8_t m = A;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Y = m;
	return;
}

void mos6502::Op_TSX(uint32_t src)
{
	uint8_t m = sp;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	X = m;
	return;
}

void mos6502::Op_TXA(uint32_t src)
{
	uint8_t m = X;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
	return;
}

void mos6502::Op_TXS(uint32_t src)
{
	sp = X;
	return;
}

void mos6502::Op_TYA(uint32_t src)
{
	uint8_t m = Y;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	A = m;
	return;
}

void mos6502::Op_BRA(uint32_t src)
{
	pc = src;
	return;
}


// new ones
void mos6502::Op_JSL(uint32_t src)
{
	pc--;
	StackPush((pc >> 16) & 0xFF);
	StackPush((pc >> 8) & 0xFF);
	StackPush(pc & 0xFF);
	pc = src;
	return;
}

void mos6502::Op_RTL(uint32_t src)
{
	uint8_t lo, hi, bk;

	lo = StackPop();
	hi = StackPop();
	bk = StackPop();

	pc = ((bk << 16) + (hi << 8) + lo) + 1;
	return;
}

void mos6502::Op_REP(uint32_t src)
{
	// do stuff here
	return;
}

void mos6502::Op_SEP(uint32_t src)
{
	// do stuff here
	return;
}

void mos6502::Op_TXY(uint32_t src)
{
	uint8_t m = X;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	Y = m;
	return;
}

void mos6502::Op_TYX(uint32_t src)
{
	uint8_t m = Y;
	SET_NEGATIVE(m & 0x80);
	SET_ZERO(!m);
	X = m;
	return;
}

void mos6502::Op_XBA(uint32_t src)
{
	// do stuff here
	return;
}

void mos6502::Op_XCE(uint32_t src)
{
	if (IF_CARRY())
	{
		emulation_mode = 0x01;
	}
	else
	{
		emulation_mode = 0x00;
	}
	return;
}

void mos6502::Op_JML(uint32_t src)
{
	// do stuff here
}



