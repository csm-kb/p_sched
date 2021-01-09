#include "algorithms/fcfs/fcfs_algorithm.hpp"

#include <cassert>
#include <sstream>
#include <string>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the FCFS algorithm.
*/

FCFSScheduler::FCFSScheduler(int slice) {
	if (slice != -1) {
		throw("FCFS must have a timeslice of -1");
	}
}

std::shared_ptr<SchedulingDecision> FCFSScheduler::get_next_thread() {
	size_t rq_size = size();
	std::shared_ptr<SchedulingDecision> sd = std::make_shared<SchedulingDecision>();
	if (rq_size > 0) {
		std::shared_ptr<Thread> next_thr = ready_queue.front();
		ready_queue.pop();
		sd->thread = next_thr;
		std::ostringstream oss;
		oss << "Selected from " << rq_size << " threads. Will run to completion of burst.";
		sd->explanation = oss.str();
	} else {
		sd->thread = nullptr;
		std::ostringstream oss;
		oss << "No threads left in ready queue to execute.";
		sd->explanation = oss.str();
	}
	return sd;
}

void FCFSScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
	ready_queue.push(thread);
}

size_t FCFSScheduler::size() const {
	return ready_queue.size();
}
