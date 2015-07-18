##CS111 Spring '15 MiniLab2: Scheduling & Synchronization 

Roland Zeng  
Daniel Ong

-------------------------
Spec / Skeleton code: http://www.read.seas.harvard.edu/~kohler/class/cs111-w11/weensyos2.html

Features added to skeleton code:

- Created scheduler 1 to schedule via priority levels. schedos-1 tasks are assigned higher priority than schedos-2 tasks, etc. 
- Created scheduler 2 to schedule via priority levels. Each process is given a priority level upon creation. 
- Created scheduler 3 to schedule via time-sharing. The higher the priority level number, the more CPU time a process gets. Process 2 is given twice as much time as process 1, etc. 
- Implemented synchronization using clock interrupts and multithreading.
