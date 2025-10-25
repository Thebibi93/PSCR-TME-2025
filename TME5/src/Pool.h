#pragma once

#include "Job.h"
#include "Queue.h"
#include <thread>
#include <vector>

namespace pr {

class Pool {
  Queue<Job> queue;
  std::vector<std::thread> threads;

public:
  // create a pool with a queue of given size
  Pool(int qsize) : queue(qsize) {}
  // start the pool with nbthread workers
  void start(int nbthread) {
    /*TODO*/
    for (int i = 0; i < nbthread; i++) {
      // syntaxe pour passer une methode membre au thread
      threads.emplace_back(&Pool::worker, this);
    }
  }

  // submit a job to be executed by the pool
  void submit(Job *job) { queue.push(job); }

  // initiate shutdown, wait for threads to finish
  void stop() {
    for (size_t i = 0; i < threads.size(); ++i)
      queue.push(nullptr);
    for (auto &t : threads)
      t.join();
  }

private:
  // worker thread function
  void worker() {
    while (true) {
      Job *j = queue.pop();
      if (!j)
        break;
      j->run();
      delete j;
    }
  }
};

} // namespace pr
