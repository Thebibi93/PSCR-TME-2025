#pragma once

#include "Queue.h"
#include "Job.h"
#include <vector>
#include <thread>

namespace pr {

class Pool {
	Queue<Job> queue;
	std::vector<std::thread> threads;
public:
	// create a pool with a queue of given size
	Pool(int qsize) : queue(qsize){}
	// start the pool with nbthread workers
	void start (int nbthread) {
		/*TODO*/
		for (int i = 0; i < nbthread; i++) {
			// syntaxe pour passer une methode membre au thread
			threads.emplace_back(&Pool::worker, this);
		}
	}

	// submit a job to be executed by the pool
	void submit (Job * job) {
		queue.push(job);
	}
	
	// initiate shutdown, wait for threads to finish
	void stop() {
		for(int i=0; i<queue.size(); ++i)
			queue.push({});
		for(auto &t : threads)
			t.join();
	}
	

private:
	// worker thread function
	void worker() {
		Job *j = queue.pop();
		if (!j){
			return;
		}
		j->run();
		delete j;
	}
};

}
