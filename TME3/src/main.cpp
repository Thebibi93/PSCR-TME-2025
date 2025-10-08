#include "FileUtils.h"
#include "HashMap.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <ios>
#include <iostream>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
  using namespace std::chrono;

  // Allow filename as optional first argument, default to
  // project-root/WarAndPeace.txt Optional second argument is mode (e.g.
  // "freqstd" or "freq"). Optional third argument is num_threads (default 4).
  string filename = "../WarAndPeace.txt";
  string mode = "mt_hashes";
  int num_threads = 8;
  if (argc > 1)
    filename = argv[1];
  if (argc > 2)
    mode = argv[2];

  // Check if file is readable
  ifstream check(filename, std::ios::binary);
  if (!check.is_open()) {
    cerr << "Could not open '" << filename
         << "'. Please provide a readable text file as the first argument."
         << endl;
    cerr << "Usage: " << (argc > 0 ? argv[0] : "TME3")
         << " [path/to/textfile] [mode] [num threads]" << endl;
    return 2;
  }
  check.seekg(0, std::ios::end);
  std::streamoff file_size = check.tellg();
  check.close();

  cout << "Preparing to parse " << filename << " (mode=" << mode
       << " N=" << num_threads << "), containing " << file_size << " bytes"
       << endl;

  auto start = steady_clock::now();

  std::vector<std::pair<std::string, int>> pairs;

  if (mode == "freqstd") {
    ifstream input(filename, std::ios::binary);
    size_t total_words = 0;
    size_t unique_words = 0;
    std::unordered_map<std::string, int> um;
    std::string word;
    while (input >> word) {
      word = pr::cleanWord(word);
      if (!word.empty()) {
        total_words++;
        ++um[word];
      }
    }
    unique_words = um.size();
    pairs.reserve(unique_words);
    for (const auto &p : um)
      pairs.emplace_back(p);
    pr::printResults(total_words, unique_words, pairs, mode + ".freq");

  } else if (mode == "freqstdf") {
    size_t total_words = 0;
    size_t unique_words = 0;
    std::unordered_map<std::string, int> um;
    pr::processRange(filename, 0, file_size, [&](const std::string &word) {
      total_words++;
      um[word]++;
    });
    unique_words = um.size();
    pairs.reserve(unique_words);
    for (const auto &p : um)
      pairs.emplace_back(p);
    pr::printResults(total_words, unique_words, pairs, mode + ".freq");

  } else if (mode == "freq") {
    size_t total_words = 0;
    size_t unique_words = 0;
    HashMap<std::string, int> hm;
    pr::processRange(filename, 0, file_size, [&](const std::string &word) {
      total_words++;
      hm.incrementFrequency(word);
    });
    pairs = hm.toKeyValuePairs();
    unique_words = pairs.size();
    pr::printResults(total_words, unique_words, pairs, mode + ".freq");

  } else if (mode == "partition") {
    size_t total_words = 0;
    size_t unique_words = 0;
    std::unordered_map<std::string, int> um;
    // Diviser le fichier en parties
    std::vector<std::streamoff> file_part =
        pr::partition(filename, file_size, num_threads);
    for (std::size_t i = 0; i < file_part.size() - 1; ++i) {
      pr::processRange(filename, file_part[i], file_part[i + 1],
                       [&](const std::string &word) {
                         total_words++;
                         um[word]++;
                       });
    }
    unique_words = um.size();
    pairs.reserve(unique_words);
    for (const auto &p : um)
      pairs.emplace_back(p);
    pr::printResults(total_words, unique_words, pairs, mode + ".freq");

  } else if (mode == "mt_naive") {
    size_t total_words = 0;
    size_t unique_words = 0;
    std::unordered_map<std::string, int> um;
    // Diviser le fichier en parties
    std::vector<std::streamoff> file_part =
        pr::partition(filename, file_size, num_threads);

    // Création des threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < file_part.size() - 1; ++i) {
      // Il faut bien passer i par copie car il va quitter le scope sinon...
      threads.emplace_back(std::thread([&, i]() {
        pr::processRange(filename, file_part[i], file_part[i + 1],
                         [&](const std::string &word) {
                           total_words++;
                           um[word]++;
                         });
      }));
    }
    for (std::thread &thread : threads) {
      thread.join(); // Attendre la fin du thread...
    }
    unique_words = um.size();
    pairs.reserve(unique_words);
    for (const auto &p : um)
      pairs.emplace_back(p);
    pr::printResults(total_words, unique_words, pairs, mode + ".freq");

  } else if (mode == "mt_mutex") {
    atomic<int> total_words = 0;
    size_t unique_words = 0;
    std::unordered_map<std::string, int> um;
    // Diviser le fichier en parties
    std::vector<std::streamoff> file_part =
        pr::partition(filename, file_size, num_threads);

    // Création des threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    // Création du mutex
    std::mutex m;
    for (std::size_t i = 0; i < file_part.size() - 1; ++i) {
      // Il faut bien passer i par copie car il va quitter le scope sinon...
      threads.emplace_back(std::thread([&, i]() {
        pr::processRange(filename, file_part[i], file_part[i + 1],
                         [&](const std::string &word) {
                           total_words++;
                           m.lock();
                           um[word]++;
                           m.unlock();
                         });
      }));
    }
    for (std::thread &thread : threads) {
      thread.join(); // Attendre la fin du thread...
    }
    unique_words = um.size();
    pairs.reserve(unique_words);
    for (const auto &p : um)
      pairs.emplace_back(p);
    pr::printResults(total_words, unique_words, pairs, mode + ".freq");

  } else if (mode == "mt_hashes") {
    std::atomic<size_t> total_words = 0;
    size_t unique_words = 0;
    // Créer plusieurs
    vector<unordered_map<string, int>> hashes(num_threads);
    // Diviser le fichier en parties
    std::vector<std::streamoff> file_part =
        pr::partition(filename, file_size, num_threads);

    // Création des threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < file_part.size() - 1; ++i) {
      // Il faut bien passer i par copie car il va quitter le scope sinon...
      threads.emplace_back(std::thread([&, i]() {
        pr::processRange(filename, file_part[i], file_part[i + 1],
                         [&](const std::string &word) {
                           total_words++;
                           hashes[i][word]++;
                         });
      }));
    }
    for (std::thread &thread : threads) {
      thread.join(); // Attendre la fin du thread...
    }
    unordered_map<string, int> um;
    for (auto &h : hashes) {
      for (auto &p : h) {
        um[p.first] += p.second;
      }
    }
    unique_words = um.size();
    pairs.reserve(unique_words);
    for (const auto &p : um)
      pairs.emplace_back(p);
    pr::printResults(total_words, unique_words, pairs, mode + ".freq");
  } else {
    cerr << "Unknown mode '" << mode
         << "'. Supported modes: freqstd, freq, freqstdf" << endl;
    return 1;
  }

  // print a single total runtime for successful runs
  auto end = steady_clock::now();
  cout << "Total runtime (wall clock) : "
       << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

  return 0;
}
