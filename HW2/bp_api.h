/* 046267 Computer Architecture - Spring 2016 - HW #2 */
/* API for the predictor simulator */

#ifndef BP_API_H_
#define BP_API_H_



#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************/
/* The following functions should be implemented in your bp.c (or .cpp) */
/*************************************************************************/

/*
 * BP_init - initialize the predictor
 * all input parameters are set (by the main) as declared in the trace file
 * return 0 on success, otherwise (init failure) return <0
 */
int BP_init(unsigned btbSize, unsigned historySize,
bool isGlobalHist, bool isGlobalTable, bool isShare);

/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
bool BP_predict(uint32_t pc, uint32_t *dst);

/*
 * BP_setBranchAt - informs the predictor of a branch instruction
 * param[in] pc - the branch instruction address
 */
void BP_setBranchAt(uint32_t pc);

/*
 * BP_update - updates the predictor with actual decision (taken / not taken)
 * param[in] pc - the branch instruction address
 * param[in] targetPc - the target address
 * param[in] taken - the actual decision, true if taken and false if not taken
 */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken);

#ifdef __cplusplus
}
#endif

#endif /* BP_API_H_ */
