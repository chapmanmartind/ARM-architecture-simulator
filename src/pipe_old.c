/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 * 
 * Collaborators:
 * Martin Chapman (chapmanmartind)
 * Mukerrem Tufekcioglu (mukerrem)
 */

#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bp.h"

/* global pipeline state */
CPU_State CURRENT_STATE;

fetchStruct_t fetchStruct;
decodeStruct_t decodeStruct;
executeStruct_t executeStruct;
memoryStruct_t memoryStruct;

IFtoID_BP_t* IFtoID_BP;
IDtoEX_BP_t* IDtoEX_BP;
EXtoMEM_BP_t* EXtoMEM_BP;

bp_t* branch_predictor;

int HaltFlag = 0;

int RNFLAG = 0;
int RMFLAG = 0;
int RTFLAG = 0;

int StallFlag = 0;

int BFlag;
int CBFlag;

int CBTaken;
int CBNotTaken;

int cycleCounter = 0;

int temp_FLAG_Z = 0;
int temp_FLAG_N = 0;

int flushed = 0;

void pipe_init()
{
    memset(&CURRENT_STATE, 0, sizeof(CPU_State));
    CURRENT_STATE.PC = 0x00400000;
    IFtoID_BP  = malloc(sizeof(IFtoID_BP_t));
    IDtoEX_BP  = malloc(sizeof(IDtoEX_BP_t));
    EXtoMEM_BP = malloc(sizeof(EXtoMEM_BP_t));
    branch_predictor = malloc(sizeof(bp_t));
    branch_predictor->GHR = 0;
    for (int i = 0; i < 256; i++) { branch_predictor->PHT[i] = 0; }
    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < 4; j++) {
            branch_predictor->BTB[i][j] = 0;
        }
    }
}

void set_flags(int result) {
    if (result == 0) {
        CURRENT_STATE.FLAG_Z = 1;
        temp_FLAG_Z = 1;
    } else {
        CURRENT_STATE.FLAG_Z = 0;
        temp_FLAG_Z = 0;
    }
    if (result < 0) {
        CURRENT_STATE.FLAG_N = 1;
        temp_FLAG_N = 1;
    } else {
        CURRENT_STATE.FLAG_N = 0;
        temp_FLAG_N = 0;
    }
}

void set_temp_flags(int result) {
    if (result == 0) {
        temp_FLAG_Z = 1;
    } else {
        temp_FLAG_Z = 0;
    }
    if (result < 0) {
        temp_FLAG_N = 1;
    } else {
        temp_FLAG_N = 0;
    }
}


void check_dependency_wb() 
{
    if (memoryStruct.RdVal) 
    {
        if (decodeStruct.Rn == memoryStruct.Rd) {
            RNFLAG = 1;
            decodeStruct.RnValForwarded = memoryStruct.RdVal; 

        }
        if (decodeStruct.Rm == memoryStruct.Rd) {
            RMFLAG = 1;
            decodeStruct.RmValForwarded = memoryStruct.RdVal;

        }
        if (decodeStruct.Rt == memoryStruct.Rd) {
            RTFLAG = 1;
            decodeStruct.RtValForwarded = memoryStruct.RdVal;
        }
    }
}

void check_dependency_mem() 
{
    if (memoryStruct.RdVal) 
    {
        if (decodeStruct.Rn == executeStruct.Rd) {
            //if (!RNFLAG) {   
            RNFLAG = 1;
            decodeStruct.RnValForwarded = executeStruct.RdVal;
            //}
        }
        if (decodeStruct.Rm == executeStruct.Rd) {
            //if (!RMFLAG) {
                RMFLAG = 1;
                decodeStruct.RmValForwarded = executeStruct.RdVal;
            //}
        }
        
        if (decodeStruct.Rt == executeStruct.Rd) {
            //if (!RTFLAG) {
                RTFLAG = 1;
                decodeStruct.RtValForwarded = executeStruct.RdVal;
            //}
        }
    }
}

