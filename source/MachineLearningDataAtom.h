//	Author: Kozlov S.A. since 2018

#ifndef MachineLearningDataAtom_H_INCLUDED
#define MachineLearningDataAtom_H_INCLUDED

#include <fstream>

constexpr size_t data_atom_maximum_size = 500;

class MachineLearningDataAtom
{
public:
	MachineLearningDataAtom();
	~MachineLearningDataAtom();

	std::ifstream& operator>>(std::ifstream &input_stream);
	std::ofstream& operator<<(std::ofstream &output_stream);

	std::string GetData();
	void SetData(std::string parameter_data);

protected:

	char data[data_atom_maximum_size];
};

std::ifstream& operator>>(std::ifstream &input_stream, MachineLearningDataAtom &MachineLearningDataAtom_parameter);
std::ofstream& operator<<(std::ofstream &output_stream, MachineLearningDataAtom &MachineLearningDataAtom_parameter);

#endif
