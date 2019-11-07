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

#include <algorithm>
#include <cassert>
#include <ostream>
#include <iostream>
#include <sstream>

#include "misc.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"
#include "polybook.h"

using std::string;

UCI::OptionsMap Options; // Global object

namespace UCI {

/// 'On change' actions, triggered by an option's value change
void on_clear_hash(const Option&) { Search::clear(); }
void on_hash_size(const Option& o) { TT.resize(o);}
void on_large_pages(const Option& o) { TT.resize(o); }  // warning is ok, will be removed
void on_logger(const Option& o) { start_logger(o); }
void on_threads(const Option& o) { Threads.set(o); }
void on_full_threads(const Option& o) { Threads.setFull(o); }
void on_tb_path(const Option& o) { Tablebases::init(o); }
void on_HashFile(const Option& o) { TT.set_hash_file_name(o); }
void SaveHashtoFile(const Option&) { TT.save(); }
void LoadHashfromFile(const Option&) { TT.load(); }
void LoadEpdToHash(const Option&) { TT.load_epd_to_hash(); }
void on_book_file1(const Option& o) { polybook1.init(o); }
void on_book_file2(const Option& o) { polybook2.init(o); }
void on_book_file3(const Option& o) { polybook3.init(o); }
void on_book_file4(const Option& o) { polybook4.init(o); }
void on_book_file5(const Option& o) { polybook5.init(o); }
void on_book_file6(const Option& o) { polybook6.init(o); }
void on_best_book_move1(const Option& o) { polybook1.set_best_book_move(o); }
void on_best_book_move2(const Option& o) { polybook2.set_best_book_move(o); }
void on_best_book_move3(const Option& o) { polybook3.set_best_book_move(o); }
void on_best_book_move4(const Option& o) { polybook4.set_best_book_move(o); }
void on_best_book_move5(const Option& o) { polybook5.set_best_book_move(o); }
void on_best_book_move6(const Option& o) { polybook6.set_best_book_move(o); }
void on_book_depth1(const Option& o) { polybook1.set_book_depth(o); }
void on_book_depth2(const Option& o) { polybook2.set_book_depth(o); }
void on_book_depth3(const Option& o) { polybook3.set_book_depth(o); }
void on_book_depth4(const Option& o) { polybook4.set_book_depth(o); }
void on_book_depth5(const Option& o) { polybook5.set_book_depth(o); }
void on_book_depth6(const Option& o) { polybook6.set_book_depth(o); }

/// Our case insensitive less() function as required by UCI protocol
bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const {

  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
         [](char c1, char c2) { return tolower(c1) < tolower(c2); });
}


/// init() initializes the UCI options to their hard-coded default values

void init(OptionsMap& o) {

  // at most 2^32 clusters.
  constexpr int MaxHashMB = Is64Bit ? 131072 : 2048;

  o["Use Book1"]             << Option(false);
  o["BestBook1Move"]         << Option(false, on_best_book_move1);
  o["BookFile1"]             << Option("book1.bin", on_book_file1);
  o["BookDepth1"]            << Option(300, 1, 350, on_book_depth1);
  o["Use Book2"]             << Option(false);
  o["BestBook2Move"]         << Option(false, on_best_book_move2);
  o["BookFile2"]             << Option("book2.bin", on_book_file2);
  o["BookDepth2"]            << Option(300, 1, 350, on_book_depth2);
  o["Use Book3"]             << Option(false);
  o["BestBook3Move"]         << Option(false, on_best_book_move2);
  o["BookFile3"]             << Option("book3.bin", on_book_file2);
  o["BookDepth3"]            << Option(300, 1, 350, on_book_depth2);
  o["Use Book4"]             << Option(false);
  o["BestBook4Move"]         << Option(false, on_best_book_move2);
  o["BookFile4"]             << Option("book4.bin", on_book_file2);
  o["BookDepth4"]            << Option(300, 1, 350, on_book_depth2);
  o["Use Book5"]             << Option(false);
  o["BestBook5Move"]         << Option(false, on_best_book_move2);
  o["BookFile5"]             << Option("book5.bin", on_book_file2);
  o["BookDepth5"]            << Option(300, 1, 350, on_book_depth2);
  o["Use Book6"]             << Option(false);
  o["BestBook6Move"]         << Option(false, on_best_book_move2);
  o["BookFile6"]             << Option("book6.bin", on_book_file2);
  o["BookDepth6"]            << Option(300, 1, 350, on_book_depth2);
  o["Opening variety"]       << Option (0, 0, 40);
  o["Debug Log File"]        << Option("", on_logger);
  o["Contempt"]              << Option(0, -100, 100);
  o["Dynamic Contempt"]      << Option(false);
  o["Analysis Contempt"]     << Option("Off var Off var White var Black var Both", "Off");
  o["Threads"]               << Option(1, 1, 512, on_threads);
  o["Full depth threads"]    << Option(0, 0, 512, on_full_threads); //if this is used, must be after #Threads is set.
  o["Hash"]                  << Option(16, 1, MaxHashMB, on_hash_size);
  o["Large Pages"]           << Option(true, on_large_pages);
  o["Clear Hash"]            << Option(on_clear_hash);
  o["Show Fail High and Fail Low"] << Option(true);
  o["Ponder"]                << Option(false);
  o["MultiPV"]               << Option(1, 1, 500);
  o["Skill Level"]           << Option(20, 0, 20);
  o["Move Overhead"]         << Option(400, 0, 5000);
  o["Minimum Thinking Time"] << Option(20, 0, 5000);
  o["Slow Mover"]            << Option(84, 10, 1000);
  o["nodestime"]             << Option(0, 0, 10000);
  o["UCI_Chess960"]          << Option(false);
  o["NeverClearHash"]        << Option(false);
  o["HashFile"]              << Option("hash.hsh", on_HashFile);
  o["SaveHashtoFile"]        << Option(SaveHashtoFile);
  o["LoadHashfromFile"]      << Option(LoadHashfromFile);
  o["LoadEpdToHash"]         << Option(LoadEpdToHash);
  o["UCI_AnalyseMode"]       << Option(false);
  o["UCI_LimitStrength"]     << Option(false);
  o["UCI_Elo"]               << Option(1350, 1350, 2850);
  o["SyzygyPath"]            << Option("<empty>", on_tb_path);
  o["SyzygyProbeDepth"]      << Option(1, 1, 100);
  o["Syzygy50MoveRule"]      << Option(true);
  o["SyzygyProbeLimit"]      << Option(7, 0, 7);
  o["NullMove"]              << Option(true);
  o["NN Persisted Self-Learning"]  << Option(false);
}

