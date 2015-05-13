// Jason Tan
// OS project - version 2

#include<iostream>
#include<vector>
#include<queue>
#include<windows.h>
#include "Job.h"
#include "MemoryManager.h"
#include "cpuScheduler.h"


using namespace std;

//Function Prototypes
void swapper(long);
void update();
void bookKeeper(long);
void removeJob(long);
void getIoJob();
void getJobByNum(long);
long notInMem();
void loadAndRun(long&, long []);
void ontrace();
void offtrace();
void siodisk(long jobnum);
void siodrum(long jobnum, long jobsize, long coreaddress, long direction);


//Global Variables
vector<Job> jobsList; // Our list of jobs currently in the system
long runningJob = -1; //Location of the job in jobsList that is running in CPU
long ioRunningJob = 0;//Location of the job in jobsList that is currently doing IO
MemoryManager memory; // Memory table object
cpuScheduler cpu; // CPU scheduler object
queue<long> ioQueue; //IO queue. Contains jobnumbers
long inTransit[2];  // Location and status of job being swapped (0=JobNum 1 = direction of transport(0- to Core 1- To Drum))
bool drumBusy=false; // If a job is being swapped
//int ioJobTracker = 0; //keeps track of a particular job's own requests that come in for IO
                        //decrement when a job terminates
//vector<queue<long>> JobVecQueue; //Instead of a ioJobTracker; keep a particular job's own job requests in its own queue

void startup(){}

/**************************
    Main Interrupt Handlers
*****************************/

// Crint
// Places incoming jobs on job list
void Crint (long &a, long p[])
{
    bookKeeper(p[5]);
    // Add new job to Joblist
    jobsList.push_back(*new Job(p[1],p[2],p[3],p[4],p[5]));

    swapper(p[1]);
    runningJob = cpu.schedule(jobsList);
    loadAndRun(a,p);
}

// Dskint
// Interrupt when job finishes doing IO.
void Dskint (long &a, long p[])
{
    bookKeeper(p[5]);
    getIoJob(); // look for first job on queue (current IO job)

    // Subtract IO left
    jobsList[ioRunningJob].setIoLeft(jobsList[ioRunningJob].getIoLeft()-1);

    // Unblock and unlatch if there are no more IO for that job
    if(jobsList[ioRunningJob].getIoLeft()== 0)
    {
        jobsList[ioRunningJob].setBlocked(false);
        jobsList[ioRunningJob].setLatched(false);
    }

    // If the job was killed remover from job list
    if(jobsList[ioRunningJob].isKilled()== true && jobsList[ioRunningJob].getIoLeft()==0 && jobsList[ioRunningJob].isInMemory()== true)
        removeJob(ioRunningJob);

    ioQueue.pop();  //remove jobnumber from queue

    // Schedule next job on queue to do IO
    if(!ioQueue.empty())
    {
        getIoJob();
        siodisk(jobsList[ioRunningJob].getNumber());
    }

    swapper((notInMem()));
    runningJob = cpu.schedule(jobsList);
    loadAndRun(a,p);
}

// Drmint
// Interrupt that occurs after a successful swap in or out.
void Drmint (long &a, long p[])
{
    //Job has completed the swap so in transit should be free.
    drumBusy = false;
    bookKeeper(p[5]);

    // swap in. Set so that job is in memory
    long index = memory.findJob(jobsList,inTransit[0]);
    jobsList[index].setInMemory(true);

    // look for a job in the list not in memory and put in memory
    long jobNum = notInMem();
    if(jobNum!=-1 && !jobsList[index].isInMemory())
        swapper(jobNum);

    swapper((notInMem()));
    runningJob = cpu.schedule(jobsList);
    loadAndRun(a,p);
}

// Tro
// Occurs during an abnormal termination of jobs
void Tro (long &a, long p[])
{
    bookKeeper(p[5]);

    // if no io, terminate, or else, set to terminate after its i/o finishes
    if (jobsList[runningJob].getIoLeft() == 0)
        removeJob(runningJob);
    else
        jobsList[runningJob].setKilled(true);

    swapper((notInMem()));
    runningJob = cpu.schedule(jobsList);
    loadAndRun(a,p);
}

