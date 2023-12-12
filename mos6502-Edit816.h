//============================================================================
// Name        : mos6502
// Author      : Gianluca Ghettini
// Version     : 1.0
// Copyright   :
// Description : A MOS 6502 CPU emulator written in C++
//============================================================================

// This has been altered by Agumander (Clyde Shaffer) and by Steven Chad Burrow

#pragma once
#include <iostream>
#include <stdint.h>
using namespace std;

#define NEGATIVE  0x80
#define OVERFLOW  0x40
#define CONSTANT  0x20
#define BREAK     0x10
#define DECIMAL   0x08
#define INTERRUPT 0x04
#define ZERO      0x02
#define CARRY     0x01

#define SET_NEGATIVE(x) (x ? (status |= NEGATIVE) : (status &= (~NEGATIVE)) )
#define SET_OVERFLOW(x) (x ? (status |= OVERFLOW) : (status &= (~OVERFLOW)) )
#define SET_CONSTANT(x) (x ? (status |= CONSTANT) : (status &= (~CONSTANT)) )
#define SET_BREAK(x) (x ? (status |= BREAK) : (status &= (~BREAK)) )
#define SET_DECIMAL(x) (x ? (status |= DECIMAL) : (status &= (~DECIMAL)) )
#define SET_INTERRUPT(x) (x ? (status |= INTERRUPT) : (status &= (~INTERRUPT)) )
#define SET_ZERO(x) (x ? (status |= ZERO) : (status &= (~ZERO)) )
#define SET_CARRY(x) (x ? (status |= CARRY) : (status &= (~CARRY)) )

#define IF_NEGATIVE() ((status & NEGATIVE) ? true : false)
#define IF_OVERFLOW() ((status & OVERFLOW) ? true : false)
#define IF_CONSTANT() ((status & CONSTANT) ? true : false)
#define IF_BREAK() ((status & BREAK) ? true : false)
#define IF_DECIMAL() ((status & DECIMAL) ? true : false)
#define IF_INTERRUPT() ((status & INTERRUPT) ? true : false)
#define IF_ZERO() ((status & ZERO) ? true : false)
#define IF_CARRY() ((status & CARRY) ? true : false)



class mos6502
{
private:
	mos6502()
	{
		emulation_mode = 0x01;
	}

	typedef void (mos6502::*CodeExec)(uint32_t);
	typedef uint32_t (mos6502::*AddrExec)();

	typedef struct Instr
	{
		AddrExec addr;
		CodeExec code;
		uint8_t cycles;
	};

	Instr InstrTable[256];

	void Exec(Instr i);

	// addressing modes
	uint32_t Addr_ACC(); // ACCUMULATOR A
	uint32_t Addr_IMM(); // IMMEDIATE #
	uint32_t Addr_ABS(); // ABSOLUTE a
	uint32_t Addr_ZER(); // ZERO PAGE d
	uint32_t Addr_ZEX(); // INDEXED-X ZERO PAGE d,x
	uint32_t Addr_ZEY(); // INDEXED-Y ZERO PAGE d,y
	uint32_t Addr_ABX(); // INDEXED-X ABSOLUTE a,x
	uint32_t Addr_ABY(); // INDEXED-Y ABSOLUTE a,y
	uint32_t Addr_IMP(); // IMPLIED i
	uint32_t Addr_REL(); // RELATIVE r
	uint32_t Addr_INX(); // INDEXED-X INDIRECT (d,x)
	uint32_t Addr_INY(); // INDEXED-Y INDIRECT (d),y
	uint32_t Addr_ABI(); // ABSOLUTE INDIRECT (a)
	uint32_t Addr_ZPI(); // ZERO PAGE INDIRECT (d)

	// new ones
	uint32_t Addr_AII(); // absolute indexed indirect (a,x)
	uint32_t Addr_ABL(); // absolute long al
	uint32_t Addr_ALI(); // absolute long indexed al,x
	// block move xyc
	// direct indirect long indexed [d],y
	// direct indirect long [d]
	// relative long rl
	// stack relative d,s
	// stack relative indirect indexed (d,s),y

	// opcodes (grouped as per datasheet)
	void Op_ADC(uint32_t src);
	void Op_AND(uint32_t src);
	void Op_ASL(uint32_t src); 	void Op_ASL_ACC(uint32_t src);
	void Op_BCC(uint32_t src);
	void Op_BCS(uint32_t src);

	void Op_BEQ(uint32_t src);
	void Op_BIT(uint32_t src);
	void Op_BMI(uint32_t src);
	void Op_BNE(uint32_t src);
	void Op_BPL(uint32_t src);

	void Op_BRK(uint32_t src);
	void Op_BVC(uint32_t src);
	void Op_BVS(uint32_t src);
	void Op_CLC(uint32_t src);
	void Op_CLD(uint32_t src);

