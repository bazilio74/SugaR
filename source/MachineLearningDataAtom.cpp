//	Author: Kozlov S.A. since 2018

#include "MachineLearningDataAtom.h"


MachineLearningDataAtom::MachineLearningDataAtom()
{
}


MachineLearningDataAtom::~MachineLearningDataAtom()
{
}

std::ifstream& MachineLearningDataAtom::operator>>(std::ifstream &input_stream)
{
	input_stream.get(TestChar);

	return input_stream;
}

std::ofstream& MachineLearningDataAtom::operator<<(std::ofstream &output_stream)
{
	output_stream.put(TestChar);

	return output_stream;
}

std::ifstream& operator>>(std::ifstream &input_stream, MachineLearningDataAtom &MachineLearningDataAtom_parameter)
{
	return MachineLearningDataAtom_parameter.operator>>(input_stream);
}

std::ofstream& operator<<(std::ofstream &output_stream, MachineLearningDataAtom &MachineLearningDataAtom_parameter)
{
	return MachineLearningDataAtom_parameter.operator<<(output_stream);
}


char MachineLearningDataAtom::GetData()
{
	return TestChar;
}