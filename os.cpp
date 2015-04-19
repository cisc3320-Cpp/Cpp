#include <iostream>
#include <queue>

using namespace std;

void printLinkedList();
void swapper(long int jobNo, long int jobSize, long int transferDir);
void siodrum(long int jobNum, long int jobSize, long int startCoreAddr, long int transferDir);
void scheduler();
void display(struct jobTableEntry *head);
struct jobTableEntry *searchForFreeSpace(struct jobTableEntry *head, long int requiredSize);
void startup();
void Svc(long int&, long int[]);
void Drmint(long int&, long int[]);
void Dskint(long int&, long int[]);
void Crint(long int&, long int[]);
void Tro(long int&, long int[]);

//Struct used to store information about each job
	struct job                              /* JobTable */
	{
		int jobNo;
		int priority;
		int jobSize;
		long maxCpuTime;
		long currentTime;
		job *next;
	} ;

	struct jobTableEntry                    /* Free Space Table struct */
	{
		int jobNo;
		int startingAddr;                   //* How do we obtain starting address? *//
		int jobSize;
        boolean job_Blocked = false;        /* Default block status - Status is true when a request is made to be blocked  */
        boolean iolatchbit = false;         /* status indicating whether job is doing io */
		jobTableEntry *next;
	};

	job *root;
	job *traverser;
	jobTableEntry *curr;
	jobTableEntry *jobTableRoot;
	jobTableEntry *jobTablePrev;

	int listCounter;
	int freeSpace;
	int usedSpace;
	int nextFreeSpaceMarker;
	int freeSpaceTable [100] = { };
	const int CORE_TO_DRUM = 1;
	const int DRUM_TO_CORE = 0;
	queue <int> ioqueue;

// Allows initialization of (static) system variables declared above.
// Called once at start of the simulation.
void startup()
{
	listCounter = 0;
	freeSpace = 100;                                //* What does freeSpace do? It is just an int initialized with the value of 100 *//
	usedSpace = 0;
	/* queue <int> ioqueue; */
	queue < int >::size_type i;
	i = ioqueue.size();

	root = new job;		//creates a new node...or our first node which is at the begining
	root->next = NULL;	//tells what to point to
	traverser = root;	//points to the current node

	jobTableRoot = new jobTableEntry;               /* initial node at the beginning */
	jobTableRoot->next = NULL;                      /* the next node of the node being pointed is the end */
	jobTablePrev = jobTableRoot;                    /* points at the previous node; points at root right now */
	curr = jobTableRoot;                            /* points at the current node; points at root right now */
}

// INTERRUPT HANDLERS
// The following 5 functions are the interrupt handlers. The arguments
// passed from the environment are detailed with each function below.
// See RUNNING A JOB, below, for additional information


// Indicates the arrival of a new job on the drum.
void Crint (long int &a, long int p[])
{
	if (listCounter <=50 ){	//test for maximum capacity
		traverser->jobNo = p[1];                                /* Doesn't check whether current node memory is taken */
		traverser->priority = p[2];
		traverser->jobSize = p[3];
		traverser->maxCpuTime = p[4];
		traverser->currentTime = p[5];
		listCounter++;
		traverser->next = new job;		//creates a new node            /* the next node of node being pointed at creates a new node */
		traverser = traverser->next;	//points to the new node        /* traverser now points at the previous node's next node */
		traverser->next = NULL;			//new node is now the end
	}
	//printLinkedList();
	swapper(p[1], p[3], DRUM_TO_CORE);
}


void Dskint (long int &a, long int p[])
{
    cout<<"OS OUTPUT: i/o transfer from disk and memory has completed";
    // Disk interrupt.
    // At call : p [5] = current time
}

void Drmint (long int &a, long int p[])
{
	//cout<<"OS OUTPUT: drum to memory has completed";
    // Drum interrupt.
    // At call : p [5] = current time
    p[2] = 0;
    a = 2;
}

void Tro (long int &a, long int p[])
{
	cout<<"OS OUTPUT: requesting to be terminated";
    if (p[5] == p[4]){
   		cout<<p[1]<<" job completed";
    }
    //swapper(p[1], p[3], , CORE_TO_DRUM);
}

void Svc(long int &a, long int p[])
{
	cout<<"OS OUTPUT: requesting svc";

    if(a == 5)   /* Request for termination */
    {
        /* Free its struct jobtable entry */


    }
    if(a == 6)   /* Request for disk I/O */
    {
        /* Maybe call a function that contains an I/O queue where the job we sent will be placed in the queue
         and when it is the job's turn to do I/O it will call our custom made function "do_siodisk" which calls the siodisk() function from SOS */

        do_io_operation(p[1]);                          /* puts I/O request on queue */
    }
    if(a == 7)   /* Request to be blocked */
    {
        /* Blocks the job by preventing it from running on CPU, maybe make a loop that checks all "Outstanding I/O requests have been completed */
        /* before unblocking the job */

        /* Change the job being serviced's block bit to true. */
        /* Check if latch bit is true, if true, then 'wait' until all outstanding IO requests are completed. */
            /*  */
        /* Change the job's block bit to true and call swapper to stop it from running on CPU */
        /* After swap back to drum, it must go through the ready queue; the scheduler will check if it has a blocked bit*/
            /* If there is a blocked bit, scheduler directly calls do_io_operation */
    }
}

