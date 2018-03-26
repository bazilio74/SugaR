//	Author: Kozlov S.A. since 2018


#include <fstream>
#include <sstream>
#include <iostream>

#include "MachineLeaningControl.h"

#include "thread.h"
#include "search.h"
#include "UCI.h"
#include "types.h"
#include "movegen.h"

MachineLearningControl MachineLearningControlMain;
void learning_go_call(Position& pos, std::istringstream& is, StateListPtr& states);
void learning_position_call(Position& pos, std::istringstream& is, StateListPtr& states);

MachineLearningControl::MachineLearningControl()
	:learning_in_progress(false), learning_move_returned(true), current_position_set(false),
	states(NULL), learning_thread(&MachineLearningControl::learning_thread_function, this)
{
}


MachineLearningControl::~MachineLearningControl()
{
	learning_thread.join();
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

				output_stream << MachineLearningDataAtomCurrent;

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


void MachineLearningControl::StartLearning(Position &position_parameter, std::istringstream& is, StateListPtr& parameter_states)
{
	learning_in_progress = true;

	states = &parameter_states;
	
	current_position.init();
	current_position.set(position_parameter.fen(), Options["UCI_Chess960"], &states->get()->back(), Threads.main());

	current_position_set = true;
}


void MachineLearningControl::EndLearning()
{
	learning_in_progress = false;

	current_position_set = false;
}


void MachineLearningControl::Answer(Move parameter_Move, bool parameter_960)
{
	if (learning_in_progress)
	{
		MachineLearningDataAtom MachineLearningDataAtomCurrent;

		std::string parameter_answer = UCI::move(parameter_Move, parameter_960);

		Last_Move = parameter_Move;
		is_960 = parameter_960;

		MachineLearningDataAtomCurrent.SetData(parameter_answer);

		MachineLearningDataStore.push_back(MachineLearningDataAtomCurrent);

		learning_move_returned = true;
	}
}

void MachineLearningControl::learning_thread_function()
{
	while (true)
	{
		while (!current_position_set)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		StateInfo st;
		std::memset(&st, 0, sizeof(StateInfo));

		Options["Threads"] = 1;

		std::string fen_saved = current_position.fen();

		while (learning_in_progress)
		{
			learning_move_returned = false;

			Search::LimitsType limits;

			bool ponderMode = false;

			limits.startTime = now();
			
			{
				std::string input_stream_data("fen ");
				input_stream_data += fen_saved;
				std::istringstream input_stream(input_stream_data);

				learning_position_call(current_position, input_stream, *states);
			}

			std::cout << current_position << std::endl;

			Color us = current_position.side_to_move();

			auto moveList = MoveList<LEGAL>(current_position);

			if (moveList.size() == 0)
			{
				EndLearning();

				if (current_position.checkers())
				{
					if (us == WHITE)
					{
						std::cout << "Game over: black wins" << std::endl;
					}
					else
					{
						if (us == BLACK)
						{
							std::cout << "Game over: white wins" << std::endl;
						}
						else
						{
							assert(false);
						}
					}
				}
				else
				{
					std::cout << "Game over: draw" << std::endl;
				}

				continue;
			}

			{
				std::string input_stream_data("go depth 4");
				std::istringstream input_stream(input_stream_data);

				learning_go_call(current_position, input_stream, *states);
			}


			while (!learning_move_returned)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			if (!is_ok(Last_Move))
			{
				EndLearning();

				std::cout << "Game over" << std::endl;

				continue;
			}

			if (current_position.legal(Last_Move))
			{
				current_position.do_move(Last_Move, st);

				fen_saved = current_position.fen();

				if (!current_position.pos_is_ok())
				{
					EndLearning();
					
					current_position.undo_move(Last_Move);

					std::cout << "Game over" << std::endl;

					continue;
				}

				current_position.undo_move(Last_Move);

				if (type_of(Last_Move) == CASTLING)
				{
					if (current_position.side_to_move() == WHITE)
					{
						size_t symbol_position_w;
						size_t symbol_position;
						symbol_position_w = fen_saved.find(' ');
						assert(symbol_position_w != std::string::npos);

						symbol_position = fen_saved.find('K', symbol_position_w);
						if (symbol_position == std::string::npos)
						{
							symbol_position = fen_saved.find('Q', symbol_position_w);
						}

						if (symbol_position != std::string::npos)
						{
							if (fen_saved.at(symbol_position) == 'K')
							{
								fen_saved.erase(symbol_position, 1);
							}
						}

						symbol_position = fen_saved.find('Q', symbol_position_w);
						if (symbol_position != std::string::npos)
						{
							if (fen_saved.at(symbol_position) == 'Q')
							{
								fen_saved.erase(symbol_position, 1);
							}
						}
					}
					else
					{
						if (current_position.side_to_move() == BLACK)
						{
							size_t symbol_position_b;
							size_t symbol_position;
							symbol_position_b = fen_saved.find(' ');
							assert(symbol_position_b!=std::string::npos);

							symbol_position = fen_saved.find('k', symbol_position_b);
							if (symbol_position == std::string::npos)
							{
								symbol_position = fen_saved.find('q', symbol_position_b);
							}

							if (symbol_position != std::string::npos)
							{
								if (fen_saved.at(symbol_position) == 'k')
								{
									fen_saved.erase(symbol_position, 1);
								}
							}

							symbol_position = fen_saved.find('q', symbol_position_b);
							if (symbol_position != std::string::npos)
							{
								if (fen_saved.at(symbol_position) == 'q')
								{
									fen_saved.erase(symbol_position, 1);
								}
							}
						}
						else
						{
							assert(false);
						}
					}
				}
			}
			else
			{
				std::cout << "Game over" << std::endl;
			}

			if (learning_exit)
				return;
		}

		if (learning_exit)
			return;
	}
}

void MachineLearningControl::LearningExit()
{
	learning_exit = true;
}