void pipe_stage_wb() 
{
    printf("%s\n", "WB");
    uint64_t Rd = memoryStruct.Rd;
    uint64_t Rt = memoryStruct.Rt;
    int RdVal = memoryStruct.RdVal;
    uint32_t cmpResult = memoryStruct.cmpResult; 
    printf("OPERATION NAME: %i\n", memoryStruct.operationName);
    switch (memoryStruct.operationName) {
        
        case ADD: ; // ADD
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case ADDI: ; // ADD immediate
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case ADDS: ; // ADDS
            CURRENT_STATE.REGS[Rd] = RdVal;
            set_flags(RdVal);  
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case ADDSI: ; // ADDS immediate
            CURRENT_STATE.REGS[Rd] = RdVal;
            set_flags(RdVal);
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case STUR: ;// STUR 64 bits
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case STURB: ;// STURB
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case STURH: ;// STURH
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case AND: ;// AND
            CURRENT_STATE.REGS[Rd] = RdVal;
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case ANDS: ;// ANDS
            CURRENT_STATE.REGS[Rd] = RdVal;
            set_flags(RdVal);
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case EOR: ;// EOR
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case ORR: ;// ORR
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case LDUR: ;// LDUR 64 bit
            CURRENT_STATE.REGS[Rt] = memoryStruct.RtVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case LDURB: ;// LDURB
            CURRENT_STATE.REGS[Rt] = memoryStruct.RtVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case LDURH: ;// LDURH
            CURRENT_STATE.REGS[Rt] = memoryStruct.RtVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case LSLI: ;// LSL Immediate and LSR Immediate
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case LSRI: ;// LSL Immediate and LSR Immediate
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case BR: ;// BR
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case SUB: // SUB
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case SUBI: ; //SUB Immediate
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case SUBS: ;// SUBS -- AKA CMP if Rt = 0x11111
            CURRENT_STATE.REGS[Rd] = RdVal; 
            set_flags(RdVal);
            stat_inst_retire = stat_inst_retire + 1;
            break;
        
        case CMP: ;// SUBS -- AKA CMP if Rt = 0x11111
            //TBD 
            set_flags(cmpResult); 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case SUBSI: ; //SUBSI Immediate --  AKA CMPI if Rt = 0b11111
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case CMPI: ; //SUBSI Immediate --  AKA CMPI if Rt = 0b11111
            //TBD
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case HLT: ;//HLT
            //TBD 
            printf("(WB) HLT\n");
            RUN_BIT = 0; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case MUL: //MUL
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case CBNZ: ; // CBNZ
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case CBZ: ;// CBZ
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case MOVZ: ;// MOVZ
            CURRENT_STATE.REGS[Rd] = RdVal; 
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case B: ;// B
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case BEQ: ;// BEQ
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case BNE: ;//BNE
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case BGE: ;//BGE
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case BLT: ;//BLT
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case BGT: ;//BGT
            stat_inst_retire = stat_inst_retire + 1;
            break;

        case BLE: ;//BL
            stat_inst_retire = stat_inst_retire + 1; 
            break;
    }

    check_dependency_wb();
}

void clearMemoryStruct() {
    
    memoryStruct.instructionCode = 0; 
    memoryStruct.operationName = 0; 
    memoryStruct.PCInitial = 0; 
    memoryStruct.PCFinal = 0; 
    memoryStruct.opcode = 0;
    memoryStruct.Rd = 0;
    memoryStruct.Rn = 0;
    memoryStruct.Rm = 0;
    memoryStruct.Rt = 0;
    memoryStruct.RdVal = 0;
    memoryStruct.RnVal = 0;
    memoryStruct.RmVal = 0;
    memoryStruct.RtVal = 0;
    memoryStruct.storeValLow = 0;
    memoryStruct.storeValHigh = 0;
    memoryStruct.storeAddress = 0;
    memoryStruct.loadAddress = 0;
    memoryStruct.RtVal = 0;
}

void check_stall() {
    if (StallFlag) {
        StallFlag = 0;
        clearMemoryStruct();
    }
    else if ((memoryStruct.Rt == decodeStruct.Rn) || (memoryStruct.Rt == decodeStruct.Rm) || (memoryStruct.Rt == decodeStruct.Rt)) {
        StallFlag = 1;
    }
}

