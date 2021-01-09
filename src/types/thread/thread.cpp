#include <cassert>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "types/thread/thread.hpp"

const std::map<ThreadState, std::string> Thread::threadstate_string = {
	{ ThreadState::NEW, "NEW" },
	{ ThreadState::READY, "READY" },
	{ ThreadState::RUNNING, "RUNNING" },
	{ ThreadState::BLOCKED, "BLOCKED" },
	{ ThreadState::EXIT, "EXIT" }
};

const std::map<ThreadState, std::set<ThreadState>> Thread::valid_transitions = {
	{ ThreadState::NEW,     std::set<ThreadState>{ThreadState::READY} },
	{ ThreadState::READY,   std::set<ThreadState>{ThreadState::RUNNING} },
	{ ThreadState::RUNNING, std::set<ThreadState>{ThreadState::READY, ThreadState::BLOCKED, ThreadState::EXIT} },
	{ ThreadState::BLOCKED, std::set<ThreadState>{ThreadState::READY} },
	{ ThreadState::EXIT,    std::set<ThreadState>{} }
};

void Thread::set_ready(int time) {
	if (!is_valid_transition(current_state, ThreadState::READY)) {
		std::ostringstream oss;
		oss << "Invalid transition from " << Thread::threadstate_string.at(current_state) << " to READY";
		throw(oss.str());
	}
	previous_state = current_state;
	current_state = ThreadState::READY;
	state_change_time = time;
}

void Thread::set_running(int time) {
	if (!is_valid_transition(current_state, ThreadState::RUNNING)) {
		std::ostringstream oss;
		oss << "Invalid transition from " << Thread::threadstate_string.at(current_state) << " to RUNNING";
		throw(oss.str());
	}
	previous_state = current_state;
	current_state = ThreadState::RUNNING;
	state_change_time = time;
	if (start_time == -1) {
		start_time = time;
	}
}

void Thread::set_blocked(int time) {
	if (!is_valid_transition(current_state, ThreadState::BLOCKED)) {
		std::ostringstream oss;
		oss << "Invalid transition from " << Thread::threadstate_string.at(current_state) << " to BLOCKED";
		throw(oss.str());
	}
	previous_state = current_state;
	current_state = ThreadState::BLOCKED;
	state_change_time = time;
}

void Thread::set_finished(int time) {
	if (!is_valid_transition(current_state, ThreadState::EXIT)) {
		std::ostringstream oss;
		oss << "Invalid transition from " << Thread::threadstate_string.at(current_state) << " to EXIT";
		throw(oss.str());
	}
	previous_state = current_state;
	current_state = ThreadState::EXIT;
	state_change_time = time;
	end_time = time;
}

int Thread::response_time() const {
	return start_time - arrival_time;
}

int Thread::turnaround_time() const {
	return end_time - arrival_time;
}

void Thread::set_state(ThreadState state, int time) {
	if (!is_valid_transition(current_state, state)) {
		std::ostringstream oss;
		oss << "Invalid transition from " << Thread::threadstate_string.at(current_state) << " to EXIT";
		throw(oss.str());
	}
	previous_state = current_state;
	current_state = state;
	state_change_time = time;
}

bool Thread::is_valid_transition(ThreadState from, ThreadState to) {
	return Thread::valid_transitions.at(from).count(to);
}

std::shared_ptr<Burst> Thread::get_next_burst(BurstType type) {
	if (bursts.size() == 0) {
		return nullptr;
	}
	std::shared_ptr<Burst> burst = bursts.front();
	if (burst->burst_type != type) {
		throw("Invalid next burst type for thread");
	}
	return burst;
}

std::shared_ptr<Burst> Thread::pop_next_burst(BurstType type) {
	std::shared_ptr<Burst> burst = get_next_burst(type);
	if (burst != nullptr) {
		bursts.pop();
	}
	return burst;
}