TODO:
   - [ ] Change my jank way of stat tracking to use an iterator over the :code:`std::map<int,std::shared_ptr<Process>>` processes
   - [ ] Implement the following:
      - [ ] SPN (shortest process next)
      - [ ] Priority (priority queue based on thread priority :code:`[S<-I<-N<-B]`)
      - [ ] MLQS (multilevel queue scheduling)
      - [ ] CFS ("Completely Fair Scheduling", i.e. Linux)

CPU Scheduling Simulator
==============================================

The goal of this project was to develop a CPU scheduling simulation that can complete the execution of a group of multi-threaded processes.  
It supports plug-and-play scheduling algorithms that implement a common interface, that the user can then specify to use via a command-line flag.  
At the end of execution, the program calculates and displays several performance criteria obtained by the simulation.

The OS 9e textbook was very useful in the development of this program, specifically the sections on scheduling, threads, and process execution/contexts.

1 Constraints
--------------------
1. There is a single CPU, so only one process can be running at a time.
2. There are an infinite number of I/O devices, so any number of processes can be blocked on I/O at the same time.
3. Processes consist of one or more kernel-level threads (KLTs).
4. Threads (not processes) can exist in one of five states:

   - NEW
   - READY
   - RUNNING
   - BLOCKED
   - EXIT

5. Dispatching threads requires a non-zero amount of OS overhead:

   - If the previously executed thread belongs to a different process than the new thread, a full process switch occurs. This is also the case for the first thread being executed.
   - If the previously executed thread belongs to the same process as the new thread being dispatched, a cheaper thread switch is done.
   - A full process switch includes any work required by a thread switch.
   
6. Threads, processes, and dispatch overhead are specified via a file whose format is specified in the next section.
7. Each thread requires a sequence of CPU and I/O bursts of varying lengths as specified by an input file.
8. Processes have an associated priority, specified as part of the file. Each thread in a process has the same priority as its parent process.

   - 0: SYSTEM (highest priority)
   - 1: INTERACTIVE
   - 2: NORMAL
   - 3: BATCH (lowest priority)

9. All processes have a distinct process ID, specified as part of the file. Thread IDs are unique only within the context of their owning process (so the first thread in every process has an ID of 0).
10. Overhead is incurred only when dispatching a thread (transitioning it from READY to RUNNING); all other OS actions require zero OS overhead. For example, adding a thread to a ready queue or initiating I/O are both ”free”.
11. Threads for a given process can arrive at any time, even if some other process is currently running (i.e., some external entity—not the CPU—is responsible for creating threads).
12. Threads get executed, not processes.

2 Scheduling Algorithms
--------------------
Currently implemented scheduling algorithms:

- First Come, First Served (--algorithm FCFS)
- Round Robin (--algorithm RR)

3 Next-Event Simulation
--------------------
The simulation follows the next-event pattern. At any given time, the simulation is in a single state. The simulation state can only change at event times, where an event is defined as an occurrence that may change the state of the system.

Since the simulation state only changes at an event, the ”clock” can be advanced to the next scheduled event–regardless of whether the next event is 1 or 1,000,000 time units in the future. This is why it is called a ”next-event” simulation model. Time is in arbitrary units:

- **THREAD ARRIVED**: A thread has been created in the system.
- **THREAD DISPATCH COMPLETED**: A thread switch has completed, allowing a new thread to start executing on the CPU.
- **PROCESS DISPATCH COMPLETED**: A process switch has completed, allowing a new thread to start executing on the CPU.
- **CPU BURST COMPLETED**: A thread has finished one of its CPU bursts and has initiated an I/O request.
- **IO BURST COMPLETED**: A thread has finished one of its I/O bursts and is once again ready to be executed.
- **THREAD COMPLETED**: A thread has finished the last of its CPU bursts.
- **THREAD PREEMPTED**: A thread has been preempted during execution of one of its CPU bursts.
- **DISPATCHER INVOKED**: The OS dispatcher routine has been invoked to determine the next thread to be run on the CPU

4 Simulation File Format
--------------------
The simulation file specifies a complete specification of a unique scheduling scenario. It is formatted as follows:

.. code-block::

   num_processes thread_switch_overhead process_switch_overhead
   
   process_id process_type num_threads    // Process IDs are unique
   thread_0_arrival_time num_cpu_bursts
   cpu_time io_time
   cpu_time io_time
   ...                                    // Repeat for num_cpu_bursts
   cpu_time

   thread_1_arrival_time num_cpu_bursts
   cpu_time io_time
   cpu_time io_time
   ...                                    // Repeat for num_cpu_bursts
   cpu_time
   
   ...                                    // Repeat for the number of threads

   process_id process_type num_threads    // We are now reading in the next process
   thread_0_arrival_time num_cpu_bursts
   cpu_time io_time
   
   cpu_time io_time
   ...                                    // Repeat for num_cpu_bursts
   cpu_time

   thread_1_arrival_time num_cpu_bursts
   cpu_time io_time
   cpu_time io_time
   ...                                    // Repeat for num_cpu_bursts
   cpu_time

   ...                                    // Repeat for the number of threads
   
   ...                                    // Keep reading until EOF is reached
   