//The swapper just moves jobs in and out of memory based on the parameters passed
//usually after the swapper has completed its work with sos, the drumint is called
void swapper(long int jobNo, long int jobSize, long int transferDir)
{
    //* This if is only for the root node and its next node pointer *//
	if (transferDir == DRUM_TO_CORE){			//if moving from drum to core
		if (jobSize <= freeSpace){				//if there is enough free space available in memory
			if (freeSpace == 100){				//if nothing is in memory as yet then the starting addr is going to be
				curr->jobNo = jobNo;			//basically 0...unlike the others which will be previous job + prev size
				curr->startingAddr = jobTablePrev->startingAddr;            //* How is the address of node obtain? Shouldn't we change what it is pointing at? *//
				curr->jobSize = jobSize;                                        //* Shouldn't we be pointing at curr since curr looks to the next node? *//
			}else{
				curr->jobNo = jobNo;
				curr->startingAddr = jobTablePrev->startingAddr + jobTablePrev->jobSize;
				curr->jobSize = jobSize;
			}
			jobTablePrev = curr;
			curr->next = new jobTableEntry;
			curr = curr->next;
			curr->next = NULL;
			siodrum(jobNo, jobSize, curr->startingAddr, transferDir);
			display(jobTableRoot);
		}else{												//if there is not enough space in memory
			//first free space >= size of job
			//NOTE: this is just getting the first space in memory >= jobsize...not taking into consideration the status of it i.e running or not
			//a check to see the status can later be implemented and then re check for another job in the series starting the ptr from there
			jobTableEntry *entryToDelete = searchForFreeSpace(jobTableRoot, jobSize);	//returns the job which is >= to the jobsize required
			int tempStartingAddr = entryToDelete->startingAddr;
			//siodrum(entryToDelete->jobNo, entryToDelete->jobSize, entryToDelete->startingAddr, CORE_TO_DRUM);
			//siodrum(jobNo, jobSize, tempStartingAddr, DRUM_TO_CORE);
			//add to the freespace table
		}
		freeSpace = freeSpace - jobTablePrev->jobSize;		//calculates the total amount of free space left in the system
		usedSpace = usedSpace + jobTablePrev->jobSize;		//calculates how much used
	}
	if (transferDir == CORE_TO_DRUM){
		cout<<"requries immediate transfer from core to drum";
		//need to finish
	}
}

struct jobTableEntry *searchForFreeSpace(struct jobTableEntry *head, long int requiredSize)
{
	jobTableEntry *curr = head;
	while(curr){
		if (curr->jobSize >= requiredSize) return curr;
		curr = curr->next;
	}
	cout<<"No memory large enough to house the incoming job"<<endl;
}

void scheduler()
{

}

//used to display the entire free space table
void display(struct jobTableEntry *head)
{
	jobTableEntry *list = head;
	while(list) {
		cout << list->startingAddr << " "<<list->jobSize<<endl;                 //* Is the startingAddr the address or something represented in memory? *//
		list = list->next;
	}
	cout << endl;
	cout << endl;
}

void printLinkedList()
{
	traverser = root;
	if ( traverser != 0 ) { //Makes sure there is a place to start
	  while ( traverser->next != 0 ) {
	    cout<<traverser->jobNo<<" ";
		cout<<traverser->priority<<" ";
		cout<<traverser->jobSize<<" ";
		cout<<traverser->maxCpuTime<<" ";
		cout<<traverser->currentTime<<" ";
	    traverser = traverser->next;
	  }
	cout<<traverser->jobNo<<" ";
	cout<<traverser->priority<<" ";
	cout<<traverser->jobSize<<" ";
	cout<<traverser->maxCpuTime<<" ";
	cout<<traverser->currentTime<<" ";
	}
}

//called by svc to add an I/O request to I/O queue
void do_io_operation(int long p[])
{
    if(ioqueue.front != 1)              /* '1' is changed depending on the first job number */
    {
        ioqueue.push(p[1]);
        i = ioqueue.front();
        cout<<" The element at the front of the queue is" << i << endl;
    /*
    Change the running job's latch bit to true when it is that running job's turn to do io
    */
        siodisk(p[1]);                      //* How to notify scheduler that siodisk was called? *//
    }
    else
    {
        ioqueue.pop();
        ioqueue.push(p[1]);
        i = ioqueue.front();
        cout<<" The element at the front of the queue is" << i << endl;
    /*
    Change the running job's latch bit to true when it is that running job's turn to do !/O.
    On the scheduler, once a job's I/O requests are completed, change the latch bit to false.
    */
        siodisk(p[1]);
    }
}

void complete_io()
