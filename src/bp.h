/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200
 * 
 * Collaborators:
 * Martin Chapman (chapmanmartind)
 * Mukerrem Tufekcioglu (mukerrem)
 */

#ifndef _BP_H_
#define _BP_H_

#include <stdint.h>
#include "pipe.h"

typedef struct
{
    /* gshare */
    uint8_t GHR;
    uint8_t PHT[256];
    /* BTB */
    uint64_t BTB[1024][4]; /* Pr address, Br address, Valid, Conditional */
} bp_t;

void bp_predict(bp_t *predictor, IFtoID_BP_t *IFtoID_BP, uint64_t pr_address);

void bp_update(bp_t *predictor, EXtoMEM_BP_t *EXtoMEM_BP, uint64_t pr_address, uint64_t br_address, uint64_t pred_address, int taken, uint64_t pred_taken, int cond); 

#endif