void pipe_stage_mem()
{
    memoryStruct.instructionCode = executeStruct.instructionCode;
    memoryStruct.operationName = executeStruct.operationName;
    memoryStruct.PCInitial = executeStruct.PCInitial;
    memoryStruct.PCFinal = executeStruct.PCFinal;
    memoryStruct.opcode = executeStruct.opcode;
    memoryStruct.Rd = executeStruct.Rd;
    memoryStruct.Rn = executeStruct.Rn;
    memoryStruct.Rm = executeStruct.Rm;
    memoryStruct.Rt = executeStruct.Rt;
    memoryStruct.cmpResult = executeStruct.cmpResult; 

    memoryStruct.RdVal = executeStruct.RdVal;
    memoryStruct.RnVal = executeStruct.RnVal;
    memoryStruct.RmVal = executeStruct.RmVal;
    memoryStruct.RtVal = executeStruct.RtVal;
    memoryStruct.storeValLow = executeStruct.storeValLow;
    memoryStruct.storeValHigh = executeStruct.storeValHigh;
    memoryStruct.storeAddress = executeStruct.storeAddress;
    memoryStruct.loadAddress = executeStruct.loadAddress;

    int storeValLow = executeStruct.storeValLow;
    int storeValHigh = executeStruct.storeValHigh;
    uint32_t loadValLow = executeStruct.loadValLow;
    uint64_t storeAddress = executeStruct.storeAddress;
    uint64_t loadAddress = executeStruct.loadAddress;

    switch (memoryStruct.operationName) {
 
        case STUR:
            mem_write_32(storeAddress, storeValLow);
            mem_write_32(storeAddress + 4, storeValHigh); 
            break;

        case STURB:
            mem_write_32(storeAddress, storeValLow);
            break;

        case STURH:
            mem_write_32(storeAddress, storeValLow); 
            break;

        case LDUR:
            memoryStruct.RtVal = (mem_read_32(loadValLow) | (((uint64_t) mem_read_32(loadValLow + 0x4)) << 32));
            check_stall(); 
            break;

        case LDURB:
            memoryStruct.RtVal = (uint64_t) mem_read_32(loadValLow) & 0xFF;
            check_stall();
            break;

        case LDURH:
            memoryStruct.RtVal = (uint64_t) mem_read_32(loadValLow) & 0xFFFF;
            check_stall();
            break;
    }
    
    check_dependency_mem();
}

void clearDecodeStruct() {
        decodeStruct.instructionCode = 0;
        decodeStruct.operationName = 0;
        decodeStruct.Rd           = 50; 
        decodeStruct.Rn           = 40; 
        decodeStruct.Rm           = 40; 
        decodeStruct.Rt           = 40;
        decodeStruct.opcode       = 0;
        decodeStruct.imm          = 0;
        decodeStruct.imm19        = 0;
        decodeStruct.signImm19   = 0;
        decodeStruct.signedImm19 = 0;
        decodeStruct.imm9         = 0;
        decodeStruct.signImm9    = 0;
        decodeStruct.signedImm9  = 0;
}

void clearFetchStruct() {
    
    fetchStruct.instructionCode = 0;
    fetchStruct.PC = 0;
}

