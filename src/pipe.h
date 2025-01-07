/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 * 
 * Collaborators:
 * Martin Chapman (chapmanmartind)
 * Mukerrem Tufekcioglu (mukerrem)
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"
#include "stdbool.h"
#include <limits.h>


typedef struct CPU_State {
	/* register file state */
	int64_t REGS[ARM_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */

	/* program counter in fetch stage */
	uint64_t PC;
	
} CPU_State;

int RUN_BIT;

/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;

typedef enum operationName_t {
    
    NoOp,
    ADD,
    ADDI,
    ADDS,
    ADDSI,
    STUR,
    STURB,
    STURH,
    AND,
    ANDS,
    EOR,
    ORR,
    LDUR,
    LDURB,
    LDURH,
    LSLI,
    LSRI,
    BR,
    SUB,
    SUBI,
    SUBS,
    CMP,
    SUBSI,
    CMPI,
    HLT,
    MUL,
    CBNZ,
    CBZ,
    MOVZ,
    B,
    BEQ,
    BNE,
    BGE,
    BLT,
    BGT,
    BLE

} operationName_t;

typedef struct fetchStruct_t {
 
    uint64_t PC;
    uint64_t instructionCode;
    uint64_t pred_address;
    uint64_t pred_taken;

} fetchStruct_t;

typedef struct decodeStruct_t {
    
    uint64_t instructionCode;
    operationName_t operationName;
    uint64_t PC;
    uint64_t opcode;
    uint64_t Rd;
    uint64_t Rn;
    uint64_t Rm;
    uint64_t Rt;
    int64_t RnValForwarded;
    int64_t RmValForwarded;
    uint64_t RtValForwarded;
    uint64_t imm;
    uint64_t imm19;
    int64_t signImm19;
    int64_t signedImm19;
    uint64_t imm9;
    int64_t signImm9;
    int64_t signedImm9;
    uint64_t pred_address;
    uint64_t pred_taken;

} decodeStruct_t;

typedef struct executeStruct_t {
    
    uint64_t instructionCode;
    operationName_t operationName;
    uint64_t PCInitial;
    uint64_t PCFinal;
    uint64_t opcode;
    int32_t cmpResult;          //FOR CMP
    
    uint64_t Rd;
    uint64_t Rn;
    uint64_t Rm;
    uint64_t Rt;

    int64_t RdVal;
    int64_t RnVal;
    int64_t RmVal;
    uint64_t RtVal;
   
    int storeValLow;
    int storeValHigh;

    uint32_t loadValLow;
    uint32_t loadValHigh;

    uint64_t storeAddress;
    uint64_t loadAddress;

    uint64_t pred_address;
    uint64_t pred_taken;

} executeStruct_t;

typedef struct memoryStruct_t {
    
    uint64_t instructionCode;
    operationName_t operationName;
    uint64_t PCInitial;
    uint64_t PCFinal;
    uint64_t opcode;
    int32_t cmpResult;          //the result of the operation
    
    uint64_t Rd;
    uint64_t Rn;
    uint64_t Rm;
    uint64_t Rt;

    int64_t RdVal;
    int64_t RnVal;
    int64_t RmVal;
    uint64_t RtVal;
  
    int storeValLow;
    int storeValHigh;

    uint32_t loadValLow;
    uint32_t loadValHigh;

    uint64_t storeAddress;
    uint64_t loadAddress;

} memoryStruct_t;

typedef struct IFtoID_BP_t {
    uint64_t pred_address;
    uint64_t pred_taken;
} IFtoID_BP_t;

typedef struct IDtoEX_BP_t {
    uint64_t pred_address;
    uint64_t pred_taken;
} IDtoEX_BP_t;

typedef struct EXtoMEM_BP_t {
    int flush;
} EXtoMEM_BP_t;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

#endif
