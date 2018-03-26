//	Author: Kozlov S.A. since 2018

#ifndef MachineLeaningControl_H_INCLUDED
#define MachineLeaningControl_H_INCLUDED

#include <string>
#include <list>

#include "MachineLearningData.h"

class MachineLearningControl
{
protected:
	std::string file_name;

	typedef std::list<MachineLearningDataAtom> MachineLearningDataList;
	typedef std::list<MachineLearningDataAtom>::iterator MachineLearningDataListIterator;
	MachineLearningDataList MachineLearningDataStore;
public:
	MachineLearningControl();
	~MachineLearningControl();

	void SetFileName(std::string parameter_file_name);

protected:
	//	0 - success
	//	1 - fail
	int LoadData();
	//	0 - success
	//	1 - fail
	int SaveData();
};

extern MachineLearningControl MachineLearningControlMain;

#endif