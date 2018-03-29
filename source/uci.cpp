/*
  SugaR, a UCI chess playing engine derived from Stockfish
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  SugaR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SugaR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "evaluate.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "timeman.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

#include "MachineLeaningControl.h"

void learning(Position& pos, std::string position_string, StateListPtr& states, const Search::LimitsType& limits, bool ponderMode, std::istringstream& is);
void position_make_move(Position& pos, std::string& parameter_string, std::istringstream& is, StateListPtr& states);
void learning_position_call(Position& pos, std::istringstream& is, StateListPtr& states);
void go_stub(Position& pos, std::string position_string, std::istringstream& is, StateListPtr& states);
void prepare_position(std::string parameter_fen, Position &position_parameter, StateListPtr& parameter_states);
void learning_infinite(Position& pos, std::string position_string, StateListPtr& states, const Search::LimitsType& limits, bool ponderMode, std::istringstream& is);

bool infinite_stop = false;

using namespace std;

extern vector<string> setup_bench(const Position&, istream&);

namespace {

	// FEN string of the initial position, normal chess
	const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


	// position() is called when engine receives the "position" UCI command.
	// The function sets up the position described in the given FEN string ("fen")
	// or the starting position ("startpos") and then makes the moves given in the
	// following move list ("moves").

	void position(Position& pos, istringstream& is, StateListPtr& states) {

		Move m;
		string token, fen;

		is >> token;

		if (token == "startpos")
		{
			fen = StartFEN;
			is >> token; // Consume "moves" token if any
		}
		else if (token == "fen")
			while (is >> token && token != "moves")
				fen += token + " ";
		else
			return;

		states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one
		pos.set(fen, Options["UCI_Chess960"], &states->back(), Threads.main());

		// Parse move list (if any)
		while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE)
		{
			states->emplace_back();
			pos.do_move(m, states->back());
		}
	}


	// setoption() is called when engine receives the "setoption" UCI command. The
	// function updates the UCI option ("name") to the given value ("value").

	void setoption(istringstream& is) {

		string token, name, value;

		is >> token; // Consume "name" token

		// Read option name (can contain spaces)
		while (is >> token && token != "value")
			name += (name.empty() ? "" : " ") + token;

		// Read option value (can contain spaces)
		while (is >> token)
			value += (value.empty() ? "" : " ") + token;

		if (Options.count(name))
			Options[name] = value;
		else
			sync_cout << "No such option: " << name << sync_endl;
	}


	// go() is called when engine receives the "go" UCI command. The function sets
	// the thinking time and other parameters from the input string, then starts
	// the search.

	void go(Position& pos, istringstream& is, StateListPtr& states) {

		Search::LimitsType limits;
		string token;
		bool ponderMode = false;

		limits.startTime = now(); // As early as possible!

		while (is >> token)
			if (token == "searchmoves")
				while (is >> token)
					limits.searchmoves.push_back(UCI::to_move(pos, token));

			else if (token == "wtime")     is >> limits.time[WHITE];
			else if (token == "btime")     is >> limits.time[BLACK];
			else if (token == "winc")      is >> limits.inc[WHITE];
			else if (token == "binc")      is >> limits.inc[BLACK];
			else if (token == "movestogo") is >> limits.movestogo;
			else if (token == "depth")     is >> limits.depth;
			else if (token == "nodes")     is >> limits.nodes;
			else if (token == "movetime")  is >> limits.movetime;
			else if (token == "mate")      is >> limits.mate;
			else if (token == "perft")     is >> limits.perft;
			else if (token == "infinite")  limits.infinite = 1;
			else if (token == "ponder")    ponderMode = true;

			Threads.start_thinking(pos, states, limits, ponderMode);
	}


	// bench() is called when engine receives the "bench" command. Firstly
	// a list of UCI commands is setup according to bench parameters, then
	// it is run one by one printing a summary at the end.

	void bench(Position& pos, istream& args, StateListPtr& states) {

		string token;
		uint64_t num, nodes = 0, cnt = 1;

		vector<string> list = setup_bench(pos, args);
		num = count_if(list.begin(), list.end(), [](string s) { return s.find("go ") == 0; });

		TimePoint elapsed = now();

		for (const auto& cmd : list)
		{
			istringstream is(cmd);
			is >> skipws >> token;

			if (token == "go")
			{
				cerr << "\nPosition: " << cnt++ << '/' << num << endl;
				go(pos, is, states);
				Threads.main()->wait_for_search_finished();
				nodes += Threads.nodes_searched();
			}
			else if (token == "setoption")  setoption(is);
			else if (token == "position")   position(pos, is, states);
			else if (token == "ucinewgame") Search::clear();
		}

		elapsed = now() - elapsed + 1; // Ensure positivity to avoid a 'divide by zero'

		dbg_print(); // Just before exiting

		cerr << "\n==========================="
			<< "\nTotal time (ms) : " << elapsed
			<< "\nNodes searched  : " << nodes
			<< "\nNodes/second    : " << 1000 * nodes / elapsed << endl;
	}

} // namespace


/// UCI::loop() waits for a command from stdin, parses it and calls the appropriate
/// function. Also intercepts EOF from stdin to ensure gracefully exiting if the
/// GUI dies unexpectedly. When called with some command line arguments, e.g. to
/// run 'bench', once the command is executed the function returns immediately.
/// In addition to the UCI ones, also some additional debug commands are supported.

void UCI::loop(int argc, char* argv[]) {

	Position pos;
	string token, cmd;
	StateListPtr states(new std::deque<StateInfo>(1));
	auto uiThread = std::make_shared<Thread>(0);

	pos.set(StartFEN, false, &states->back(), uiThread.get());

	std::string position_fen = pos.fen();

	Position learning_pos;
	StateListPtr learning_states(new std::deque<StateInfo>(1));

	learning_pos.set(StartFEN, false, &learning_states->back(), uiThread.get());

	std::string learning_position_fen = learning_pos.fen();

	std::string position_string_default(" startpos moves ");

	std::string position_string(position_string_default);
	std::string position_learning_string(position_string_default);

	for (int i = 1; i < argc; ++i)
		cmd += std::string(argv[i]) + " ";

	do {
		if (argc == 1 && !getline(cin, cmd)) // Block here waiting for input or EOF
			cmd = "quit";

		istringstream is(cmd);
		
		istringstream learning_is(cmd);

		token.clear(); // Avoid a stale if getline() returns empty or blank line
		is >> skipws >> token;

		learning_is >> skipws >> token;


		if (false)
		{
			istringstream local_is(cmd);
			std::ofstream output_stream;

			output_stream.open("GUI_commands.txt", std::ios_base::app | std::ios_base::out);

			if (output_stream.is_open())
			{
				std::string local_string;

				while (local_is >> local_string)
				{
					output_stream << local_string << " ";
				}

				output_stream << std::endl;
			}
		}


		// The GUI sends 'ponderhit' to tell us the user has played the expected move.
		// So 'ponderhit' will be sent if we were told to ponder on the same move the
		// user has played. We should continue searching but switch from pondering to
		// normal search. In case Threads.stopOnPonderhit is set we are waiting for
		// 'ponderhit' to stop the search, for instance if max search depth is reached.
		if (token == "quit"
			|| token == "stop"
			|| (token == "ponderhit" && Threads.stopOnPonderhit))
		{
			Threads.stop = true;

			infinite_stop = true;
		}

		else if (token == "ponderhit")
			Threads.ponder = false; // Switch to normal search

		else if (token == "uci")
			sync_cout << "id name " << engine_info(true)
			<< "\n" << Options
			<< "\nuciok" << sync_endl;

		else if (token == "setoption")  setoption(is);
		else if (token == "go")
		{
			Threads.stop = true;
			if (MachineLearningControlMain.IsLearningInProgress())
			{
				MachineLearningControlMain.EndLearning();
			}
			{
				Threads.main()->wait_for_search_finished();

				StateListPtr state_list_ptr;

				prepare_position(position_fen, pos, state_list_ptr);

				StateListPtr learning_state_list_ptr;

				prepare_position(learning_position_fen, learning_pos, learning_state_list_ptr);

				std::istringstream position_string_is(position_string);
				std::istringstream position_learning_string_is(position_learning_string);

				position(pos, position_string_is, states);
				position(learning_pos, position_learning_string_is, learning_states);
			}
			go_stub(pos, position_string, is, states);
		}
		else if (token == "position")
		{
			position(pos, is, states);
			position(learning_pos, learning_is, learning_states);

			std::string local_string = cmd;

			size_t position_command_start = local_string.find(token.c_str());
			local_string = local_string.substr(position_command_start + token.length() + 1);

			position_string = local_string;
			position_learning_string = local_string;

			position_fen = pos.fen();
			learning_position_fen = learning_pos.fen();
		}
		else if (token == "ucinewgame") Search::clear();
		else if (token == "isready")    sync_cout << "readyok" << sync_endl;

		// Additional custom non-UCI commands, mainly for debugging
		else if (token == "flip")
		{
			StateListPtr state_list_ptr;

			prepare_position(position_fen, pos, state_list_ptr);

			StateListPtr learning_state_list_ptr;

			prepare_position(learning_position_fen, learning_pos, learning_state_list_ptr);

			std::istringstream position_string_is(position_string);
			std::istringstream position_learning_string_is(position_learning_string);

			position(pos, position_string_is, states);
			position(learning_pos, position_learning_string_is, learning_states);

			pos.flip();
		}
		else if (token == "bench")
		{
			StateListPtr state_list_ptr;

			prepare_position(position_fen, pos, state_list_ptr);

			StateListPtr learning_state_list_ptr;

			prepare_position(learning_position_fen, learning_pos, learning_state_list_ptr);

			std::istringstream position_string_is(position_string);
			std::istringstream position_learning_string_is(position_learning_string);

			position(pos, position_string_is, states);
			position(learning_pos, position_learning_string_is, learning_states);

			bench(pos, is, states);
		}
		else if (token == "d")
		{
			StateListPtr state_list_ptr;

			prepare_position(position_fen, pos, state_list_ptr);

			StateListPtr learning_state_list_ptr;

			prepare_position(learning_position_fen, learning_pos, learning_state_list_ptr);

			std::istringstream position_string_is(position_string);
			std::istringstream position_learning_string_is(position_learning_string);

			position(pos, position_string_is, states);
			position(learning_pos, position_learning_string_is, learning_states);

			sync_cout << pos << sync_endl;
		}
		else if (token == "eval")  sync_cout << Eval::trace(pos) << sync_endl;
		else if (token == "move")
		{
			StateListPtr state_list_ptr;

			prepare_position(position_fen, pos, state_list_ptr);

			StateListPtr learning_state_list_ptr;

			prepare_position(learning_position_fen, learning_pos, learning_state_list_ptr);

			position_make_move(pos, position_string, is, states);
			position_make_move(learning_pos, position_learning_string, learning_is, learning_states);

			std::istringstream position_string_is(position_string);
			std::istringstream position_learning_string_is(position_learning_string);

			position(pos, position_string_is, states);
			position(learning_pos, position_learning_string_is, learning_states);

			position_fen = pos.fen();
			learning_position_fen = learning_pos.fen();
		}
		else if (token == "learning")
		{
			Threads.stop = true;

			Threads.main()->wait_for_search_finished();

			StateListPtr state_list_ptr;

			prepare_position(position_fen, pos, state_list_ptr);

			StateListPtr learning_state_list_ptr;

			prepare_position(learning_position_fen, learning_pos, learning_state_list_ptr);

			std::istringstream position_string_is(position_string);
			std::istringstream position_learning_string_is(position_learning_string);

			position(pos, position_string_is, states);
			position(learning_pos, position_learning_string_is, learning_states);

			Search::LimitsType search_limits = Search::Limits;

			MachineLearningControlMain.PrepareLearning(learning_pos, learning_is, learning_states);
			learning(learning_pos, position_learning_string, learning_states, search_limits, false, learning_is);
		}
		else
			sync_cout << "Unknown command: " << cmd << sync_endl;

		if (token == "quit")
		{
			Threads.stop = true;

			MachineLearningControlMain.LearningExit();
			
			Threads.main()->wait_for_search_finished();
		}

		if (token == "stop")
		{
			Threads.stop = true;

			MachineLearningControlMain.EndLearning();

			Threads.main()->wait_for_search_finished();
		}

	} while (token != "quit" && argc == 1); // Command line args are one-shot
}


/// UCI::value() converts a Value to a string suitable for use with the UCI
/// protocol specification:
///
/// cp <x>    The score from the engine's point of view in centipawns.
/// mate <y>  Mate in y moves, not plies. If the engine is getting mated
///           use negative values for y.

string UCI::value(Value v) {

	assert(-VALUE_INFINITE < v && v < VALUE_INFINITE);

	stringstream ss;

	if (abs(v) < VALUE_MATE - MAX_PLY)
		ss << "cp " << v * 100 / PawnValueEg;
	else
		ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;

	return ss.str();
}


/// UCI::square() converts a Square to a string in algebraic notation (g1, a7, etc.)

std::string UCI::square(Square s) {
	return std::string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
}


/// UCI::move() converts a Move to a string in coordinate notation (g1f3, a7a8q).
/// The only special case is castling, where we print in the e1g1 notation in
/// normal chess mode, and in e1h1 notation in chess960 mode. Internally all
/// castling moves are always encoded as 'king captures rook'.

string UCI::move(Move m, bool chess960) {

	Square from = from_sq(m);
	Square to = to_sq(m);

	if (m == MOVE_NONE)
		return "(none)";

	if (m == MOVE_NULL)
		return "0000";

	if (type_of(m) == CASTLING && !chess960)
		to = make_square(to > from ? FILE_G : FILE_C, rank_of(from));

	string move = UCI::square(from) + UCI::square(to);

	if (type_of(m) == PROMOTION)
		move += " pnbrqk"[promotion_type(m)];

	return move;
}


/// UCI::to_move() converts a string representing a move in coordinate notation
/// (g1f3, a7a8q) to the corresponding legal Move, if any.

Move UCI::to_move(const Position& pos, string& str) {

	if (str.length() == 5) // Junior could send promotion piece in uppercase
		str[4] = char(tolower(str[4]));

	for (const auto& m : MoveList<LEGAL>(pos))
		if (str == UCI::move(m, pos.is_chess960()))
			return m;

	return MOVE_NONE;
}

void learning_infinite(Position& pos, std::string position_string, StateListPtr& states, const Search::LimitsType& limits, bool ponderMode, std::istringstream& is)
{
	string token;

	while (is >> token)
	{
		if (token == "start")
		{
			MachineLearningControlMain.StartInfiniteLearning(pos, position_string, is, states, limits, ponderMode);
		}
	}
}


void learning(Position& pos, std::string position_string, StateListPtr& states, const Search::LimitsType& limits, bool ponderMode, std::istringstream& is)
{
	string token;

	while (is >> token)
	{
		if (token == "start")
		{
			MachineLearningControlMain.StartLearning(pos, position_string, is, states, limits, ponderMode);
		}
		else if (token == "end")
		{
			MachineLearningControlMain.EndLearning();
		}
		else if (token == "save")
		{
			MachineLearningControlMain.SaveData();
		}
		else if (token == "load")
		{
			MachineLearningControlMain.LoadData();
		}
		else if (token == "clear")
		{
			MachineLearningControlMain.ClearData();
		}
	}
}

void learning_go_call(Position& pos, std::istringstream& is, StateListPtr& states)
{
	go(pos, is, states);
}

void learning_position_call(Position& pos, std::istringstream& is, StateListPtr& states)
{
	position(pos, is, states);
}

void position_make_move(Position& current_position, std::string &parameter_string, std::istringstream& is, StateListPtr& states)
{
	Color us = current_position.side_to_move();

	std::string token;

	is >> token;

	parameter_string += " " + token;
}

void go_stub(Position& pos, std::string position_string, istringstream& is, StateListPtr& states) {

	Search::LimitsType limits;
	string token;
	bool ponderMode = false;

	limits.startTime = now(); // As early as possible!

	while (is >> token)
		if (token == "searchmoves")
			while (is >> token)
				limits.searchmoves.push_back(UCI::to_move(pos, token));

		else if (token == "wtime")     is >> limits.time[WHITE];
		else if (token == "btime")     is >> limits.time[BLACK];
		else if (token == "winc")      is >> limits.inc[WHITE];
		else if (token == "binc")      is >> limits.inc[BLACK];
		else if (token == "movestogo") is >> limits.movestogo;
		else if (token == "depth")     is >> limits.depth;
		else if (token == "nodes")     is >> limits.nodes;
		else if (token == "movetime")  is >> limits.movetime;
		else if (token == "mate")      is >> limits.mate;
		else if (token == "perft")     is >> limits.perft;
		else if (token == "infinite")  limits.infinite = 1;
		else if (token == "ponder")    ponderMode = true;

		if (token == "infinite")
		{
			if (true)
			{
				Threads.start_thinking(pos, states, limits, ponderMode);
			}
			else
			{
				//	Own realization of infinite analisys using machine learning is not ready yet
				Search::LimitsType start_limit;
				start_limit.time[WHITE] = 1000 * 10;
				start_limit.time[BLACK] = 1000 * 10;

				std::string input_stream_data("start");
				std::istringstream input_stream(input_stream_data);

				learning_infinite(pos, position_string, states, start_limit, ponderMode, input_stream);
			}
		}
		else
		{
			std::string input_stream_data("start");
			std::istringstream input_stream(input_stream_data);

			learning(pos, position_string, states, limits, ponderMode, input_stream);
		}
}

void go(Position& pos, StateListPtr& states, Search::LimitsType limits, bool ponderMode)
{
	Threads.start_thinking(pos, states, limits, ponderMode);
}

void prepare_position(std::string parameter_fen, Position &position_parameter, StateListPtr& parameter_states)
{
	parameter_states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one

	position_parameter.init();
	position_parameter.set(parameter_fen, Options["UCI_Chess960"], &parameter_states->front(), Threads.main());
}