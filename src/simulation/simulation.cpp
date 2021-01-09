#include <fstream>
#include <iostream>

#include "algorithms/fcfs/fcfs_algorithm.hpp"
#include "algorithms/rr/rr_algorithm.hpp"

#include "simulation/simulation.hpp"
#include "types/enums.hpp"

#include "utilities/flags/flags.hpp"

Simulation::Simulation(FlagOptions flags)
{
	// Hello!
	if (flags.scheduler == "FCFS")
	{
		// Create a FCFS scheduling algorithm
		this->scheduler = std::make_shared<FCFSScheduler>();
	}
	else if (flags.scheduler == "RR")
	{
		// Create a RR scheduling algorithm
		if (flags.time_slice > -1) {
			this->scheduler = std::make_shared<RRScheduler>(flags.time_slice);
		} else {
			this->scheduler = std::make_shared<RRScheduler>();
		}
	}
	this->flags = flags;
	this->logger = Logger(flags.verbose, flags.per_thread, flags.metrics);
	for (auto i = 0; i < 4; i++)
		this->all_threads[i] = std::vector<std::shared_ptr<Thread>>();
}

void Simulation::run()
{
	this->read_file(this->flags.filename);

	bool first_event = true;
	while (!this->events.empty())
	{
		auto event = this->events.top();
		this->events.pop();

		// Invoke the appropriate method in the simulation for the given event type.

		switch (event->type)
		{
		case THREAD_ARRIVED:
			if (first_event) {
				first_event = false;
				this->system_stats.total_idle_time = event->time;
			}
			this->handle_thread_arrived(event);
			break;

		case THREAD_DISPATCH_COMPLETED:
		case PROCESS_DISPATCH_COMPLETED:
			this->handle_dispatch_completed(event);
			break;

		case CPU_BURST_COMPLETED:
			this->handle_cpu_burst_completed(event);
			break;

		case IO_BURST_COMPLETED:
			this->handle_io_burst_completed(event);
			break;
		case THREAD_COMPLETED:
			this->handle_thread_completed(event);
			break;

		case THREAD_PREEMPTED:
			this->handle_thread_preempted(event);
			break;

		case DISPATCHER_INVOKED:
			this->handle_dispatcher_invoked(event);
			break;
		}

		// If this event triggered a state change, print it out.
		if (event->thread && event->thread->current_state != event->thread->previous_state)
		{
			this->logger.print_state_transition(event, event->thread->previous_state, event->thread->current_state);
		}
		else if (event->type == EventType::DISPATCHER_INVOKED)
		{
			this->logger.print_verbose(event, event->thread, event->scheduling_decision->explanation);
		}
		this->system_stats.total_time = event->time;
		event.reset();
	}
	// We are done!

	std::cout << "SIMULATION COMPLETED!\n\n";

	for (auto entry : this->processes)
	{
		this->logger.print_per_thread_metrics(entry.second);
	}

	this->logger.print_simulation_metrics(this->calculate_statistics());
}

//==============================================================================
// Event-handling methods
//==============================================================================

void Simulation::handle_thread_arrived(const std::shared_ptr<Event> event)
{
	// set event thread ready
	event->thread->set_ready(event->time);
	// increase stats
	this->system_stats.thread_counts[event->thread->priority]++;
	this->all_threads[event->thread->priority].emplace_back(event->thread);
	// schedule thread
	scheduler->add_to_ready_queue(event->thread);
	if (active_thread != nullptr) {
		return;
	} else {
		std::shared_ptr<Event> next_event = std::make_shared<Event>(
			EventType::DISPATCHER_INVOKED,
			event->time,
			event_num++,
			event->thread,
			nullptr
			);
		add_event(next_event);
	}
}