Here is a commented example. The comments will not be in an actual simulation file.

.. code-block:: 

   2 3 7    // 2 processes , thread overhead is 3, process overhead is 7
   
   0 1 2    // Process 0, Priority is INTERACTIVE , it contains 2 threads
   0 3      // The first thread arrives at time 0 and has 3 bursts
   4 5      // The first pair of bursts : CPU is 4, IO is 5
   3 6      // The second pair of bursts : CPU is 3, IO is 6
   1        // The last CPU burst has a length of 1

   1 2      // The second thread in Process 0 arrives at time 1 and has 2 bursts
   2 2      // The first pair of bursts : CPU is 2, IO is 2
   7        // The last CPU burst has a length of 7

   1 0 3    // Process 1, priority is SYSTEM , it contains 3 threads
   5 3      // The first thread arrives at time 5 and has 3 bursts
   4 1      // The first pair of bursts : CPU is 4, IO is 1
   2 2      // The second pair of bursts : CPU is 2, IO is 2
   2        // The last CPU burst has a length of 2

   6 2      // The second thread arrives at time 6 and has 2 bursts
   2 2      // The first pair of bursts : CPU is 2, IO is 2
   3        // The last CPU burst has a length of 3

   7 5      // The third thread arrives at time 7 and has 5 bursts
   5 7      // CPU burst of 5 and IO of 7
   2 1      // CPU burst of 2 and IO of 1
   8 1      // CPU burst of 8 and IO of 1
   5 7      // CPU burst of 5 and IO of 7
   3        // The last CPU burst has a length of 3
   
5 Command Line Parsing
--------------------
.. code-block:: 

   ./cpu-sim [flags] [simulation_file]
   
   -h, --help
      Print a help message on how to use the program.
      
   -m, --metrics
      If set, output general metrics for the simulation.
      
   -s, --time_slice [positive integer]
      The time slice for preemptive scheduling algorithms.
      
   -t, --per_thread
      If set, outputs per-thread metrics at the end of the simulation.
      
   -v, --verbose
      If set, outputs all state transitions and scheduling choices.
      
   -a, --algorithm <algorithm>
      The scheduling algorithm to use, implementation depending.

5.1 --metrics
~~~~~~~~~~~~~~~~~~~
When the metrics flag has been specified, it outputs info similar to the following:

.. code-block::
   
   SIMULATION COMPLETED !

   SYSTEM THREADS :
      Total Count : 3
      Avg . response time : 23.33
      Avg . turnaround time : 94.67
   
   INTERACTIVE THREADS :
      Total Count : 2
      Avg . response time : 10.00
      Avg . turnaround time : 73.50

   NORMAL THREADS :
      Total Count : 0
      Avg . response time : 0.00
      Avg . turnaround time : 0.00

   BATCH THREADS :
      Total Count : 0
      Avg . response time : 0.00
      Avg . turnaround time : 0.00

   Total elapsed time : 130
   Total service time : 53
   Total I/O time : 34
   Total dispatch time : 69
   Total idle time : 8

   CPU utilization : 93.85%
   CPU efficiency : 40.77%

5.2 --per thread
~~~~~~~~~~~~~~~~~~~
When the per thread flag has been specified, it outputs information about each of the threads.

.. code-block::

   SIMULATION COMPLETED !

   Process 0 [INTERACTIVE]:
      Thread   0:    ARR : 0      CPU : 8     I/O: 11     TRT: 88        END: 88
      Thread   1:    ARR : 1      CPU : 9     I/O: 2      TRT: 59        END: 60

   Process 1 [SYSTEM]:
      Thread   0:    ARR : 5      CPU : 8     I/O: 3      TRT : 92       END: 97
      Thread   1:    ARR : 6      CPU : 5     I/O: 2      TRT : 69       END: 75
      Thread   2:    ARR : 7      CPU : 23    I/O: 16     TRT : 123      END: 130
   
5.3 --verbose
~~~~~~~~~~~~~~~~~~~
When the verbose flag has been specified, it outputs information about each state transition. This information is outputted ”on the fly”.

.. code-block::

   At time 0:
      THREAD_ARRIVED
      Thread 0 in process 0 [INTERACTIVE]
      Transitioned from NEW to READY

   At time 0:
      DISPATCHER_INVOKED
      Thread 0 in process 0 [INTERACTIVE]
      Selected from 1 threads . Will run to completion of burst.
      
This continues until the end of the simulation:

.. code-block::

   At time 127:
      THREAD_DISPATCH_COMPLETED
      Thread 2 in process 1 [ SYSTEM ]
      Transitioned from READY to RUNNING

   At time 130:
      THREAD_COMPLETED
      Thread 2 in process 1 [ SYSTEM ]
      Transitioned from RUNNING to EXIT

   SIMULATION COMPLETED !

5.4 Notes regarding use of this repository
~~~~~~~~~~~~~~~~~~~
Don't blindly rip the code for this if you are taking a course in OS development and don't understand what it is doing! Not to mention, it may trigger some plagiarism flags ;)