void pipe_stage_execute() {
    if (!StallFlag) {
        printf("%s\n", "EXECUTE");
        
        uint64_t instructionCode = decodeStruct.instructionCode;
        uint64_t PC              = decodeStruct.PC;
        uint64_t Rd              = decodeStruct.Rd;
        uint64_t Rn              = decodeStruct.Rn;
        uint64_t Rm              = decodeStruct.Rm;
        uint64_t opcode          = decodeStruct.opcode;
        uint64_t Rt              = decodeStruct.Rt;
        uint64_t imm             = decodeStruct.imm;
        uint64_t imm19           = decodeStruct.imm19;
        int32_t signImm19        = decodeStruct.signImm19;
        int32_t signedImm19      = decodeStruct.signedImm19;
        uint64_t imm9            = decodeStruct.imm19;
        int32_t signImm9         = decodeStruct.signImm9;
        uint32_t signedImm9      = decodeStruct.signedImm9;
        executeStruct.instructionCode = instructionCode;
        executeStruct.operationName = decodeStruct.operationName;
        executeStruct.opcode          = opcode;
        executeStruct.PCInitial       = decodeStruct.PC;
        executeStruct.Rd = decodeStruct.Rd;
        executeStruct.Rn = decodeStruct.Rn;
        executeStruct.Rm = decodeStruct.Rm;
        executeStruct.Rt = decodeStruct.Rt;
        
        uint64_t correct_addr;

        switch (executeStruct.operationName)
        {
            case ADD: ; // ADD
                
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded + decodeStruct.RmValForwarded; 
                }
                
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded + CURRENT_STATE.REGS[Rm];
                }

                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] + decodeStruct.RmValForwarded;
                }

                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] + CURRENT_STATE.REGS[Rm];
                }

                break;

            case ADDI: ; // ADD immediate
                if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded + imm;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] + imm;
                }
                break;

            case ADDS: ; // ADDS
                printf("IN ADDS\n");

                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded + decodeStruct.RmValForwarded; 
                    printf("ADD1\n");
                }
                
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded + CURRENT_STATE.REGS[Rm];
                    printf("ADD2\n");
                }

                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] + decodeStruct.RmValForwarded;
                    printf("ADD3\n");
                }

                else {
                    printf("%s\n", "NO FORWARDING");
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] + CURRENT_STATE.REGS[Rm];
                }
                break;

            case ADDSI: ; // ADDS immediate
                if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded + imm;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] + imm;
                }
                break;

            case STUR: ;// STUR 64 bits
                if (RNFLAG) {
                    executeStruct.storeAddress = decodeStruct.RnValForwarded + signedImm9;
                }
                else {
                    executeStruct.storeAddress = CURRENT_STATE.REGS[Rn] + signedImm9;
                }
                if (RTFLAG) {
                    executeStruct.storeValLow = decodeStruct.RtValForwarded;
                    executeStruct.storeValHigh = (decodeStruct.RtValForwarded >> 32) & 0xFFFFFFFF;
                }
                else {
                    executeStruct.storeValLow = CURRENT_STATE.REGS[Rt];
                    executeStruct.storeValHigh = (CURRENT_STATE.REGS[Rt] >> 32) & 0xFFFFFFFF;
                }
                break;

            case STURB: ;// STURB
                if (RNFLAG) {
                    executeStruct.storeAddress = decodeStruct.RnValForwarded + signedImm9;
                }
                else {
                    executeStruct.storeAddress = CURRENT_STATE.REGS[Rn] + signedImm9;
                }
                
                if (RTFLAG) {
                    executeStruct.storeValLow = decodeStruct.RtValForwarded & 0xFF;
                }
                else {
                    executeStruct.storeValLow = CURRENT_STATE.REGS[Rt] & 0xFF;
                }
                break;

            case STURH: ;// STURH
                if (RNFLAG) {
                    executeStruct.storeAddress = decodeStruct.RnValForwarded + signedImm9;
                }
                else {
                    executeStruct.storeAddress = CURRENT_STATE.REGS[Rn] + signedImm9;
                }
                
                if (RTFLAG) {
                    executeStruct.storeValLow = decodeStruct.RtValForwarded & 0xFF;
                }
                else {
                    executeStruct.storeValLow = CURRENT_STATE.REGS[Rt] & 0xFF;
                }
                break;

            case AND: ;// AND
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded & decodeStruct.RmValForwarded; 
                }
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded & CURRENT_STATE.REGS[Rm];
                }
                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] & decodeStruct.RmValForwarded;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] & CURRENT_STATE.REGS[Rm];
                }
                break;

            case ANDS: ;// ANDS
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded & decodeStruct.RmValForwarded; 
                }
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded & CURRENT_STATE.REGS[Rm];
                }
                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] & decodeStruct.RmValForwarded;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] & CURRENT_STATE.REGS[Rm];
                }

                break;

            case EOR: ;// EOR
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded ^ decodeStruct.RmValForwarded; 
                }
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded ^ CURRENT_STATE.REGS[Rm];
                }
                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] ^ decodeStruct.RmValForwarded;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] ^ CURRENT_STATE.REGS[Rm];
                }
                break;

            case ORR: ;// ORR
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded | decodeStruct.RmValForwarded; 
                }
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded | CURRENT_STATE.REGS[Rm];
                }
                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] | decodeStruct.RmValForwarded;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] | CURRENT_STATE.REGS[Rm];
                }

                break;

            case LDUR: ;// LDUR 64 bit
                if (RNFLAG) {
                    executeStruct.loadValLow = decodeStruct.RnValForwarded + signedImm9;
                }

                else {
                    executeStruct.loadValLow = CURRENT_STATE.REGS[Rn] + signedImm9; 
                }
                break;

            case LDURB: ;// LDURB
                if (RNFLAG) {
                    executeStruct.loadValLow = decodeStruct.RnValForwarded + signedImm9;
                }

                else {
                    executeStruct.loadValLow = CURRENT_STATE.REGS[Rn] + signedImm9; 
                }
                break;

            case LDURH: ;// LDURH
                if (RNFLAG) {
                    executeStruct.loadValLow = decodeStruct.RnValForwarded + signedImm9;
                }

                else {
                    executeStruct.loadValLow = CURRENT_STATE.REGS[Rn] + signedImm9; 
                }
                break;

            case LSLI: ;// LSL Immediate and LSR Immediate
                int shift = (executeStruct.instructionCode >> 10) & 0x3F;
                if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded << (63 - shift);
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] << (63 - shift);
                }
                break;

            case LSRI: ;// LSL Immediate and LSR Immediate
                shift = (executeStruct.instructionCode >> 16) & 0x3F;
                if (RNFLAG) {
                    executeStruct.RdVal = (uint64_t)decodeStruct.RnValForwarded >> shift;
                }
                else {
                    executeStruct.RdVal = (uint64_t)CURRENT_STATE.REGS[Rn] >> shift;
                }
                break;

            case BR: ;// BR
                if (RNFLAG) {
                    if (decodeStruct.RnValForwarded == CURRENT_STATE.PC - 4) {
                        correct_addr = decodeStruct.RnValForwarded;

                    }
                    else {
                        correct_addr = decodeStruct.RnValForwarded;
                    }
                }
                else {
                    if (CURRENT_STATE.REGS[Rn] == CURRENT_STATE.PC - 4) {
                        correct_addr = CURRENT_STATE.REGS[Rn]; 
                    }
                    else {
                        correct_addr = CURRENT_STATE.REGS[Rn];
                    }
                }
                
                bp_update(branch_predictor, EXtoMEM_BP, decodeStruct.PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 0);

                if (EXtoMEM_BP->flush) {
                    printf("Flushing\n");
                    clearDecodeStruct();
                    clearFetchStruct();
                    printf("Correct address: %ld\n", correct_addr);
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;

            case SUB: // SUB
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded - decodeStruct.RmValForwarded; 
                }
                
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded - CURRENT_STATE.REGS[Rm];
                }

                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] - decodeStruct.RmValForwarded;
                }

                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] - CURRENT_STATE.REGS[Rm];
                }
                break;

            case SUBI: ; //SUB Immediate
                int imm12 = (executeStruct.instructionCode >> 10) & 0xFFF;
                if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] - imm12;
                }
                break;

            case SUBS: ;// SUBS -- AKA CMP if Rt = 0x11111
                int sumSubs_subs = 0;
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded - decodeStruct.RmValForwarded; 
                }
                
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded - CURRENT_STATE.REGS[Rm];
                }

                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] - decodeStruct.RmValForwarded;
                }

                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] - CURRENT_STATE.REGS[Rm];
                }
                executeStruct.cmpResult = sumSubs_subs; 
                set_temp_flags(sumSubs_subs);
                break;

            case CMP: ;// SUBS -- AKA CMP if Rt = 0x11111
                printf("IN CMP\n");
                int sumSubs = 0;
                if (RNFLAG & RMFLAG) {
                    sumSubs = decodeStruct.RnValForwarded - decodeStruct.RmValForwarded; 
                    printf("CMP Case 1\n");
                }
                
                else if (RNFLAG) {
                    sumSubs = decodeStruct.RnValForwarded - CURRENT_STATE.REGS[Rm];
                    printf("CMP Case 2\n");
                }

                else if (RMFLAG) {
                    sumSubs = CURRENT_STATE.REGS[Rn] - decodeStruct.RmValForwarded;
                    printf("CMP Case 3\n");
                }

                else {
                    sumSubs = CURRENT_STATE.REGS[Rn] - CURRENT_STATE.REGS[Rm];
                    printf("CMP Case 4\n");
                }
                printf("sumSubs = %d\n", sumSubs);
                executeStruct.cmpResult = sumSubs; 
                set_temp_flags(sumSubs);

                break;

            case SUBSI: ; //SUBSI Immediate --  AKA CMPI if Rt = 0b11111
                imm12 = (executeStruct.instructionCode >> 10) & 0xFFF;
                if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded;
                }
                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] - imm12;
                }
                break;

            case CMPI: ; //SUBSI Immediate --  AKA CMPI if Rt = 0b11111
                imm12 = (executeStruct.instructionCode >> 10) & 0xFFF;
                sumSubs = 0;
                if (RNFLAG) {
                    sumSubs = decodeStruct.RnValForwarded;
                }
                else {
                    sumSubs = CURRENT_STATE.REGS[Rn] - imm12;
                }
                executeStruct.cmpResult = sumSubs;
                set_temp_flags(sumSubs);

                break;


            case HLT: //HLT
                printf("(EX) HLT\n");
                HaltFlag = 1;
                break;

            case MUL: //MUL
                if (RNFLAG & RMFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded * decodeStruct.RmValForwarded; 
                }
                
                else if (RNFLAG) {
                    executeStruct.RdVal = decodeStruct.RnValForwarded * CURRENT_STATE.REGS[Rm];
                }

                else if (RMFLAG) {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] * decodeStruct.RmValForwarded;
                }

                else {
                    executeStruct.RdVal = CURRENT_STATE.REGS[Rn] * CURRENT_STATE.REGS[Rm];
                }
                break;

            case CBNZ: ; // CBNZ
                if (RTFLAG) {
                    if (decodeStruct.RtValForwarded != 0) {
                        if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                            correct_addr = PC + (signedImm19 * 4); 
                        }
                        else {
                            correct_addr = PC + (signedImm19 * 4);
                        }
                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1);
                    } else {
                        correct_addr = PC + 8; //I am assuming that the correct address is 8 away from the instruction that led to this one 
                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1);

                    }
                }
                else {
                    if (CURRENT_STATE.REGS[Rt] != 0) {
                        if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                            correct_addr = PC + (signedImm19 * 4);
                        }
                        else {
                            correct_addr = PC + (signedImm19 * 4);
                        }
                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1);
                    } else {
                        correct_addr = PC + 8;
                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1);
                    }
                }
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;

            case CBZ: ;// CBZ
                if (RTFLAG) {
                    if (decodeStruct.RtValForwarded == 0) {
                        if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                            correct_addr = PC + (signedImm19 * 4);
                        }
                        else {
                            correct_addr = PC + (signedImm19 * 4);
                        }

                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1);

                    } else {
                        correct_addr = PC + 8;
                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1);

                    }
                }
                else {
                    if (CURRENT_STATE.REGS[Rt] == 0) {
                        if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                            correct_addr = PC + (signedImm19 * 4);
                        }
                        else {
                            correct_addr = PC + (signedImm19 * 4);
                        }
                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1);

                    } else {
                        correct_addr = PC + 8;
                        bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1);

                    }
                }
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;

            case MOVZ: ;// MOVZ
                int imm16 = (executeStruct.instructionCode >> 5) & 0xFFFF;
                executeStruct.RdVal = imm16;
                break;

            case B: ;// B
                int branch        = executeStruct.instructionCode & 0x3FFFFFF;
                int sign_branch   = ((branch >> 25) << 25) * -1;
                int signed_branch = (branch & 0x1FFFFFF) + sign_branch;
                if ((PC + (4 * signed_branch)) == CURRENT_STATE.PC - 4) { 
                    correct_addr = PC + (4 * signed_branch);
                }
                else {
                    correct_addr = PC + (4 * signed_branch);
                }
                bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 0); 
                
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                    printf("B at EX branching to: %ld\n", correct_addr);
                }
                break;

            case BEQ: ;// BEQ
                if (temp_FLAG_Z == 1) {
                    if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    else {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1); 
                } else {
                    correct_addr = PC + 8;
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1); 
                }
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;

            case BNE: ;//BNE
                if (temp_FLAG_Z == 0) {
                    if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    else {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1); 
                } else {
                    correct_addr = PC + 8;
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1); 
                }
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;

            case BGE: ;//BGE
                if (temp_FLAG_N == 0) {
                    if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    else {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1); 
                } else {
                    correct_addr = PC + 8;
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1); 
                }
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;



            case BLT: ;//BLT
                if (temp_FLAG_N == 1) {
                    if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    else {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1); 
                } else {
                    correct_addr = PC + 8;
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1); 
                }
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;


            case BGT: ;//BGT
                printf("(Execute) BGT\n");
                if ((temp_FLAG_Z == 0) && (temp_FLAG_N == 0)) {
                    if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                        printf("(Execute) BGT Branch 1\n");
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    else {
                        printf("(Execute) BGT Branch 2\n");
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1); 
                } else {
                    printf("(Execute) BGT Branch 3\n");
                    correct_addr = PC + 4;
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1); 
                }
                if (EXtoMEM_BP->flush) {
                    printf("(execute) BGT Flushed\n");
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;

            case BLE: ;//BLE
                if ((temp_FLAG_Z == 1) || (temp_FLAG_N == 1)) {
                    if ((PC + (signedImm19 * 4)) == CURRENT_STATE.PC - 4) {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    else {
                        correct_addr = PC + (signedImm19 * 4);
                    }
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 1, IDtoEX_BP->pred_taken, 1); 
                } else {
                    correct_addr = PC + 8;
                    bp_update(branch_predictor, EXtoMEM_BP, PC, correct_addr, IDtoEX_BP->pred_address, 0, IDtoEX_BP->pred_taken, 1); 
                }
                if (EXtoMEM_BP->flush) {
                    clearDecodeStruct();
                    clearFetchStruct();
                    CURRENT_STATE.PC = correct_addr;
                    flushed = 1;
                }
                break;
            
        RNFLAG = 0;
        RMFLAG = 0;
        RTFLAG = 0;
        }
    }
}

