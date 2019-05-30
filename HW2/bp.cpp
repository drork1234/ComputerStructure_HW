/* 046267 Computer Architecture - Spring 2016 - HW #2 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <exception>
#include <stdexcept>


using namespace std;

#define SNT 0
#define WNT 1
#define WT 2
#define ST 3

#define PREDICTION(x) (x & 0b10) >> 1

#define TAG(pc, mask) ( pc & mask) >> 2

#define HISTORY(unmasked, mask) (unmasked & mask)

class PredictionTable;
class BranchTargetBuffer;
class BranchPredictor;

class PredictionTable
{
public:
	PredictionTable(unsigned char histTableSize) : m_table_tag_mask(0x1) {
		while (--histTableSize)
			m_table_tag_mask = (m_table_tag_mask << 1) | 0x1;
		
	}
	virtual ~PredictionTable(){}

	virtual bool Prediction(unsigned char history, unsigned  = 0) = 0;

	virtual void Update(unsigned char history, bool actual_prediction, unsigned int = 0) = 0;

protected:
	unsigned char m_table_tag_mask;
};

class LocalTable : public PredictionTable
{
public:
	LocalTable(unsigned btbSize, unsigned histSize) : PredictionTable(histSize), m_tag_mask(0x1) {

		m_tables.clear();
		m_tables.resize(btbSize);

		const unsigned table_size = (unsigned)pow(2, histSize);
		//initialize all state machines to Weakly-Not-Taken (WNT)
		for (size_t i = 0; i < m_tables.size(); i++){
			m_tables[i].reserve(table_size);
			
			for (size_t j = 0; j < table_size; j++)
				m_tables[i].push_back(WNT);
		}

		while (btbSize / 2 - 1){
			m_tag_mask = (m_tag_mask << 1) | 0x1;
			btbSize /= 2;
		}

		m_tag_mask = m_tag_mask << 2;
	}
	virtual ~LocalTable() {
		m_tables.clear();
	}

	virtual bool Prediction(unsigned char history, unsigned pc) {
		//should not happen in this stage
		if ((TAG(pc, m_tag_mask)) >= m_tables.size() ||
			(unsigned)(HISTORY(history, m_table_tag_mask)) >= m_tables[0].size())
			throw std::exception();

		unsigned char history_machine_state = m_tables[TAG(pc, m_tag_mask)][HISTORY(history, m_table_tag_mask)];
		int prediction = PREDICTION(history_machine_state);

		return prediction ? true : false;
	}

	virtual void Update(unsigned char history, bool actual_prediction, unsigned int pc) {
		//should not happen in this stage
		if ((TAG(pc, m_tag_mask)) >= m_tables.size() || 
			(unsigned)(HISTORY(history, m_table_tag_mask)) >= m_tables[0].size())
			throw std::exception();

		//else
		unsigned char& decision = m_tables[TAG(pc, m_tag_mask)][HISTORY(history, m_table_tag_mask)];
		switch (decision)
		{
		case SNT: 
			decision = (actual_prediction) ? (WNT) : (SNT);
			break;
		case WNT: 
			decision = (actual_prediction) ? (WT) : (SNT);
			break;
		case WT: 
			decision = (actual_prediction) ? (ST) : (WNT);
			break;
		case ST: 
			decision = (actual_prediction) ? (ST) : (WT);
			break;
			//should not happen
		default:
			throw std::exception();

		}

		return;
	}

	void InitAt(uint32_t pc) {
		//should not happen
		if (TAG(pc, m_tag_mask) >= m_tables.size())
			throw std::exception();

		const unsigned branch_tag = TAG(pc, m_tag_mask);
		const unsigned table_size = m_tables[0].size();

		//flush the table
		m_tables[branch_tag].resize(0);
		
		//realloc and reset
		m_tables[branch_tag].reserve(table_size);

		//reset all state machines at the table to 'WNT' (Weakly Not Taken)
		for (size_t i = 0; i < table_size; i++)
			m_tables[branch_tag].push_back(WNT);

		return;
	}

private:
	unsigned int m_tag_mask;
	std::vector<std::vector<unsigned char> > m_tables;
};

class GlobalTable : public PredictionTable
{
public:
	GlobalTable(unsigned char histTableSize) : PredictionTable(histTableSize){
		const unsigned table_size = (unsigned)pow(2, histTableSize);
		m_tables.reserve(table_size);

		for (size_t i = 0; i < table_size; i++)
			m_tables.push_back(WNT);

		return;
	}
	virtual ~GlobalTable() {
		m_tables.clear();
	}

	virtual bool Prediction(unsigned char history, unsigned = 0) {
		if ((unsigned)(HISTORY(history, m_table_tag_mask)) >= m_tables.size())
			throw std::exception();

		//else 
		return (PREDICTION(m_tables[HISTORY(history, m_table_tag_mask)])) ? true : false;
	}

	virtual void Update(unsigned char history, bool actual_prediction, unsigned int = 0) {
		unsigned masked_history = HISTORY(history, m_table_tag_mask);
		//should not happen here
		if ( masked_history >= m_tables.size())
			throw std::exception();

		unsigned char& decision = m_tables[HISTORY(history, m_table_tag_mask)];
		switch (decision)
		{
		case SNT:
			decision = (actual_prediction) ? (WNT) : (SNT);
			break;
		case WNT:
			decision = (actual_prediction) ? (WT) : (SNT);
			break;
		case WT:
			decision = (actual_prediction) ? (ST) : (WNT);
			break;
		case ST:
			decision = (actual_prediction) ? (ST) : (WT);
			break;
			//should not happen
		default:
			throw std::exception();

		}

		return;
	}
private:
	std::vector<unsigned char> m_tables;
};


class BranchTargetBuffer
{
	
public:
	BranchTargetBuffer(unsigned btbSize, unsigned histSize) : m_btb_tag_mask(0x1), m_history_mask(0x1){
		m_buffer.clear();
		m_buffer.reserve(btbSize);

		for (size_t i = 0; i < btbSize; i++)
			m_buffer.push_back(BTB_line(0, 0));

		while (--histSize) 
			m_history_mask = (m_history_mask << 1) | 0x1;
		

		while ((btbSize / 2) - 1) {
			m_btb_tag_mask = (m_btb_tag_mask << 1) | 0x1;
			btbSize /= 2;
		}
		m_btb_tag_mask = m_btb_tag_mask << 2;

	}
	virtual ~BranchTargetBuffer() {
		m_buffer.clear();
	}

	virtual unsigned char ExtractHistory(uint32_t pc, uint32_t* dst) = 0;

	virtual void Update(uint32_t pc, uint32_t targetPc, bool taken) = 0;

	virtual bool InitAt(uint32_t pc) {
		if (m_buffer[TAG(pc, m_btb_tag_mask)].first == pc) return false;

		m_buffer[TAG(pc, m_btb_tag_mask)] = BTB_line(pc, 0);
		return true;
	}

protected:
	typedef std::pair<unsigned, unsigned> BTB_line;
	std::vector<BTB_line> m_buffer;

	unsigned int  m_btb_tag_mask;
	unsigned char m_history_mask;
};

class LocalBTB : public BranchTargetBuffer
{
public:
	LocalBTB(unsigned btbSize, unsigned histSize) : BranchTargetBuffer(btbSize, histSize) {
		m_histories.clear();
		m_histories.reserve(btbSize);

		for (size_t i = 0; i < btbSize; i++) m_histories.push_back(0x0);
	}
	virtual ~LocalBTB() {
		m_histories.clear();
	}

	virtual unsigned char ExtractHistory(uint32_t pc, uint32_t* dst) {
		//if the tag of the pc is not found in the buffer throw an exception
		//The branch predictor will handle it accordingly
		if (m_buffer[TAG(pc, m_btb_tag_mask)].first != pc)
			throw std::exception();

		if(NULL != dst) *dst = m_buffer[TAG(pc, m_btb_tag_mask)].second;
		//else
		//in case the history is not masked (should not happen, handled by Update), mask it
		return m_histories[TAG(pc, m_btb_tag_mask)] & m_history_mask;
	}

	virtual void Update(uint32_t pc, uint32_t targetPc, bool taken) {
		const unsigned branch_tag = TAG(pc, m_btb_tag_mask);
		///should not happen at this stage
		if (m_buffer[branch_tag].first != pc)
			throw std::exception();

		m_buffer[branch_tag] = BTB_line(pc, targetPc);

		//shift the history by one bit, then perform an OR operation, then mask it 
		m_histories[branch_tag] = ((m_histories[branch_tag] << 1) | int(taken)) & m_history_mask;
		
		return;
	}

	virtual bool InitAt(uint32_t pc) {
		if (!BranchTargetBuffer::InitAt(pc)) return false;
		
		m_histories[TAG(pc, m_btb_tag_mask)] = 0x0;
		return true;
	}

protected:
	std::vector<unsigned char> m_histories;
};

class LShareBTB : public LocalBTB
{
public:
	LShareBTB(unsigned btbSize, unsigned histSize) : LocalBTB(btbSize, histSize) {}
	virtual ~LShareBTB() {}

	virtual unsigned char ExtractHistory(uint32_t pc, uint32_t* dst) {
		//if the tag of the pc is not found in the buffer throw an exception
		//The branch predictor will handle it accordingly
		const unsigned branch_tag = TAG(pc, m_btb_tag_mask);
		if (m_buffer[branch_tag].first != pc)
			throw std::exception();

		if (NULL != dst) *dst = m_buffer[branch_tag].second;
		//else
		//in case the history is not masked (should not happen, handled by Update), mask it
		unsigned char char_tag = branch_tag & 0xFF;
		char_tag &= m_history_mask;

		//here the 'share' feature is implemented by bitwise XOR between the tag and the history
		return m_histories[branch_tag] ^ char_tag;
	}

private:

};

class GlobalBTB : public BranchTargetBuffer
{
public:
	GlobalBTB(unsigned btbSize, unsigned histSize) : BranchTargetBuffer(btbSize, histSize), m_history(0x0) {}
	virtual ~GlobalBTB() {}

	virtual unsigned char ExtractHistory(uint32_t pc, uint32_t* dst) {
		const unsigned tag = TAG(pc, m_btb_tag_mask);
		if (m_buffer[tag].first != pc)
			throw std::exception();

		if(NULL != dst) *dst = m_buffer[tag].second;

		return m_history;
	}

	virtual void Update(uint32_t pc, uint32_t targetPc, bool taken) {
		const unsigned branch_tag = TAG(pc, m_btb_tag_mask);

		///should not happen at this stage
		if (m_buffer[branch_tag].first != pc)
			throw std::exception();

		m_buffer[branch_tag].second = targetPc;

		//shift the history by one bit, then perform an OR operation, then mask it 
		m_history = ((m_history << 1) | int(taken)) & m_history_mask;

		return;
	}


protected:
	unsigned char m_history;
};

class GShareBTB : public GlobalBTB
{
public:
	GShareBTB(unsigned btbSize, unsigned histSize) : GlobalBTB(btbSize, histSize) {}
	virtual ~GShareBTB() {}

	virtual unsigned char ExtractHistory(uint32_t pc, uint32_t* dst) {
		const unsigned tag = TAG(pc, m_btb_tag_mask);
		if (m_buffer[tag].first != pc)
			throw std::exception();

		if (NULL != dst) *dst = m_buffer[tag].second;

		unsigned char char_tag = tag & 0xFF;
		char_tag &= m_history_mask;

		//here the 'share' feature is implemented by bitwise XOR between the tag and the history
		return m_history ^ char_tag ;
	}

private:

};


class BranchPredictor
{
public:
	BranchPredictor() : m_btb(NULL), m_tables(NULL) {}
	~BranchPredictor() {
		if(NULL != m_btb)		delete m_btb;
		if(NULL != m_tables)	delete m_tables;
  }

	void Reset(unsigned btbSize, unsigned historySize,
		bool isGlobalHist, bool isGlobalTable, bool isShare) {
		
		if (!isGlobalTable && isShare)
			throw std::runtime_error("");
		
		if (NULL != m_btb)
			delete m_btb;

		if (isGlobalHist){
			if (isGlobalTable)	
				m_btb = new GShareBTB(btbSize, historySize);
			else
				m_btb = new GlobalBTB(btbSize, historySize);
		} 
		else{
			if (isGlobalTable)
				m_btb = new LShareBTB(btbSize, historySize);
			else	
				m_btb = new LocalBTB(btbSize, historySize);
		}
			
		if (isGlobalTable)
				m_tables = new GlobalTable(historySize);
		else	m_tables = new LocalTable(btbSize, historySize);

		m_is_global_table = isGlobalTable;
		m_is_share = isShare;
		return;
	}

	void InitAt(uint32_t pc) {
		if (m_btb->InitAt(pc)){
			if (!m_is_global_table)
				dynamic_cast<LocalTable*>(m_tables)->InitAt(pc);
		}

		return;
	}

	bool Predict(uint32_t pc, uint32_t *dst) {
		try
		{
			unsigned char history = m_btb->ExtractHistory(pc, dst);
			//add here bit concatenation in case that isShare is true
			bool prediction = m_tables->Prediction(history, pc);
			*dst = (prediction) ? *dst : (pc + 4);

			return prediction;
		}
		catch (const std::exception&)
		{
			*dst = pc + 4;
			return false;
		}
	}

	void Update(uint32_t pc, uint32_t targetPc, bool taken) {
		try
		{
			unsigned char history = m_btb->ExtractHistory(pc, NULL);
			m_btb->Update(pc, targetPc, taken);
			m_tables->Update(history, taken, pc);
			return;
		}
		catch (...)
		{
			return;
		}
	}

private:
	BranchTargetBuffer* m_btb;
	PredictionTable* m_tables;
	bool m_is_global_table;
	bool m_is_share;
};


static BranchPredictor Predictor;


int BP_init(unsigned btbSize, unsigned historySize,
             bool isGlobalHist, bool isGlobalTable, bool isShare){
	try
	{
		Predictor.Reset(btbSize, historySize, isGlobalHist, isGlobalTable, isShare);
	}
	catch (const std::bad_alloc& AllocExp)
	{
		AllocExp.what();
		return -1;
	}
	catch (const std::runtime_error& RTExp) {
		RTExp.what();
		return -1;
	}
	
	if (!isGlobalTable && isShare) return -1;

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){

	return Predictor.Predict(pc, dst);
}

void BP_setBranchAt(uint32_t pc){
	Predictor.InitAt(pc);
	return;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken){
	Predictor.Update(pc, targetPc, taken);
	return;
}
