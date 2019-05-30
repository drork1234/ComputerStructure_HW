#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <string>

#include "bp_api.h"

using namespace std;

#define INVALID_ARG_COUNT -1
#define INVALID_FILE_PATH -2
#define BROKEN_FILE -3
#define GEN_ERROR -4

int main(int argc, char** argv) {

	if (argc < 2){
		cerr << __func__ << ": Error: correct usage is : '" << argv[0] << "' <trace filename>" << endl;
		return INVALID_ARG_COUNT;
	}

	ifstream i_stream(argv[1]);
	if (!i_stream.is_open()){
		cerr << __func__ << ": Error: could not open file at path " << argv[1] << endl;
		return INVALID_FILE_PATH;
	}

	unsigned btbSize, BHRlength;
	string sBHRtype, sTableType, sIsShare;
	string line;

	getline(i_stream, line);
	istringstream first_line_stream(line);

	//parse the first line of the file
	if (!(first_line_stream >> btbSize >> BHRlength >> sBHRtype >> sTableType >> sIsShare)) {
		cerr << __func__ << ": Error: first line of file at path " << argv[1] << " is broken. Exisiting...";
		i_stream.close();
		return BROKEN_FILE;
	}

	if (0 >= btbSize || 0 >= BHRlength || 8 < BHRlength){
		cerr << __func__ << ": Error: configuration arguments are invalid" << endl;
		i_stream.close();
		return BROKEN_FILE;
	}

	bool isShare, isGlobalTable, isGlobalHist;

	if ("global_history" == sBHRtype) isGlobalHist = true;
	else if ("local_history" == sBHRtype) isGlobalHist = false;
	else {
		cerr << __func__ << ": Error: configuration arguments are invalid" << endl;
		i_stream.close();
		return BROKEN_FILE;
	}

	if ("global_tables" == sTableType) isGlobalTable = true;
	else if ("local_tables" == sTableType) isGlobalTable = false;
	else{
		cerr << __func__ << ": Error: configuration arguments are invalid" << endl;
		i_stream.close();
		return BROKEN_FILE;
	}

	if ("using_share" == sIsShare) isShare = true;
	else if ("not_using_share" == sIsShare) isShare = false;
	else {
		cerr << __func__ << ": Error: configuration arguments are invalid" << endl;
		i_stream.close();
		return BROKEN_FILE;
	}

	if (0 > BP_init(btbSize, BHRlength, isGlobalHist, isGlobalTable, isShare)) {
		cerr << __func__ << ": Error: predictor initialization failed" << endl;
		i_stream.close();
		return GEN_ERROR;
	}

	map<unsigned, bool> prediction_selector;
	unsigned counter = 0;

	while (getline(i_stream, line)){
		istringstream parser(line);
		string sPc;

		if (!(parser >> sPc)) break;

		unsigned pc = (unsigned)strtoul(sPc.c_str(), NULL, 0);

		if (prediction_selector.end() == prediction_selector.find(pc))
			prediction_selector[pc] = (counter % 2) ? false : true;
		
		counter++;
	}

	i_stream.clear();
	i_stream.seekg(0, i_stream.beg);

	//advance to the second line
	getline(i_stream, line);

	while (getline(i_stream, line)){
		istringstream parser(line);
		string sPc, sTargetPc;

		unsigned pc, targetPc;
		char sczTaken;
		bool taken;

		if (!(parser >> sPc >> sczTaken >> sTargetPc))
			break;

		pc = strtoul(sPc.c_str(), NULL, 0);
		targetPc = strtoul(sTargetPc.c_str(), NULL, 0);

		if ('T' == sczTaken) taken = true;
		else if ('N' == sczTaken) taken = false;
		else {
			cerr << __func__ << ": Error: bad trace file" << endl;
			i_stream.close();
			return BROKEN_FILE;
		}

		uint32_t dst = 0;
		cout << "0x" << std::hex << pc << " ";
		cout << (BP_predict(pc, &dst) ? "T" : "N") << " ";
		cout << "0x" << std::hex << dst << endl;

		if (prediction_selector[pc]){
			BP_setBranchAt(pc);
			BP_update(pc, targetPc, taken);
		}
	}

	i_stream.close();

	return 0;
}