void initLearning() {
  loadLearningFileIntoLearningTables(true);
  loadSlaveLearningFilesIntoLearningTables();
  writeLearningFile(HashTableType::experience);
  experienceHT.clear();
  globalLearningHT.clear();
  loadLearningFileIntoLearningTables(false);
}


/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {

  for (size_t idx = 0; idx < om.size(); ++idx)
      for (const auto& it : om)
          if (it.second.idx == idx)
          {
              const Option& o = it.second;
              os << "\noption name " << it.first << " type " << o.type;

              if (o.type == "string" || o.type == "check" || o.type == "combo")
                  os << " default " << o.defaultValue;

              if (o.type == "spin")
                  os << " default " << int(stof(o.defaultValue))
                     << " min "     << o.min
                     << " max "     << o.max;

              break;
          }

  return os;
}


/// Option class constructors and conversion operators

Option::Option(const char* v, OnChange f) : type("string"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(bool v, OnChange f) : type("check"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = (v ? "true" : "false"); }

Option::Option(OnChange f) : type("button"), min(0), max(0), on_change(f)
{}

Option::Option(double v, int minv, int maxv, OnChange f) : type("spin"), min(minv), max(maxv), on_change(f)
{ defaultValue = currentValue = std::to_string(v); }

Option::Option(const char* v, const char* cur, OnChange f) : type("combo"), min(0), max(0), on_change(f)
{ defaultValue = v; currentValue = cur; }

Option::operator double() const {
  assert(type == "check" || type == "spin");
  return (type == "spin" ? stof(currentValue) : currentValue == "true");
}

Option::operator std::string() const {
  assert(type == "string");
  return currentValue;
}

bool Option::operator==(const char* s) const {
  assert(type == "combo");
  return   !CaseInsensitiveLess()(currentValue, s)
        && !CaseInsensitiveLess()(s, currentValue);
}


/// operator<<() inits options and assigns idx in the correct printing order

void Option::operator<<(const Option& o) {

  static size_t insert_order = 0;

  *this = o;
  idx = insert_order++;
}


/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value
/// from the user by console window, so let's check the bounds anyway.

Option& Option::operator=(const string& v) {

  assert(!type.empty());

  if (   (type != "button" && v.empty())
      || (type == "check" && v != "true" && v != "false")
      || (type == "spin" && (stof(v) < min || stof(v) > max)))
      return *this;

  if (type == "combo")
  {
      OptionsMap comboMap; // To have case insensitive compare
      string token;
      std::istringstream ss(defaultValue);
      while (ss >> token)
          comboMap[token] << Option();
      if (!comboMap.count(v) || v == "var")
          return *this;
  }

  if (type != "button")
      currentValue = v;

  if (on_change)
      on_change(*this);

  return *this;
}

} // namespace UCI
