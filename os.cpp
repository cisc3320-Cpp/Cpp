#include <iostream>

using namespace std;

void printLinkedList();
void swapper(int jobNo, int jobSize, int transferDir);
void siodrum(int jobNum, int jobSize, int startCoreAddr, int transferDir);
void scheduler();
void display(struct jobTableEntry *head);
struct jobTableEntry *searchForFreeSpace(struct jobTableEntry *head, int requiredSize);

//Struct used to store information about each job
	struct job
	{
		int jobNo;
		int priority;
		int jobSize;
		long maxCpuTime;
		long currentTime;
		job *next;
	} ;

	struct jobTableEntry
	{
		int jobNo;
		int startingAddr;
		int jobSize;
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

// Allows initialization of (static) system variables declared above.
// Called once at start of the simulation.
void startup()
{
	listCounter = 0; 
	freeSpace = 100;
	usedSpace = 0;

	root = new job;		//creates a new node...or our first node which is at the begining
	root->next = NULL;	//tells what to point to 
	traverser = root;	//points to the current node

	jobTableRoot = new jobTableEntry;
	jobTableRoot->next = NULL;
	jobTablePrev = jobTableRoot;
	curr = jobTableRoot;
}

// INTERRUPT HANDLERS
// The following 5 functions are the interrupt handlers. The arguments
// passed from the environment are detailed with each function below.
// See RUNNING A JOB, below, for additional information


// Indicates the arrival of a new job on the drum.
void Crint (int &a, int p[])
{
	if (listCounter <=50 ){	//test for maximum capacity
		traverser->jobNo = p[1];
		traverser->priority = p[2];
		traverser->jobSize = p[3];
		traverser->maxCpuTime = p[4];
		traverser->currentTime = p[5];
		listCounter++;
		traverser->next = new job;		//creates a new node
		traverser = traverser->next;	//points to the new node
		traverser->next = NULL;			//new node is now the end
	}
	//printLinkedList();
	swapper(p[1], p[3], DRUM_TO_CORE);
}


void Dskint (int &a, int p[])
{
    cout<<"OS OUTPUT: i/o transfer from disk and memory has completed";
    // Disk interrupt.
    // At call : p [5] = current time
}

void Drmint (int &a, int p[])
{
	//cout<<"OS OUTPUT: drum to memory has completed";
    // Drum interrupt.
    // At call : p [5] = current time
    p[2] = 0;
    a = 2;
}

void Tro (int &a, int p[])
{
	cout<<"OS OUTPUT: requesting to be terminated";
    if (p[5] == p[4]){
   		cout<<p[1]<<" job completed";
    }
    //swapper(p[1], p[3], , CORE_TO_DRUM);
}

void Svc(int &a, int p[])
{     
	cout<<"OS OUTPUT: requesting svc";
	if(a == 5){
		// a = 5 => job has terminated
	} else if(a == 6){
		// a = 6 => job requests disk i/o
	} else if(a == 7){
		//job wants to be blocked until all its pending
      	// I/O requests are completed
	}
}

//The swapper just moves jobs in and out of memory based on the parameters passed
//usually after the swapper has completed its work with sos, the drumint is called
void swapper(int jobNo, int jobSize, int transferDir)
{
	if (transferDir == DRUM_TO_CORE){			//if moving from drum to core 
		if (jobSize <= freeSpace){				//if there is enough free space available in memory
			if (freeSpace == 100){				//if nothing is in memory as yet then the starting addr is going to be
				curr->jobNo = jobNo;			//basically 0...unlike the others which will be previous job + prev size
				curr->startingAddr = jobTablePrev->startingAddr;
				curr->jobSize = jobSize;
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
			siodrum(entryToDelete->jobNo, entryToDelete->jobSize, entryToDelete->startingAddr, CORE_TO_DRUM);
			siodrum(jobNo, jobSize, tempStartingAddr, DRUM_TO_CORE);
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

struct jobTableEntry *searchForFreeSpace(struct jobTableEntry *head, int requiredSize)
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
		cout << list->startingAddr << " "<<list->jobSize<<endl; 
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

