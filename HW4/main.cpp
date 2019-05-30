#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

#include "CacheSim.h"

#define INVALID_CMD -1
#define INAVLID_FILE -2

#define NUM_ARGS 8
#define CACHE_ARGS 3

using namespace std;

vector<cache_args> AssembleCacheArgs(vector<unsigned> const& args) {
	vector<cache_args> caches;
	caches.reserve((args.size() - 2) / CACHE_ARGS);
	for (size_t i = 0; i < caches.capacity(); i++) {
		cache_args cache_arg;
		cache_arg.resize(CACHE_ARGS);
		for (size_t j = 0; j < CACHE_ARGS; j++)
			cache_arg[j] = args[2 + (i * CACHE_ARGS) + j];

		caches.push_back(cache_arg);
	}

	return caches;
}

int main(int argc, char* argv[]) {
	if (argc < 17){
		cerr << __func__ << "Wrong usage: should be:" << endl;
		cerr << "./cacheSim <input file> --mem-cyc <# of cycles> --bsize <block log2(size)> "
				"--l1-size <log2(size)> --l1-assoc<log2(# of ways)> --l1-cyc <# of cycles> "
				"--l2-size <log2(size)> --l2-assoc<log2(# of ways)> --l2-cyc <# of cycles>" << endl;

		return INVALID_CMD;
	}

	ifstream file(argv[1]);
	if (!file){
		cerr << __func__ << ": I/O error: file at path " << argv[1] << " could not be found" << endl;
		return INAVLID_FILE;
	}

	string tokens[NUM_ARGS] = {	"--mem-cyc",
					"--bsize",
					"--l1-size",
					"--l1-assoc",
					"--l1-cyc",
					"--l2-size",
					"--l2-assoc",
					"--l2-cyc" };

	vector<unsigned> args;
	vector<string> sArgs;
	args.reserve(NUM_ARGS);
	sArgs.reserve(argc);
	for (int i = 0; i < argc; i++)
		sArgs.push_back(argv[i]);



	for (size_t i = 0; i < NUM_ARGS; i++) {
		auto found_token_it = find(sArgs.begin(), sArgs.end(), tokens[i]);

		//if the token wasn't found it means that the CMD line is broken, so exit
		if (sArgs.end() == found_token_it){
			cerr << __func__ << "Wrong usage: should be:" << endl;
			cerr << "./cacheSim <input file> --mem-cyc <# of cycles> --bsize <block log2(size)> "
				"--l1-size <log2(size)> --l1-assoc<log2(# of ways)> --l1-cyc <# of cycles> "
				"--l2-size <log2(size)> --l2-assoc<log2(# of ways)> --l2-cyc <# of cycles>" << endl;

			return INVALID_CMD;
		}
		args.push_back(strtoul((found_token_it + 1)->c_str(), NULL, 0));
	}

	//extract the specific arguments of each cache
	vector<cache_args> caches = AssembleCacheArgs(args);

	CacheSim Simulator(args[0], args[1], caches);
	
	char op;
	unsigned address;
	string hexAddress;
	
	//main loop
	//read from the start of the file to the end
	while (file >> op >> hexAddress){
		
		//convert the address from string to unsigned
		address = strtoul(hexAddress.c_str(), NULL, 0);
		

		//if the operation isn't recognized by the simulator continue to the next line
		if (tolower(op) != 'w' && tolower(op) != 'r') continue;

		//here call the cache with the data acquired by
		Simulator.Access(op, address);
		
	}

	//output the statistics of the cache access
	cout << fixed << setprecision(3);
	try
	{
		cout << "L1miss=" << Simulator.L1MissRate() << " ";
		cout << "L2miss=" << Simulator.L2MissRate() << " ";
		cout << "AccTimeAvg=" << Simulator.AvgAccTime() << endl;

	}
	catch (const std::runtime_error& RTError){
		cerr << "Runtime Exception: " << RTError.what() << endl;
		return -1;
	}
	
	file.close();
	return 0;
}