	void Op_CLI(uint32_t src);
	void Op_CLV(uint32_t src);
	void Op_CMP(uint32_t src);
	void Op_CPX(uint32_t src);
	void Op_CPY(uint32_t src);

	void Op_DEC(uint32_t src);	void Op_DEC_ACC(uint32_t src);
	void Op_DEX(uint32_t src);
	void Op_DEY(uint32_t src);
	void Op_EOR(uint32_t src);
	void Op_INC(uint32_t src);	void Op_INC_ACC(uint32_t src);

	void Op_INX(uint32_t src);
	void Op_INY(uint32_t src);
	void Op_JMP(uint32_t src);
	void Op_JSR(uint32_t src);
	void Op_LDA(uint32_t src);

	void Op_LDX(uint32_t src);
	void Op_LDY(uint32_t src);
	void Op_LSR(uint32_t src); 	void Op_LSR_ACC(uint32_t src);
	void Op_NOP(uint32_t src);
	void Op_ORA(uint32_t src);

	void Op_PHA(uint32_t src);
	void Op_PHP(uint32_t src);
	void Op_PHX(uint32_t src);
	void Op_PHY(uint32_t src);
	void Op_PLA(uint32_t src);
	void Op_PLP(uint32_t src);
	void Op_PLX(uint32_t src);
	void Op_PLY(uint32_t src);
	void Op_ROL(uint32_t src); 	void Op_ROL_ACC(uint32_t src);

	void Op_ROR(uint32_t src);	void Op_ROR_ACC(uint32_t src);
	void Op_RTI(uint32_t src);
	void Op_RTS(uint32_t src);
	void Op_SBC(uint32_t src);
	void Op_SEC(uint32_t src);
	void Op_SED(uint32_t src);

	void Op_SEI(uint32_t src);
	void Op_STA(uint32_t src);
	void Op_STZ(uint32_t src);
	void Op_STX(uint32_t src);
	void Op_STY(uint32_t src);
	void Op_TAX(uint32_t src);

	void Op_TAY(uint32_t src);
	void Op_TSX(uint32_t src);
	void Op_TXA(uint32_t src);
	void Op_TXS(uint32_t src);
	void Op_TYA(uint32_t src);

	void Op_WAI(uint32_t src);
	void Op_STP(uint32_t src);
	void Op_BRA(uint32_t src);

	// new ones
	void Op_JSL(uint32_t src);
	void Op_REP(uint32_t src);
	void Op_SEP(uint32_t src);	
	void Op_RTL(uint32_t src);
	void Op_TXY(uint32_t src);
	void Op_TYX(uint32_t src);
	void Op_JML(uint32_t src);
	void Op_XBA(uint32_t src);	
	void Op_XCE(uint32_t src);
	

	void Op_ILLEGAL(uint32_t src);

	// IRQ, reset, NMI vectors
	static const uint32_t irqVectorEH = 0x0000FFFF;
	static const uint32_t irqVectorEL = 0x0000FFFE;
	static const uint32_t rstVectorEH = 0x0000FFFD;
	static const uint32_t rstVectorEL = 0x0000FFFC;
	static const uint32_t nmiVectorEH = 0x0000FFFB;
	static const uint32_t nmiVectorEL = 0x0000FFFA;

	static const uint32_t irqVectorNH = 0x0000FEFF;
	static const uint32_t irqVectorNL = 0x0000FEFE;
	static const uint32_t rstVectorNH = 0x0000FFFD; // reset is same as emulation version
	static const uint32_t rstVectorNL = 0x0000FFFC;
	static const uint32_t nmiVectorNH = 0x0000FEFB;
	static const uint32_t nmiVectorNL = 0x0000FEFA;

	// read/write callbacks
	typedef void (*CPUEvent)(void);
	typedef void (*BusWrite)(uint32_t, uint8_t);
	typedef uint8_t (*BusRead)(uint32_t);
	BusRead Read;
	BusWrite Write;
	CPUEvent Stopped;
	BusRead Sync;

	// stack operations
	inline void StackPush(uint8_t byte);
	inline uint8_t StackPop();

	uint32_t irq_timer;
	bool irq_line = false;

public:
	bool illegalOpcode;
	bool waiting;
	uint32_t illegalOpcodeSrc;

	// registers
	uint8_t A; // accumulator
	uint8_t X; // X-index
	uint8_t Y; // Y-index

	// stack pointer
	uint8_t sp;

	// program counter
	uint32_t pc;

	// status register
	uint8_t status;
	
	enum CycleMethod {
		INST_COUNT,
		CYCLE_COUNT,
	};
	mos6502(BusRead r, BusWrite w, CPUEvent stp, BusRead sync = NULL);
	void NMI();
	void IRQ();
	void ScheduleIRQ(uint32_t cycles);
	void ClearIRQ();
	void Reset();
	void Run(
		int32_t cycles,
		uint64_t& cycleCount,
		CycleMethod cycleMethod = CYCLE_COUNT);

	// new ones
	uint8_t emulation_mode;
};
