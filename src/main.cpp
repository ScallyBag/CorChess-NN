/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2019 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include "bitboard.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "endgame.h"
#include "syzygy/tbprobe.h"
#include "polybook.h"

namespace PSQT {
  void init();
}

int main(int argc, char* argv[]) {
	
  {
    std::time_t result = std::time(NULL);
    std::cout << std::asctime(std::localtime(&result));
  }

  std::cout << engine_info() << std::endl;

  UCI::init(Options);
  //from Kelly begin
  if(Options["NN Persisted Self-Learning"])
  {
    UCI::initLearning();
  }
  //from Kelly end
  PSQT::init();
  Bitboards::init();
  Position::init();
  Bitbases::init();
  Endgames::init();
  Threads.set(Options["Threads"]);
  Threads.setFull(Options["Full depth threads"]);//Full threads patch
  polybook1.init(Options["BookFile1"]);
  polybook2.init(Options["BookFile2"]);
  polybook3.init(Options["BookFile3"]);
  polybook4.init(Options["BookFile4"]);  
  polybook5.init(Options["BookFile5"]);
  polybook6.init(Options["BookFile6"]);
  
  Search::clear(); // After threads are up

  UCI::loop(argc, argv);

  Threads.set(0);  return 0;
}
