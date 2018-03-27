//	Author: Kozlov S.A. since 2018

#ifndef MachineLeaningControl_H_INCLUDED
#define MachineLeaningControl_H_INCLUDED

#include <string>
#include <list>

#include <condition_variable>
#include <mutex>
#include <thread>

#include "MachineLearningData.h"

#include "position.h"
#include "search.h"

class MachineLearningControl
{
protected:

	std::string file_name;
	bool learning_in_progress;
	bool learning_move_returned;
	bool learning_exit;
	bool learning_round_finished;

	std::thread learning_thread;

	Position current_position;
	Move Last_Move;
	bool is_960;
	StateListPtr *states;
	bool current_position_set;

	std::string fen_saved_main;
	Search::LimitsType game_simulation_limits;
	bool game_simulation_ponderMode;

	Search::LimitsType final_game_limits;

	typedef std::list<MachineLearningDataAtom> MachineLearningDataList;
	typedef std::list<MachineLearningDataAtom>::iterator MachineLearningDataListIterator;
	MachineLearningDataList MachineLearningDataStore;

	void learning_thread_function();
public:
	MachineLearningControl();
	~MachineLearningControl();

	static const size_t games_to_simulate = 2;

	void SetFileName(std::string parameter_file_name);

	//	0 - success
	//	1 - fail
	int LoadData();

	//	0 - success
	//	1 - fail
	//	2 - not required
	int SaveData();

	void ClearData();

	void StartLearning(Position &position_parameter, std::istringstream& is, StateListPtr& parameter_states, const Search::LimitsType& limits, bool ponderMode);
	void EndLearning();
	void LearningRoundFinished();
	bool IsLearningInProgress();
	void PrepareLearning(Position &position_parameter, std::istringstream& is, StateListPtr& parameter_states);

	void LearningExit();

	void Answer(Move parameter_Move, bool parameter_960);
};

extern MachineLearningControl MachineLearningControlMain;

#endif