#ifndef THREAD_HPP
#define THREAD_HPP

#include <iostream>
#include <memory>
#include <vector>
#include <queue>
#include <set>
#include <map>

#include "types/burst/burst.hpp"
#include "types/enums.hpp"

/**
Thread:
A class representing a single thread. It contains all the information
a thread needs, as well as the functionality that it needs. Anything
that you may need to do on a thread should be done here. If there is
something you feel is missing, please add it!
*/
class Thread {
public:

	//==================================================
	//  Member variables
	//==================================================

	/**
	thread_id:
	    The thread's ID. These are only unique within the context of a process.
	*/
	int thread_id = -1;

	/**
	process_id:
	    The ID for the thread's parent process. Each thread has a parent!
	*/
	int process_id = -1;

	/**
	arrival_time:
	    When the thread arrived into the simulation. Taken from the input file.
	*/
	int arrival_time = -1;

	/**
	start_time:
	    The time the CPU was first able to execute this thread. Should be set when
	    the thread transitions from READY to RUNNING.
	*/
	int start_time = -1;

	/**
	end_time:
	    The time that all of this thread's CPU and IO bursts were completed.
	    Set when the thread transitions from RUNNING to EXIT.
	*/
	int end_time = -1;

	/**
	service_time:
	    The service time for the thread. The total time it was spent on the CPU.
	*/
	int service_time = 0;

	/**
	io_time:
	    The IO time for the thread. The total time it spent in IO.
	*/
	int io_time = 0;

	/**
	state_change_time:
	    The time of the last state change.
	*/
	int state_change_time = -1;

	/**
	priority:
	    The priority of the parent process (and thus the priority of the thread).
	*/
	ProcessPriority priority;

	/**
	current_state:
	    The current state of the thread. All threads start as NEW.
	*/
	ThreadState current_state = NEW;

	/**
	previous_state:
	    The previous state of the thread.
	*/
	ThreadState previous_state;

	/**
	 Helper map for converting a ThreadState to a string.
	 */
	static const std::map<ThreadState, std::string> threadstate_string;

	/**
	 Helper map for validating ThreadState transitions.
	 */
	static const std::map<ThreadState, std::set<ThreadState>> valid_transitions;

	/**
	bursts:
	    A queue of bursts. Should contain the CPU and IO bursts in the correct order as
	    specified in the simulation file.
	*/
	std::queue<std::shared_ptr<Burst>> bursts;

	//==================================================
	//  Member functions
	//==================================================

	/**
	Thread(arrival, thread_id, process_id, priority):
	    A constuctor for a thread object. We give it an arrival time, thread ID,
	    process ID, and priority, and a thread with those variables is constructed.
	*/
	Thread(int arrival, int thread_id, int process_id, ProcessPriority priority):
		arrival_time(arrival), thread_id(thread_id), process_id(process_id), priority(priority) {}

	/*
	set_*:
	    Sets the thread to the appropriate state at the appropriate time. The time is
	    used to set the state_change_time value. We should perform some sort of checking
	    to make sure that the transition is valid, e.g., is NEW->BLOCKED a valid transition?
	    Throwing an exception for an invalid transition may be a good idea.
	*/
	void set_ready(int time);

	void set_running(int time);

	void set_blocked(int time);

	void set_finished(int time);

	void set_state(ThreadState state, int time);

	/**
	 Returns whether or not the specified transition is valid.
	 */
	bool is_valid_transition(ThreadState from, ThreadState to);

	/**
	response_time():
	    Calculate the response time for this particular thread.
	*/
	int response_time() const;

	/**
	turnaround_time():
	    Calculate the turnaround time for this particular thread.
	*/
	int turnaround_time() const;

	/**
	get_next_burst(type):
	    Get the next burst. We should ensure that the next burst in the queue
	    is of the appropriate type.
	*/
	std::shared_ptr<Burst> get_next_burst(BurstType type);


	/**
	pop_next_burst(type):
	    Pop the next burst. We should ensure that the next burst in the queue
	    is of the appropriate type.
	*/
	std::shared_ptr<Burst> pop_next_burst(BurstType type);

};

#endif
