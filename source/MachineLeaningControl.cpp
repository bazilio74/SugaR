//	Author: Kozlov S.A. since 2018

#include "MachineLeaningControl.h"

#include <fstream>


MachineLearningControl MachineLearningControlMain;


MachineLearningControl::MachineLearningControl()
{
}


MachineLearningControl::~MachineLearningControl()
{
}


void MachineLearningControl::SetFileName(std::string parameter_file_name)
{
	file_name = parameter_file_name;
}


//	0 - success
//	1 - fail
int MachineLearningControl::LoadData()
{
	std::ifstream input_stream;
	
	input_stream.open(file_name);

	if (input_stream.is_open())
	{
		if (!input_stream.eof())
		{
			do
			{
				MachineLearningDataAtom MachineLearningDataAtomCurrent;

				input_stream >> MachineLearningDataAtomCurrent;

				MachineLearningDataStore.push_back(MachineLearningDataAtomCurrent);

			} while (!input_stream.eof());
		}
		else
		{
			return 1;
		}

		if (MachineLearningDataStore.size() > 0)
		{
			MachineLearningDataStore.pop_back();
		}
	}
	else
	{
		return 1;
	}

	return 0;
}


//	0 - success
//	1 - fail
int MachineLearningControl::SaveData()
{
	std::ofstream output_stream;
	
	output_stream.open(file_name);

	if (output_stream.is_open())
	{
		MachineLearningDataListIterator MachineLearningDataListIteratorCurrent = MachineLearningDataStore.begin();

		if (MachineLearningDataListIteratorCurrent != MachineLearningDataStore.end())
		{
			do
			{
				MachineLearningDataAtom MachineLearningDataAtomCurrent;

				MachineLearningDataAtomCurrent = *MachineLearningDataListIteratorCurrent;

				output_stream << MachineLearningDataAtomCurrent.GetData();

				MachineLearningDataListIteratorCurrent++;

			} while (MachineLearningDataListIteratorCurrent != MachineLearningDataStore.end());
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

void MachineLearningControl::ClearData()
{
	MachineLearningDataStore.clear();
}

void MachineLearningControl::StartLearning()
{

}


void MachineLearningControl::EndLearning()
{

}