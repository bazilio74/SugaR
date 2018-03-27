//	Author: Kozlov S.A. since 2018

#include "MachineLearningDataAtom.h"


MachineLearningDataAtom::MachineLearningDataAtom()
{
	memset(data, 0, data_atom_maximum_size);
}


MachineLearningDataAtom::~MachineLearningDataAtom()
{
}

std::ifstream& MachineLearningDataAtom::operator>>(std::ifstream &input_stream)
{
	input_stream.getline(data, data_atom_maximum_size);

	return input_stream;
}

std::ofstream& MachineLearningDataAtom::operator<<(std::ofstream &output_stream)
{
	size_t data_counter = 0;
	for (; data[data_counter]!=char(0) && data_counter<data_atom_maximum_size; data_counter++);

	output_stream.write(data, data_counter);

	output_stream << std::endl;

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


std::string MachineLearningDataAtom::GetData()
{
	return std::string(data);
}

void MachineLearningDataAtom::SetData(std::string parameter_data)
{
	memset(data, 0, data_atom_maximum_size);
	strcpy_s(data, data_atom_maximum_size, parameter_data.c_str());
}