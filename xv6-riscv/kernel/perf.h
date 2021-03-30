
struct perf
{
    int ctime; // process creation time.
    int ttime; // process termination time.
    int stime; // the total time the process spent in the SLEEPING state.
    int retime; // the total time the process spent in the RUNNABLE state.
    int rutime; // the total time the process spent in the RUNNING state.
    int average_bursttime; //average of bursstimes in 100ths (so average*100)
};