void Simulation::handle_dispatch_completed(const std::shared_ptr<Event> event)
{
	// with the active thread now loaded, set it to running
	event->thread->set_running(event->time);
	std::shared_ptr<Burst> burst = event->thread->get_next_burst(BurstType::CPU);
	// handle time slicing, if scheduling algorithm supports it
	if ( (event->scheduling_decision->time_slice != -1) && (burst->length > event->scheduling_decision->time_slice) ) {
		int ts = event->scheduling_decision->time_slice;
		burst->length -= ts;
		event->thread->service_time += ts;
		this->system_stats.service_time += ts;
		std::shared_ptr<Event> next_event = std::make_shared<Event>(
			EventType::THREAD_PREEMPTED,
			event->time + ts,
			event_num++,
			event->thread,
			event->scheduling_decision
			);
		io_time_start = event->time; // for handling idle times if CPU becomes idle
		add_event(next_event);
		next_event = std::make_shared<Event>(
			EventType::DISPATCHER_INVOKED,
			event->time + event->scheduling_decision->time_slice,
			event_num++,
			event->thread,
			nullptr
		);
		add_event(next_event);
		return;
	}
	burst = event->thread->pop_next_burst(BurstType::CPU);
	// complete CPU burst
	event->thread->service_time += burst->length;
	this->system_stats.service_time += burst->length;
	std::shared_ptr<Event> next_event;
	if (event->thread->get_next_burst(BurstType::IO) == nullptr) {
		next_event = std::make_shared<Event>(
			EventType::THREAD_COMPLETED,
			event->time + burst->length,
			event_num++,
			event->thread,
			nullptr
			);
	} else {
		next_event = std::make_shared<Event>(
			EventType::CPU_BURST_COMPLETED,
			event->time + burst->length,
			event_num++,
			event->thread,
			nullptr
		);
	}
	add_event(next_event);
}

void Simulation::handle_cpu_burst_completed(const std::shared_ptr<Event> event)
{
	// first, block the thread
	event->thread->set_blocked(event->time);
	// next, complete an I/O burst
	std::shared_ptr<Burst> io_burst = event->thread->pop_next_burst(BurstType::IO);
	event->thread->io_time += io_burst->length;
	this->system_stats.io_time += io_burst->length;
	std::shared_ptr<Event> next_event = std::make_shared<Event>(
		EventType::IO_BURST_COMPLETED,
		event->time + io_burst->length,
		event_num++,
		event->thread,
		nullptr
		);
	io_time_start = event->time;
	add_event(next_event);
	next_event = nullptr;
	// now that it will go do an I/O burst, we may have time for someone else to do something
	next_event = std::make_shared<Event>(
		EventType::DISPATCHER_INVOKED,
		event->time,
		event_num++,
		nullptr,
		nullptr
	);
	add_event(next_event);
}

void Simulation::handle_io_burst_completed(const std::shared_ptr<Event> event)
{
	// the thread can begin executing again when told to do so
	event->thread->set_ready(event->time);
	scheduler->add_to_ready_queue(event->thread);
	if (active_thread != nullptr) {
		return; // if there is work being done
	} else {
		// add time CPU spent idle to stats
		this->system_stats.total_idle_time += event->time - io_time_start;
	}
	std::shared_ptr<Event> next_event = std::make_shared<Event>(
		EventType::DISPATCHER_INVOKED,
		event->time,
		event_num++,
		nullptr,
		nullptr
		);
	add_event(next_event);
}

void Simulation::handle_thread_completed(const std::shared_ptr<Event> event)
{
	event->thread->set_finished(event->time);
	prev_thread = active_thread;
	active_thread = nullptr;
	if (scheduler->empty()) {
		return;
	}
	// invoke dispatcher once more
	std::shared_ptr<Event> next_event = std::make_shared<Event>(
		EventType::DISPATCHER_INVOKED,
		event->time,
		event_num++,
		nullptr,
		nullptr
	);
	add_event(next_event);
}

void Simulation::handle_thread_preempted(const std::shared_ptr<Event> event)
{
	// return thread to READY status
	event->thread->set_ready(event->time);
	scheduler->add_to_ready_queue(event->thread);
	// update stats from partially-completed CPU burst
	std::shared_ptr<Burst> cpu_burst = event->thread->get_next_burst(BurstType::CPU);
	int time_spent = event->time - event->thread->state_change_time;
	event->thread->service_time += time_spent;
	this->system_stats.service_time += time_spent;
	// shorten burst, then check invoker
	cpu_burst->update_time(time_spent);
	if (active_thread != nullptr) {
		return; // if there is work being done
	} else {
		// handle preempted thread CPU idle time
		this->system_stats.total_idle_time += event->time - io_time_start;
	}
	std::shared_ptr<Event> next_event = std::make_shared<Event>(
		EventType::DISPATCHER_INVOKED,
		event->time,
		event_num++,
		event->thread,
		nullptr
	);
	add_event(next_event);
}

