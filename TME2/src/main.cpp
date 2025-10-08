#include "HashMap.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// helper to clean a token (keep original comments near the logic)
static std::string cleanWord(const std::string &raw) {
  // une regex qui reconnait les caractères anormaux (négation des lettres)
  static const std::regex re(R"([^a-zA-Z])");
  // élimine la ponctuation et les caractères spéciaux
  std::string w = std::regex_replace(raw, re, "");
  // passe en lowercase
  std::transform(w.begin(), w.end(), w.begin(), ::tolower);
  return w;
}

int main(int argc, char **argv) {
  using namespace std;
  using namespace std::chrono;

  // Allow filename as optional first argument, default to
  // project-root/WarAndPeace.txt Optional second argument is mode (e.g. "count"
  // or "unique").
  string filename = "../WarAndPeace.txt";
  string mode = "freqstd";
  if (argc > 1)
    filename = argv[1];
  if (argc > 2)
    mode = argv[2];

  ifstream input(filename);
  if (!input.is_open()) {
    cerr << "Could not open '" << filename
         << "'. Please provide a readable text file as the first argument."
         << endl;
    cerr << "Usage: " << (argc > 0 ? argv[0] : "TME2") << " [path/to/textfile]"
         << endl;
    return 2;
  }
  // cout << "Parsing " << filename << " (mode=" << mode << ")" << endl;

  auto start = steady_clock::now();

  // prochain mot lu
  string word;

  if (mode == "count") {
    size_t nombre_lu = 0;

    // default counting mode: count total words
    while (input >> word) {
      // élimine la ponctuation et les caractères spéciaux
      word = cleanWord(word);

      // word est maintenant "tout propre"
      // if (nombre_lu % 100 == 0)
      // on affiche un mot "propre" sur 100
      // cout << nombre_lu << ": "<< word << endl;
      nombre_lu++;
    }
    input.close();
    cout << "Finished parsing." << endl;
    cout << "Found a total of " << nombre_lu << " words." << endl;

  } else if (mode == "unique") {
    // skeleton for unique mode
    // before the loop: declare a vector "seen"
    // TODO

    vector<std::string> seen;

    while (input >> word) {
      // élimine la ponctuation et les caractères spéciaux
      word = cleanWord(word);

      // add to seen if it is new
      // TODO

      if (std::find(seen.begin(), seen.end(), word) == seen.end()) {
        seen.push_back(word);
      }
    }
    input.close();
    // TODO
    cout << "Finished parsing." << endl;
    cout << "Found " << seen.size() << " unique words." << endl;

  } else if (mode == "freq") {
    vector<pair<std::string, int>> seen;

    while (input >> word) {
      // élimine la ponctuation et les caractères spéciaux
      word = cleanWord(word);
      bool found = false;

      for (pair<std::string, int> &p : seen) {
        if (p.first == word) {
          p.second = p.second + 1;
          found = true;
          break;
        }
      }
      if (found) {
        continue;
      }
      seen.push_back(std::make_pair(word, 1));
    }
    input.close();
    cout << "Finished parsing." << endl;
    // Chercher war peace et toto
    /* for (pair<std::string, int> p : seen) {
        if (p.first == "war" || p.first == "peace" || p.first == "toto") {
                        cout << "Nombre occurence de " << p.first << " : " <<
      p.second << endl;
        }
      }*/
    std::sort(
        seen.begin(), seen.end(),
        [](const pair<std::string, int> &a, const pair<std::string, int> &b) {
          return a.second > b.second;
        });
    for (std::size_t i = 0; i < 10 && i < seen.size(); i++) {
      cout << "Nombre occurence de " << seen[i].first << " : " << seen[i].second
           << endl;
    }
  } else if (mode == "freqhash") {
    HashMap<std::string, int> count(1024);
    while (input >> word) {
      word = cleanWord(word);
      int *value = count.get(word);
      if (value) {
        count.put(word, *value + 1);
      } else {
        count.put(word, 1);
      }
    }
    input.close();
    cout << "Finished parsing." << endl;

    // Récupérer tous les couples (mot, nb) et trier pour afficher les plus
    // fréquents
    auto pairs = count.toKeyValuePairs();
    std::sort(pairs.begin(), pairs.end(),
              [](const std::pair<std::string, int> &a,
                 const std::pair<std::string, int> &b) {
                return a.second > b.second;
              });
    for (std::size_t i = 0; i < 10 && i < pairs.size(); ++i) {
      cout << "Nombre occurence de " << pairs[i].first << " : "
           << pairs[i].second << endl;
    }
  } else if (mode == "freqstd") {
    std::unordered_map<std::string, int> map;
    while (input >> word) {
      word = cleanWord(word);
      map[word]++;
    }
    input.close();
    cout << "Finished parsing." << endl;

    std::vector<std::pair<std::string, int>> pairs(map.begin(), map.end());
    std::sort(pairs.begin(), pairs.end(),
        [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second > b.second;
        });
    for (std::size_t i = 0; i < 10 && i < pairs.size(); ++i) {
        cout << "Nombre occurence de " << pairs[i].first << " : " << pairs[i].second << endl;
    }
  }

  else {
    // unknown mode: print usage and exit
    cerr << "Unknown mode '" << mode << "'. Supported modes: count, unique"
         << endl;
    input.close();
    return 1;
  }

  // print a single total runtime for successful runs
  auto end = steady_clock::now();
  cout << "Total runtime (wall clock) : "
       << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

  return 0;
}