void pipe_stage_decode()
{   
    //if ((!StallFlag) & (BFlag == 0) & (CBFlag == 0)) {
    if (!StallFlag) {
        printf("%s\n", "DECODE");
        uint64_t instructionCode = fetchStruct.instructionCode;
        decodeStruct.instructionCode = instructionCode;
        decodeStruct.PC           = fetchStruct.PC;
        decodeStruct.Rd           = (instructionCode >> 0)  & 0x1F;
        decodeStruct.Rn           = (instructionCode >> 5)  & 0x1F;
        decodeStruct.Rm           = (instructionCode >> 16) & 0x1F;
        decodeStruct.Rt           = (instructionCode >> 0)  & 0x1F;
        decodeStruct.opcode       = (instructionCode >> 21) & 0x7FF;
        printf("Opcode: %ld\n", decodeStruct.opcode);
        decodeStruct.imm          = (instructionCode >> 10) & 0xFFF;
        decodeStruct.imm19        = (instructionCode >> 5)  & 0x7FFFF;
        decodeStruct.signImm19   = ((decodeStruct.imm19 >> 18) << 18) * -1;
        decodeStruct.signedImm19 = (decodeStruct.imm19 & 0x3FFFF) + decodeStruct.signImm19;
        decodeStruct.imm9         = (instructionCode >> 12) & 0x1FF;
        decodeStruct.signImm9    = ((decodeStruct.imm9 >> 8) << 8) * -1;
        decodeStruct.signedImm9  = (decodeStruct.imm9 & 0xFF) + decodeStruct.signImm9;
        
        IDtoEX_BP->pred_address = IFtoID_BP->pred_address;
        IDtoEX_BP->pred_taken = IFtoID_BP->pred_taken;

        switch (decodeStruct.opcode)
        {
            case 0x458 ... 0x459: ; // ADD
                decodeStruct.operationName = ADD;
                decodeStruct.Rt = 40;
                break;

            case 0x488 ... 0x48F: ; // ADD immediate
                decodeStruct.operationName = ADDI;
                decodeStruct.Rm = 40;
                decodeStruct.Rt = 40;
                break;

            case 0x558 ... 0x559: ; // ADDS
                decodeStruct.operationName = ADDS;
                decodeStruct.Rt = 40;
                break;

            case 0x588 ... 0x58F: ; // ADDS immediate
                decodeStruct.operationName = ADDSI;
                decodeStruct.Rm = 40;
                decodeStruct.Rt = 40;
                break;

            case 0x7C0: ;// STUR 64 bits
                decodeStruct.operationName = STUR;
                decodeStruct.Rd = 50;
                decodeStruct.Rm = 40;
                break;

            case 0x1C0: ;// STURB
                decodeStruct.operationName = STURB;
                decodeStruct.Rd = 50;
                decodeStruct.Rm = 40;
                break;

            case 0x3C0: ;// STURH
                decodeStruct.operationName = STURH;
                decodeStruct.Rd = 50;
                decodeStruct.Rm = 50;
                break;

            case 0x450: ;// AND
                decodeStruct.operationName = AND;
                decodeStruct.Rt = 40;
                break;

            case 0x750: ;// ANDS
                decodeStruct.operationName = ANDS;
                decodeStruct.Rt = 40;
                break;

            case 0x650: ;// EOR
                decodeStruct.operationName = EOR;
                decodeStruct.Rt = 40;
                break;

            case 0x550: ;// ORR
                decodeStruct.operationName = ORR;
                decodeStruct.Rt = 40;
                break;

            case 0x7C2: ;// LDUR 64 bit
                decodeStruct.operationName = LDUR; 
                decodeStruct.Rm = 40;
                decodeStruct.Rd = 50;
                break;

            case 0x1C2: ;// LDURB
                decodeStruct.operationName = LDURB;
                decodeStruct.Rm = 40;
                decodeStruct.Rd = 50;
                break;

            case 0x3C2: ;// LDURH
                decodeStruct.operationName = LDURH;
                decodeStruct.Rm = 40;
                decodeStruct.Rd = 50;
                break;

            case 0x69A ... 0x69B: ;// LSL Immediate and LSR Immediate
                int shift = (decodeStruct.instructionCode >> 10) & 0x3F;
                if (shift != 0x3F) {
                    decodeStruct.operationName = LSLI;
                    decodeStruct.Rm = 40;
                    decodeStruct.Rt = 40;
                    break;
                }
                else {
                    decodeStruct.operationName = LSRI;
                    decodeStruct.Rm = 40;
                    decodeStruct.Rt = 40;
                    break;
                }

            case 0x6B0: ;// BR
                decodeStruct.operationName = BR;
                decodeStruct.Rd = 50;
                decodeStruct.Rm = 40;
                decodeStruct.Rt = 40;
                printf("IN BR DECODE");
                BFlag = 2;
                break;

            case 0x658: // SUB
                decodeStruct.operationName = SUB;
                decodeStruct.Rt = 40;
                break;

            case 0x688 ... 0x68F: ; //SUB Immediate
                decodeStruct.operationName = SUBI;
                decodeStruct.Rm = 40;
                decodeStruct.Rt = 40;
                break;

            case 0x758: ;// SUBS -- AKA CMP if Rt = 0x11111
                if (decodeStruct.Rt != 0b11111) {
                    decodeStruct.operationName = SUBS;
                    decodeStruct.Rt = 40;
                }
                else {
                    decodeStruct.operationName = CMP;
                }
                break;

            case 0x788 ... 0x78F: ; //SUBSI Immediate --  AKA CMPI if Rt = 0b11111
                if (decodeStruct.Rt != 0b11111) {
                    decodeStruct.operationName = SUBSI;
                    decodeStruct.Rm = 40;
                    decodeStruct.Rt = 40;
                }
                else {
                    decodeStruct.operationName = CMPI;
                }
                break;

            case 0x6A2: //HLT
                printf("(ID) HLT\n");
                decodeStruct.operationName = HLT;
                break;

            case 0x4D8: //MUL
                decodeStruct.operationName = MUL;
                decodeStruct.Rt = 40;
                break;

            case 0x5A8 ... 0x5AF: ; // CBNZ
                decodeStruct.operationName = CBNZ;
                decodeStruct.Rd = 50;
                decodeStruct.Rn = 40;
                decodeStruct.Rm = 40;
                CBFlag = 2;
                break;

            case 0x5A0 ... 0x5A7: ;// CBZ
                decodeStruct.operationName = CBZ;
                decodeStruct.Rd = 50;
                decodeStruct.Rn = 40;
                decodeStruct.Rm = 40;
                CBFlag = 2; 
                break;

            case 0x694 ... 0x697: ;// MOVZ
                decodeStruct.operationName = MOVZ;
                decodeStruct.Rn = 40;
                decodeStruct.Rt = 40;
                decodeStruct.Rm = 40;
                break;

            case 0x0A0 ... 0x0BF: ;// B
                decodeStruct.operationName = B;
                decodeStruct.Rd = 50;
                decodeStruct.Rn = 40;
                decodeStruct.Rm = 40;
                decodeStruct.Rt = 40;
                BFlag = 2; 
                break;

            case 0x2A0 ... 0x2A7: ; //B.cond
                int cond = decodeStruct.instructionCode & 0xF;
                decodeStruct.Rd = 50;
                decodeStruct.Rn = 40;
                decodeStruct.Rm = 40;
                decodeStruct.Rt = 40;
                CBFlag = 2;
                switch (cond)
                {
                    case 0x0: ;// BEQ
                        decodeStruct.operationName = BEQ; 
                        break;

                    case 0x1: ;//BNE
                        decodeStruct.operationName = BNE;
                        break;

                    case 0xA: ;//BGE
                        decodeStruct.operationName = BGE;
                        break;

                    case 0xB: ;//BLT
                        decodeStruct.operationName = BLT;
                        break;

                    case 0xC: ;//BGT
                        decodeStruct.operationName = BGT;
                        break;

                    case 0xD: ;//BLE
                        decodeStruct.operationName = BLE;
                        break;
                }
                break;
        }
    }
}

void pipe_stage_fetch()
{   
    if (StallFlag) { 
        ;
    }
    else if (HaltFlag) {
        ;
    }
    else {
        if (flushed) {
            flushed = 0;
            printf("YOU FUCKING FLUSHED IT!");
        }
        else {
            printf("(IF) PC BEFORE PREDICT (CURRENT PC) = %ld\n", CURRENT_STATE.PC);
            fetchStruct.PC = CURRENT_STATE.PC;
            fetchStruct.instructionCode = mem_read_32(CURRENT_STATE.PC);
            printf("FETCH INSTRUCTION CODE: %ld\n", fetchStruct.instructionCode);
            bp_predict(branch_predictor, IFtoID_BP, fetchStruct.instructionCode);
            CURRENT_STATE.PC = IFtoID_BP->pred_address;
            printf("(IF) PC AFTER PREDICT (NEXT PC) = %ld\n", CURRENT_STATE.PC);
        }
    }
}

void pipe_cycle()
{
    printf("###########################\n");
    printf("Cycle %d\n", stat_cycles + 1);
    pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
    printf("FLUSHED: %i\n", flushed);
	pipe_stage_fetch();
    cycleCounter ++;
    printf("###########################\n\n");
}