// Svc
// Service request
void Svc(long &a, long p[])
{
    bookKeeper(p[5]);

    switch(a){
        // a job requested to terminate. Remove from list if there is no outstanding i/o
        // else, set to be killed later
        case 5:
            if (jobsList[runningJob].getIoLeft() == 0 && jobsList[runningJob].isKilled()== true)
                removeJob(runningJob);
            else
                jobsList[runningJob].setKilled(true);
            break;

        // a job requested to do i/o
        case 6:

            //ioJobTracker++;

            // if the queue is empty, immediately do i/o
            if(ioQueue.empty())
                siodisk(jobsList[runningJob].getNumber());

            // put job number on i/o queue and set as latched
            ioQueue.push(jobsList[runningJob].getNumber());
            jobsList[runningJob].setIoLeft(jobsList[runningJob].getIoLeft()+1);
            jobsList[runningJob].setLatched(true);

            // set ioRunningJob to the index
            getIoJob();

            break;

        // job requested to be blocked
        case 7:

            //set the job that wants this service to blocked
            //push onto queue the job that wants to be blocked
            //ioQueue.push(jobsList[find job that called for service])

            //findBlocked();

            if(jobsList[ioRunningJob].getIoLeft()!= 0)
                jobsList[ioRunningJob].setBlocked(true);
            break;

        default:
            cout<<"ERROR: The value of 'a' is invalid!"<<endl;
    }

    swapper((notInMem()));
    runningJob = cpu.schedule(jobsList);
    loadAndRun(a,p);
}
/******************************
    Other functions
*******************************/

/*void findBlocked()
{
        for(long i=0; i< jobsList.size(); i++)
    {
        //look in back of queue for that blocked job
        if(jobsList[i].getNumber() == ioQueue.back() && jobsList[i].isBlocked)
            blockedRunningJob = i;
    }
}
*/

// BookKeeper function
// Keeps track of remaining Max CPU Time
// Called when a job first enters an interrupt
void bookKeeper(long currentTime)
{
    // If a job was running, subtract the amount of time it spent in the CPU
    // Uses a job's currentTime and enteredTime to figure this out
    // Remove that time from Max CPU time to see how much time it has left
    if (runningJob!= -1)
    {
        long value = currentTime - jobsList[runningJob].getEnteredTime();
        jobsList[runningJob].setCurrentTime(jobsList[runningJob].getMaxCpu());
        jobsList[runningJob].setMaxCpu(jobsList[runningJob].getMaxCpu() - value);
    }
}

// Update
// Updataes memory table
void update()
{
    // scans job list to see if there are any jobs to remove or add to memoty
    for(long i=0; i<jobsList.size(); i++)
    {
        if(jobsList[i].isInMemory() == true)                                 //job arriving
            memory.setJob(i,jobsList);
        if(jobsList[i].isKilled()== true && jobsList[i].getIoLeft()==0)     //termination request
            removeJob(i);
    }
}

// getIoJob
// Sets ioRunningJob to the index of the job
// that is waiting to do i/o in the front of the IOqueue.
void getIoJob()
{
    for(long i=0; i< jobsList.size(); i++)
    {
        if(jobsList[i].getNumber() == ioQueue.front())
            ioRunningJob = i;
    }
}

// removeJob
// Removes a job from jobsList by using its index.
void removeJob(long index)
{
    memory.eraseJob(jobsList[index].getLocation());
    jobsList.erase(jobsList.begin()+index);
}

// Swapper
// Swaps jobs from Drum to Memory or Memory to Drum
// Updates memory accordingly
void swapper(long jobNum)
{
    if (!drumBusy && jobNum!=-1)
    {
        // update memory
        update();

        // look for a job in memory that can fit in the table
        long index = memory.findJob(jobsList,jobNum);
        long i =memory.MemTable((jobsList[index].getSize()));

        //if it can fit
        if(i<100 && i >=0)
        {
            // if the job was not already in memory, set it in
            if(jobsList[index].isInMemory()== false)
            {
                jobsList[index].setLocation(i);
                siodrum(jobsList[index].getNumber(),jobsList[index].getSize(),jobsList[index].getLocation(),0);
                inTransit[0]=jobNum;
                inTransit[1]=0;
                memory.setTable(jobsList,index,i);
                drumBusy=true; // swapper in now busy
            }
        }
    }
}

// notInMem
// Returns the job number of a job from the list that's not in memory.
long notInMem()
{
    long index=0,time=10000000000;
    for(long i=0; i<jobsList.size(); i++)
    {
        if(jobsList[i].isInMemory()== false)
            if(memory.MemTable(i)<100)
                if(jobsList[i].getMaxCpu()<time){
                    time=jobsList[i].getMaxCpu();
                    index=i;
                }
    }
    if(index!=0)
        return jobsList[index].getNumber();
    return -1;
}

// loadAndrun
// The dispatcher
void loadAndRun(long &a, long p[])
{
    // If the scheduler did not find a job, set the cpu idle
    if(!jobsList[runningJob].isInMemory() || jobsList[runningJob].isBlocked() || runningJob == -1)
        a=1;

    // if there was a job scheduled to run, set p[] accordingly and set CPU to active
    else
    {
        a = 2;
        p[2] = jobsList[runningJob].getLocation();
        p[3] = jobsList[runningJob].getSize();
        p[4] = jobsList[runningJob].getMaxCpu();
        jobsList[runningJob].setEnteredTime(p[5]);
        jobsList[runningJob].setInCpu(true);
    }
}
