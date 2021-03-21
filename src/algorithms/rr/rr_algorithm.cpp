#include "algorithms/rr/rr_algorithm.hpp"

#include <cassert>
#include <stdexcept>
#include <sstream>

/*
    The round-robin (RR) scheduling algorithm -- utilizes a preemptive queue-based mechanism for slicing and queueing threads.
*/

RRScheduler::RRScheduler(int slice) {    
    this->time_slice = slice;
}

std::shared_ptr<SchedulingDecision> RRScheduler::get_next_thread() {
	size_t rq_size = size();
	std::shared_ptr<SchedulingDecision> sd = std::make_shared<SchedulingDecision>();
	if (rq_size > 0) {
		std::shared_ptr<Thread> next_thr = ready_queue.front();
		ready_queue.pop();
		sd->thread = next_thr;
		std::ostringstream oss;
		oss << "Selected from " << rq_size << " threads. ";
		oss << "Will run for at most " << this->time_slice << " ticks.";
		sd->time_slice = this->time_slice;
		sd->explanation = oss.str();
	} else {
		sd->thread = nullptr;
		std::ostringstream oss;
		oss << "No threads left in ready queue to execute.";
		sd->explanation = oss.str();
	}
	return sd;
}

void RRScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
	ready_queue.push(thread);
}

size_t RRScheduler::size() const {
	return ready_queue.size();
}
