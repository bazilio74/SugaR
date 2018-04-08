//	Author: Kozlov S.A. since 2018

#ifndef MachineLeaningControl_H_INCLUDED
#define MachineLeaningControl_H_INCLUDED

#include <string>
#include <list>

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
	bool simulating_in_progress;
	bool first_game_move_answer;
	bool infinite_learning_exit;
	bool infinite_start;

	std::string position_infinite_string;
	Search::LimitsType current_limit;
	bool ponder_mode_infinite;
	Position infinite_position;
	std::string fen_infinite_saved_main;
	StateListPtr *infinite_states;


	std::thread learning_thread;
	std::thread infinite_learning_thread;

	bool infinite_analysis_in_progress;
	int64_t current_infinite_depth;

	Position current_position;
	Move Last_Move;
	bool is_960;
	StateListPtr *states;
	bool current_position_set;

	std::string fen_saved_main;
	std::string position_saved_main;

	Search::LimitsType game_simulation_limits;
	bool game_simulation_ponderMode;

	Search::LimitsType final_game_limits;

	typedef std::list<MachineLearningDataAtom> MachineLearningDataList;
	typedef std::list<MachineLearningDataAtom>::iterator MachineLearningDataListIterator;
	MachineLearningDataList MachineLearningDataStore;

	void learning_thread_function();
	void infinite_learning_thread_function();
public:
	MachineLearningControl();
	~MachineLearningControl();

	static constexpr size_t games_to_simulate = 10;			//	select maximum 10 candidates moves on each move during simulation

	void SetFileName(std::string parameter_file_name);

	//	0 - success
	//	1 - fail
	//	2 - not required
	int LoadData();

	//	0 - success
	//	1 - fail
	//	2 - not required
	int SaveData();

	void ClearData();

	void StartLearning(Position &position_parameter, std::string position_string, std::istringstream& is, StateListPtr& parameter_states, const Search::LimitsType& limits, bool ponderMode);
	void StartInfiniteLearning(Position &position_parameter, std::string position_string, std::istringstream& is, StateListPtr& parameter_states, const Search::LimitsType& limits, bool ponderMode);

	void EndLearning();
	void LearningRoundFinished();
	bool IsLearningInProgress();
	bool IsSimulatingInProgress();
	void PrepareLearning(Position &position_parameter, std::istringstream& is, StateListPtr& parameter_states);
	void PrepareInfiniteLearning(Position &position_parameter, std::istringstream& is, StateListPtr& parameter_states);

	void LearningExit();

	void Answer(Move parameter_Move, bool parameter_960);

	int64_t GetCurrentInfiniteDepth();
	bool IsInfiniteAnalysisInProgress();

	bool learning_in_progress_object;
};

extern MachineLearningControl MachineLearningControlMain;

#endif