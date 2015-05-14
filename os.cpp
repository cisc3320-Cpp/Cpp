// Raymond
// OS project - Revisement

#include<iostream>
#include<vector>
#include<queue>
#include<windows.h>
#include "Job.h"
#include "MemoryManager.h"
#include "cpuScheduler.h"

using namespace std;

//Function Prototypes
void swapper(long, bool);
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
long victimForSpace = -1; //Used as the location of the job currently at the back of I/O Queue; Used when JobTable indicated as full
MemoryManager memory; // Memory table object
cpuScheduler cpu; // CPU scheduler object
deque<long> ioQueue; //IO queue. Contains jobnumbers
long inTransit[2];  // Location and status of job being swapped (0=JobNum 1 = direction of transport(0- to Core 1- To Drum))
bool drumBusy=false; // If a job is being swapped
int jobTable = 0;    // counter used to keep track of jobs on the jobtable
bool noRoomInJt = false; //JobTable initially has room. Used to indicate to swapper whether swapping is needed to make room.
int dummyVariable = 50; //Variable is arbitrary. Used to indicate that JobTable has reached a maximum of 50 jobs.
long whichSwap[1]; // Indicates what swap has occurred. 0 : to Core | 1 : to Drum

void startup(){}

/**************************
    Main Interrupt Handlers
*****************************/

// Crint
// Places incoming jobs on job list
void Crint (long &a, long p[])
{
    jobTable++;  // increments JobTable counter

    // if jobTable is full at 50, it calls swap function
    if (jobTable >= 50){
        //If jobTable is filled at >= 50, then there is no room in JobTable
        noRoomInJt = true;
        //Swaps job at back of I/O from Core out to Drum
        swapper(dummyVariable, noRoomInJt);

        //After swapper returns, handle the newest job that just came in from the drum.
    }
        bookKeeper(p[5]);
        // Add new job to Joblist
        jobsList.push_back(*new Job(p[1],p[2],p[3],p[4],p[5]));

        swapper(p[1], noRoomInJt);
        runningJob = cpu.schedule(jobsList);
        loadAndRun(a,p);

}

// Dskint
// Interrupt when job finishes doing I/O.
void Dskint (long &a, long p[])
{
    bookKeeper(p[5]);
    getIoJob(); // look for first job on queue (current I/O job)

    // Subtract I/O left
    jobsList[ioRunningJob].setIoLeft(jobsList[ioRunningJob].getIoLeft()-1);

    // Unblock and unlatch if there are no more I/O for that job
    if(jobsList[ioRunningJob].getIoLeft()== 0)
    {
        jobsList[ioRunningJob].setBlocked(false);
        jobsList[ioRunningJob].setLatched(false);
    }

    // If the job was killed remove it from job list
    if(jobsList[ioRunningJob].isKilled()== true && jobsList[ioRunningJob].getIoLeft()==0 && jobsList[ioRunningJob].isInMemory()== true)
        removeJob(ioRunningJob);

    ioQueue.pop_front();  //remove jobnumber from queue

    // Schedule next job on queue to do I/O
    if(!ioQueue.empty())
    {
        getIoJob();
        siodisk(jobsList[ioRunningJob].getNumber());
    }

    swapper((notInMem()), noRoomInJt);
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
    if(jobNum!=-1 && jobsList[index].isInMemory() == false)
        swapper(jobNum, noRoomInJt);

    swapper((notInMem()), noRoomInJt);
    runningJob = cpu.schedule(jobsList);
    loadAndRun(a,p);
}

// Tro
// Occurs during an abnormal termination of jobs
void Tro (long &a, long p[])
{
    bookKeeper(p[5]);

    // if no i/o, terminate, or else, set to terminate after its i/o finishes
    if (jobsList[runningJob].getIoLeft() == 0)
        removeJob(runningJob);
    else
        jobsList[runningJob].setKilled(true);

    swapper((notInMem()), noRoomInJt);
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

            // if the queue is empty, immediately do i/o
            if(ioQueue.empty())
                siodisk(jobsList[runningJob].getNumber());

            // put job number on i/o queue and set as latched
            ioQueue.push_front(jobsList[runningJob].getNumber());
            jobsList[runningJob].setIoLeft(jobsList[runningJob].getIoLeft()+1);
            jobsList[runningJob].setLatched(true);

            // set ioRunningJob to the index
            getIoJob();

            break;

        // job requested to be blocked
        case 7:
            if(jobsList[runningJob].getIoLeft()!= 0)
                jobsList[runningJob].setBlocked(true);
            break;

        default:
            cout<<"ERROR: The value of 'a' is invalid!"<<endl;
    }

    swapper((notInMem()), noRoomInJt);
    runningJob = cpu.schedule(jobsList);
    loadAndRun(a,p);
}
/******************************
    Other functions
*******************************/
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
// Updates memory table
void update()
{
    // scans job list to see if there are any jobs to remove or add to memory
    for(long i=0; i<jobsList.size(); i++)
    {
        if(jobsList[i].isInMemory()== true)
            memory.setJob(i,jobsList);
        if(jobsList[i].isKilled()== true && jobsList[i].getIoLeft()==0)
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
    jobTable--;  // removes job from jobTable counter
}

// Swapper
// Swaps jobs from Drum to Memory or Memory to Drum
// Updates memory accordingly
void swapper(long jobNum, bool makeSpace)
{
    if (!drumBusy && jobNum!=-1)
    {
        /* If check from Crint indicates that JobTable is not full */
        if(makeSpace == false){
            // update memory
            update();

            // look for a job in memory that can fit in the table
            long index = memory.findJob(jobsList,jobNum);
            long i =memory.MemTable((jobsList[index].getSize()));

            //if it can fit
            if(i < 100 && i >= 0)
            {
                // if the job was not already in memory, set it in
                if(jobsList[index].isInMemory()== false)
                {
                    jobsList[index].setLocation(i);
                    whichSwap[0] = 0;
                    siodrum(jobsList[index].getNumber(),jobsList[index].getSize(),jobsList[index].getLocation(),0);
                    inTransit[0]=jobNum;
                    inTransit[1]=0;
                    memory.setTable(jobsList,index,i);
                    drumBusy=true; // swapper is now busy
                }
            }
        }
        /* If check from Crint indicates that JobTable is full */
        if(makeSpace == true){
            for(long i=0; i< jobsList.size(); i++) //Looks for a matching job on jobsList as job on back of ioQueue
                if(jobsList[i].getNumber() == ioQueue.back())
                    victimForSpace = i;
            if(jobsList[victimForSpace].isKilled()== true && jobsList[victimForSpace].getIoLeft()==0 && jobsList[victimForSpace].isInMemory()== true){
                    ioQueue.pop_back(); //pops the job at back of ioQueue
                    whichSwap[0] = 1;
                    siodrum(jobsList[victimForSpace].getNumber(),jobsList[victimForSpace].getSize(),jobsList[victimForSpace].getLocation(),1);
                    removeJob(victimForSpace);
                    inTransit[0] = victimForSpace;   //What job is inTransit
                    inTransit[1] = 1;  //In transit from Core to Drum
                    drumBusy=true; //swapper is now busy
                    noRoomInJt = false; //Indicate that JobTable is no longer full
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
    if(jobsList[runningJob].isInMemory() != true || jobsList[runningJob].isBlocked() == true || runningJob == -1)
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
