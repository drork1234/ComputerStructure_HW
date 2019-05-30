/* 046267 Computer Architecture - Spring 2016 - HW #3 */
/* Main program for testing invocations to dflow_calc */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif //_WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dflow_calc.h"

/// Quota of program buffer size increase
#define PROG_SIZE_QUOTA 64

/// Max. number of program users dumped
#define MAX_USERS_DUMP 16

/// Resize program buffer
/// \param[in] maxSize New program buffer size limit
/// \param[in] oldBuf The old buffer used for the program. May be NULL for new allocation.
/// \returns The allocated program buffer of given maxSize entries
InstInfo *allocProgBuffer(size_t maxSize, InstInfo *oldBuf)
{
    InstInfo *newBuf = (InstInfo*)realloc(oldBuf, maxSize * sizeof(InstInfo));
    if (newBuf == NULL) {
        printf("Error: Failed allocating program buffer for %zu instructions!\n", maxSize);
        exit(1);
    }
	
    return newBuf;
}

/// readTrace: Read a program file formatted with {dst src1 src2} triplets
/// \param[in] filename The trace file name
/// \param[out] prog The program trace (array) that was read from the given file. Allocated in this function. Should be freed by the caller.
/// \returns >0 The number of elements (instructions) in the returned prog[] , <0 error reading the trace file or creating the array
int readProgram(const char *filename, InstInfo **prog) {
    int i;
    unsigned lineNum = 0;
	char curLine[81];
    char *curField, *endOfVal;
    long int fieldVal[3];
    char *tokenizerEntry; // Entry "tag" for strtok()
    InstInfo *progBuf = NULL; // Current program buffer (may be resized as needed)
    size_t maxInsts = 0; // Current limit on the program buffer

    *prog = NULL; // Initialize in case of exit with error
    
    FILE *progFile = fopen(filename, "r");
	if (progFile == NULL) {
		printf("Failed openning the program file: %s\n", filename);
		return -1;
	}
    
    // Read program file lines
	while (fgets(curLine, sizeof(curLine), progFile) != NULL) {
        //printf("*** curLine=%s\n", curLine);
        if (lineNum == maxInsts) { // Need to resize buffer
            maxInsts += PROG_SIZE_QUOTA;
            progBuf = allocProgBuffer(maxInsts, progBuf);
        }
        tokenizerEntry = curLine;
        while (isspace(*tokenizerEntry)) ++tokenizerEntry; // Strip leading whitespace
        if ((tokenizerEntry[0] == 0) || (tokenizerEntry[0] == '#'))
            continue; // Ignore empty lines and comments (lines that start with '#')
        // Parse line of 3 decimal numbers (register indices: dst src1 src2)
        for (i = 0; i < 3; ++i) {
            curField = strtok(tokenizerEntry, " \t\n\r");
            if (curField == NULL) {
                printf("Error parsing instruction #%u of %s\n", lineNum, filename);
                return -2;
            }
            fieldVal[i] = strtol(curField, &endOfVal, 10);
            if (endOfVal[0] != 0) {
                printf("Failed parsing field %d of line #%u of %s: %s\n", i, lineNum, filename, curLine);
                return -2;
            }
            tokenizerEntry = NULL; // for next tokens should provide strtok NULL
        }
        progBuf[lineNum].dstIdx = fieldVal[0];
        progBuf[lineNum].src1Idx = fieldVal[1];
        progBuf[lineNum].src2Idx = fieldVal[2];
        ++lineNum;
	}
    
    fclose(progFile);
    *prog = progBuf; // To return to caller
    return lineNum;
}

void usage(void) {
    printf("Usage: dflow_calc <program filename> <Query> [Query...]\n");
    printf("\tQuery: [p|d|u]<program line#> - Report [dependency depth| dependency of this inst.| uses of this inst.]\n");
    printf("Example: dflow_calc example1.in d4 d7 u6 p12\n");
    exit(1);
}

int main(int argc, const char *argv[]) {
    const char *progName = argv[1];
    InstInfo *theProg;
    int progLen, i, rc;
    int src1Dep, src2Dep, curUser;
    ProgCtx ctx;
    char *endPtr;
    unsigned int users[MAX_USERS_DUMP];

    if (argc < 3) {
        usage();
    }

    printf("Reading the program file %s ... ", progName);
    progLen = readProgram(progName, &theProg);
    if (progLen <= 0) {
        printf("Error reading program file %s!\n", progName);
        exit(1);
    }
    printf("Found %d instructions\n", progLen);
    // Analyze the program
    ctx = analyzeProg(theProg, progLen);
    if (ctx == PROG_CTX_NULL) {
        printf("Error on invocation to analyzeCtx()\n");
        exit(2);
    }
    // Read queries and ask them
    for (i = 2; i < argc; ++i) {
        const char qType = argv[i][0];
        const unsigned int instNum = strtol(argv[i]+1, &endPtr, 10);
        if (*endPtr != 0) {
            printf("Error: Invalid instruction number in the query: %s\n", argv[i]);
            exit(3);
        }
        switch (qType) {
        case 'p': // Dependency depth
            rc = getDepDepth(ctx, instNum);
            if (rc < 0) {
                printf("Error %d for getDepDepth(%u)\n", rc, instNum);
            } else {
                printf("getDepDepth(%u)==%d\n", instNum, rc);
            }
            break;
        case 'd': // Instruction dependencies
            rc = getInstDeps(ctx, instNum, &src1Dep, &src2Dep);
            if (rc != 0) {
                printf("Error %d for getInstDeps(%u)\n", rc, instNum);
            } else {
                printf("getInstDeps(%u)=={%d,%d}\n", instNum, src1Dep, src2Dep);
            }
            break;
        case 'u': // Instruction users
            rc = getInstUsers(ctx, instNum, MAX_USERS_DUMP, users);
            if (rc < 0) {
                printf("Error %d for getInstUsers(%u)\n", rc, instNum);
            } else if (rc > MAX_USERS_DUMP) {
                printf("Instruction %u has more than %u users!", instNum, MAX_USERS_DUMP);
            } else {
                printf("getInstUsers(%u)==%d: ", instNum, rc);
                for (curUser = 0; curUser < rc; ++curUser) {
                    printf("%u  ", users[curUser]);
                }
                printf("\n");
            }
            break;
        default:
            printf("Invalid query type '%c' in argument '%s'\n", qType, argv[i]);
            exit(3);
        }
    }
    freeProgCtx(ctx);
    free(theProg); // We keep theProg up to here to allow your analyzer to use it - if it wants
    return 0;
}
