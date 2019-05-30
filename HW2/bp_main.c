/* 046267 Computer Architecture - Spring 2016 - HW #2	*/
/* Main program                  					 	*/
/* Usage: ./bp_main <trace filename>  				 	*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "bp_api.h"

int main(int argc, char **argv) {
	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <trace filename>\n", argv[0]);
		exit(1);
	}

	FILE *trace = fopen(argv[1], "r");
	if (trace == 0) {
		fprintf(stderr, "cannot open trace file\n");
		exit(2);
	}

	char line[1024];
	if (fgets(line, 256, trace) == NULL) {
		fprintf(stderr, "Error in input file: cannot read config\n");
		exit(3);
	}
	char *elemnts[5];
	int i = 0;
	elemnts[0] = strtok(line, " ");
	for (i = 1; i < 5; ++i) {
		elemnts[i] = strtok(NULL, " \n");
	}

	int btbSize = strtoul(elemnts[0], NULL, 0);
	int historySize = strtoul(elemnts[1], NULL, 0);
	if (btbSize == 0 || historySize == 0) {
		fprintf(stderr, "Error in input file: cannot read config\n");
		exit(4);
	}
	bool isGlobalHist;
	if (strcmp(elemnts[2], "local_history") == 0) {
		isGlobalHist = false;
	} else if (strcmp(elemnts[2], "global_history") == 0) {
		isGlobalHist = true;
	} else {
		fprintf(stderr, "Error in input file: cannot read config\n");
		exit(5);
	}
	bool isGlobalTable;
	if (strcmp(elemnts[3], "local_tables") == 0) {
		isGlobalTable = false;
	} else if (strcmp(elemnts[3], "global_tables") == 0) {
		isGlobalTable = true;
	} else {
		fprintf(stderr, "Error in input file: cannot read config\n");
		exit(6);
	}
	bool isShare;
	if (strcmp(elemnts[4], "using_share") == 0) {
		isShare = true;
	} else if (strcmp(elemnts[4], "not_using_share") == 0) {
		isShare = false;
	} else {
		fprintf(stderr, "Error in input file: cannot read config\n");
		exit(7);
	}

	if (BP_init(btbSize, historySize, isGlobalHist,
			isGlobalTable, isShare) < 0) {
		fprintf(stderr, "Predictor init failed\n");
		exit(8);
	}

	while ((fgets(line, 256, trace) != NULL)) {
		if (line[0] == '\n') {
			break;
		}
		char *elemnts[3];
		int i = 0;
		elemnts[0] = strtok(line, " ");
		for (i = 1; i < 3; ++i) {
			elemnts[i] = strtok(NULL, " \n");
		}
		uint32_t pc = (uint32_t) strtol(elemnts[0], NULL, 0);
		uint32_t targetPc = (uint32_t) strtol(elemnts[2], NULL, 0);
		bool taken;
		if (strcmp(elemnts[1], "T") == 0) {
			taken = true;
		} else if (strcmp(elemnts[1], "N") == 0) {
			taken = false;
		} else {
			fprintf(stderr, "Error in input file: bad trace\n");
			exit(9);
		}
		uint32_t dst = 0;
		printf("0x%x ", pc);
		printf("%c ", (BP_predict(pc, &dst)? 'T' : 'N'));
		printf("0x%x\n", dst);
		BP_setBranchAt(pc);
		BP_update(pc, targetPc, taken);
	}
 
  fclose(trace);
	return 0;
}

