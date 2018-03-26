//	Author: Kozlov S.A. since 2018

#ifndef MachineLearningDataAtom_H_INCLUDED
#define MachineLearningDataAtom_H_INCLUDED

#include <fstream>

class MachineLearningDataAtom
{
public:
	MachineLearningDataAtom();
	~MachineLearningDataAtom();

	std::ifstream& operator>>(std::ifstream &input_stream);
	std::ofstream& operator<<(std::ofstream &output_stream);

	char GetData();

protected:

	char TestChar;
};

std::ifstream& operator>>(std::ifstream &input_stream, MachineLearningDataAtom &MachineLearningDataAtom_parameter);
std::ofstream& operator<<(std::ofstream &output_stream, MachineLearningDataAtom &MachineLearningDataAtom_parameter);

#endif