void Simulation::handle_dispatcher_invoked(const std::shared_ptr<Event> event)
{
	// save current thread if cpu not idle
	if (active_thread != nullptr) {
		prev_thread = active_thread;
	}
	// try get next thread from scheduler
	std::shared_ptr<SchedulingDecision> sd = scheduler->get_next_thread();
	event->scheduling_decision = sd;
	event->thread = sd->thread;
	if (sd->thread == nullptr) {
		active_thread = nullptr; // no next thread, cpu idle
		return;
	}
	active_thread = sd->thread;

	// check if current and previous threads are from same process
	std::shared_ptr<Event> next_event;
	if ((prev_thread != nullptr) && (active_thread->process_id == prev_thread->process_id)) {
		next_event = std::make_shared<Event>(
			EventType::THREAD_DISPATCH_COMPLETED,
			event->time + thread_switch_overhead,
			event_num++,
			sd->thread,
			sd
			);
		this->system_stats.dispatch_time += thread_switch_overhead;
	} else {
		next_event = std::make_shared<Event>(
			EventType::PROCESS_DISPATCH_COMPLETED,
			event->time + process_switch_overhead,
			event_num++,
			sd->thread,
			sd
			);
		this->system_stats.dispatch_time += process_switch_overhead;
	}
	add_event(next_event);
}

//==============================================================================
// Utility methods
//==============================================================================

SystemStats Simulation::calculate_statistics()
{
	// TODO: Implement functionality for calculating the simulation statistics
	this->system_stats.cpu_utilization =
		100.0 * (1.0 - ((float)this->system_stats.total_idle_time / (float)this->system_stats.total_time));
	this->system_stats.cpu_efficiency  =
		100.0 * ((float)this->system_stats.service_time / (float)this->system_stats.total_time);
	// compute average stats
	int resp_totals[4] = {0};
	int turn_totals[4] = {0};
	for (int i = 0; i < 4; i++) {
		int thr_counts = system_stats.thread_counts[i];
		for (auto thread : this->all_threads[i]) {
			resp_totals[i] += thread->response_time();
			turn_totals[i] += thread->turnaround_time();
		}
		system_stats.avg_thread_response_times[i] =
			(thr_counts != 0) ? (float)resp_totals[i] / (float)thr_counts : 0.0;
		system_stats.avg_thread_turnaround_times[i] =
			(thr_counts != 0) ? (float)turn_totals[i] / (float)thr_counts : 0.0;
	}

	return this->system_stats;
}

void Simulation::add_event(std::shared_ptr<Event> event)
{
	if (event != nullptr)
	{
		this->events.push(event);
	}
}

void Simulation::read_file(const std::string filename)
{
	std::ifstream input_file(filename.c_str());

	if (!input_file)
	{
	std::cerr << "Unable to open simulation file: " << filename << std::endl;
	throw(std::logic_error("Bad file."));
	}

	int num_processes;

	input_file >> num_processes >> this->thread_switch_overhead >> this->process_switch_overhead;

	for (int proc = 0; proc < num_processes; ++proc)
	{
		auto process = read_process(input_file);

		this->processes[process->process_id] = process;
	}
}

std::shared_ptr<Process> Simulation::read_process(std::istream &input)
{
	int process_id, priority;
	int num_threads;

	input >> process_id >> priority >> num_threads;

	auto process = std::make_shared<Process>(process_id, (ProcessPriority)priority);

	// iterate over the threads
	for (int thread_id = 0; thread_id < num_threads; ++thread_id)
	{
		process->threads.emplace_back(read_thread(input, thread_id, process_id, (ProcessPriority)priority));
	}

	return process;
}

std::shared_ptr<Thread> Simulation::read_thread(std::istream &input, int thread_id, int process_id, ProcessPriority priority)
{
	// Stuff
	int arrival_time;
	int num_cpu_bursts;

	input >> arrival_time >> num_cpu_bursts;

	auto thread = std::make_shared<Thread>(arrival_time, thread_id, process_id, priority);

	for (int n = 0, burst_length; n < num_cpu_bursts * 2 - 1; ++n)
	{
		input >> burst_length;

		BurstType burst_type = (n % 2 == 0) ? BurstType::CPU : BurstType::IO;

		thread->bursts.push(std::make_shared<Burst>(burst_type, burst_length));
	}

	this->events.push(std::make_shared<Event>(EventType::THREAD_ARRIVED, thread->arrival_time, this->event_num, thread, nullptr));
	this->event_num++;

	return thread;
}
