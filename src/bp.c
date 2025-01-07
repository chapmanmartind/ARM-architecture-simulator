/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200
 * 
 * Collaborators:
 * Martin Chapman (chapmanmartind)
 * Mukerrem Tufekcioglu (mukerrem)
 */

#include "bp.h"
#include "pipe.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * bp_predict takes in the predictor structure of the pipeline and 
 * the program address of a branch instruction, and returns an array
 * which has the branch address prediction as first element, and
 * 1 as the second element if the branch is taken (0 otherwise).
 */
void bp_predict(bp_t *predictor, IFtoID_BP_t *IFtoID_BP, uint64_t pr_address)
{
    uint8_t   address_bits = (uint8_t) ((pr_address >> 2) & 0xFF);
    uint8_t   PHT_index    = address_bits ^ predictor->GHR;
    printf("(Prediction) Full address for BTB: %ld\n", pr_address);
    printf("(Prediction) Address bits for BTB: %d\n", address_bits);

    /* Case when BTB has valid bit == 0 for given address */
    if (predictor->BTB[address_bits][2] == 0) {
        printf("(Prediction) BTB has invalid bit\n"); 
        IFtoID_BP->pred_address = CURRENT_STATE.PC + 4;
        IFtoID_BP->pred_taken   = 0;
        return;
    }

    /* Case when BTB doesn't have the right full address for index */
    if (predictor->BTB[address_bits][0] != pr_address) {
        printf("(Prediction) Address at BTB doesn't match program address");
        IFtoID_BP->pred_address = CURRENT_STATE.PC + 4;
        IFtoID_BP->pred_taken   = 0;
        return;
    }
    
    /* Case when branch is taken because it is an unconditional branch */
    if (predictor->BTB[address_bits][3] == 0) {
        printf("(Prediction) Branching unconditionally\n");
        IFtoID_BP->pred_address = predictor->BTB[address_bits][1];
        IFtoID_BP->pred_taken   = 1;
        return;
    }

    /* Case when branch is taken because it Gshare says so*/
    if (predictor->PHT[PHT_index] > 1) {
        printf("(Prediction) Branching due to Gshare results\n");
        IFtoID_BP->pred_address = predictor->BTB[address_bits][1];
        IFtoID_BP->pred_taken   = 1;
        return;
    }

    /* Case when branch is not taken */
    printf("(Prediction) Not branching due to Gshare results\n");
    IFtoID_BP->pred_address = CURRENT_STATE.PC + 4;
    IFtoID_BP->pred_taken   = 0;
    return;
}

/*
 * bp_update takes in the pipeline's predictor structure, the program address of the branch instruction,
 * the calculated branch address, the predicted address from bp_predict, the knowledge of whether or not if this
 * branch instruction was taken, whether or not the branch was predicted to be taken, and if it was a conditional branch, 
 * and returns 1 if the pipeline must be flushed.
 */
void bp_update(bp_t *predictor, EXtoMEM_BP_t *EXtoMEM_BP, uint64_t pr_address, uint64_t br_address, uint64_t pred_address, int taken, uint64_t pred_taken, int cond)
{
    /* Update BTB */
    uint8_t address_bits = (uint8_t) ((pr_address >> 2) & 0xFF);
    printf("(Prediction update) Full address for BTB: %ld\n", pr_address);
    printf("(Prediction update) Address bits for BTB: %d\n", address_bits);
    predictor->BTB[address_bits][0] = pr_address;
    predictor->BTB[address_bits][1] = br_address;
    predictor->BTB[address_bits][2] = 1;
    predictor->BTB[address_bits][3] = cond;
    
    /* Don't update anything else if unconditional */
    if (!cond) {
        printf("(Prediction Update) Not updating Gshare because unconditional branch\n");
        EXtoMEM_BP->flush = (int) (pred_address != br_address || taken != pred_taken);
        return;
    }

    /* Update gshare directional predictor */
    printf("(Prediction Update) Updating Gshare and GHR\n");
    uint8_t PHT_index = address_bits ^ predictor->GHR;
    uint8_t PHT_pred  = predictor->PHT[PHT_index];
    printf("(Prediction Update) Updating PHT_index %d (taken = %d)\n", PHT_index, taken);
    if (taken) {
        if (PHT_pred < 3) predictor->PHT[PHT_index] += 1;
    } else {
        if (PHT_pred > 0) predictor->PHT[PHT_index] -= 1;
    }
    
    /* Update global history register */
    predictor->GHR = (predictor->GHR << 1) + taken;

    EXtoMEM_BP->flush = (int) (pred_address != br_address || taken != pred_taken);
    return;
}



