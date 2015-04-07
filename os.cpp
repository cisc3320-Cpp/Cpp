#include <iostream>

using namespace std;

void printLinkedList();

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

	job *root;
	job *traverser;
	int listCounter;

// Allows initialization of (static) system variables declared above.
// Called once at start of the simulation.
void startup()
{
	listCounter = 0; 

	root = new job;		//creates a new node...or our first node which is at the begining
	root->next = NULL;	//tells what to point to 
	traverser = root;	//points to the current node
          
}

// INTERRUPT HANDLERS
// The following 5 functions are the interrupt handlers. The arguments
// passed from the environment are detailed with each function below.
// See RUNNING A JOB, below, for additional information


// Indicates the arrival of a new job on the drum.
void Crint (int &a, int p[])
{
	if (listCounter <50 ){	//test for maximum capacity
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
	printLinkedList();
}


void Dskint (int &a, int p[])
{
      // Disk interrupt.
      // At call : p [5] = current time
}

void Drmint (int &a, int p[])
{
      // Drum interrupt.
      // At call : p [5] = current time
}

void Tro (int &a, int p[])
{
     // Timer-Run-Out.
     // At call : p [5] = current time
}

void Svc(int &a, int p[])
{
     // Supervisor call from user program.
      // At call : p [5] = current time
      // a = 5 => job has terminated
      // a = 6 => job requests disk i/o
      // a = 7 => job wants to be blocked until all its pending
      // I/O requests are completed
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