/* 046267 Computer Architecture - Spring 2016 - HW #3 */
/* API for the dataflow statistics calculator */

#ifndef _DFLOW_CALC_H_
#define _DFLOW_CALC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

/// Program context
/// This is a reference to the (internal) data maintained for a given program
typedef void *ProgCtx;
#define PROG_CTX_NULL NULL

/// Instruction info required for dataflow calculations
/// This structure provides the register file index of each source operand and the destination operand
typedef struct {
    unsigned int dstIdx;
    unsigned int src1Idx;
    unsigned int src2Idx;
} InstInfo;

/** analyzeProg: Analyze given program and save results
    \param[in] prog An array of instructions information required for dataflow analysis
    \param[in] numOfInsts The number of instructions in prog[]
    \returns Analysis context that may be queried using the following query functions or PROG_CTX_NULL on failure */
ProgCtx analyzeProg(InstInfo prog[], unsigned int numOfInsts);

/** freeProgCtx: Free the resources associated with given program context
    \param[in] ctx The program context to free
*/
void freeProgCtx(ProgCtx ctx);

/** getDepDepth: Get the dataflow dependency depth
    The first (independent) instruction that are direct decendents to the entry node should return 0
    \param[in] ctx The program context as returned from analyzeProg()
    \param[in] theInst The index of the instruction of the program to query (in given prog[])
    \returns >= 0 The dependency depth, <0 for invalid instruction index for this program context
*/
int getDepDepth(ProgCtx ctx, unsigned int theInst);

/** getInstDeps: Get the instructions that a given instruction depends upon
    \param[in] ctx The program context as returned from analyzeProg()
    \param[in] theInst The index of the instruction of the program to query (in given prog[])
    \param[out] src1DepInst Returned index of the instruction that src1 depends upon (-1 if depends on "entry")
    \param[out] src2DepInst Returned index of the instruction that src2 depends upon (-1 if depends on "entry")
    \returns 0 for success, <0 for error (e.g., invalid instruction index)
*/
int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst);

/** getInstUsers: Get the list of instructions that use the result of this instruction.
    \param[in] ctx The program context as returned from analyzeProg()
    \param[in] theInst The index of the instruction of the program to query (in given prog[])
    \param[in] numUsersIn The size of the users[] array given in the next parameter
    \param[out] users An array of numUsersIn elements to fill in the indices of the instruction that use given instruction
    \returns >=0 The number of intructions that use given instruction.
                 This value may be greater than numUsersIn.
                 If numUsersIn < actual number of users, the given array users[] would hold only the first numUsersIn (in instruction order) users 
             <0 Error (e.g., invalid instruction index)
 */
int getInstUsers(ProgCtx ctx, unsigned int theInst, unsigned int numUsersIn, unsigned int *users);


#ifdef __cplusplus
}
#endif

#endif /*_DFLOW_CALC_H_*/
