#include <iostream>
#include <windows.h>
#include <time.h>
using namespace std;

class PO_Window
{
	int windowID;
	int currentQueueSize;
	int maxQueueSize;
	int workingSpeed;
	int clientsServed;
	HANDLE queueSemaphore;

public:
	PO_Window( int n, int cqs, int mqs, int ws, int cs = 0 ) :
		windowID( n ),
		currentQueueSize( cqs ),
		maxQueueSize( mqs ),
		workingSpeed( ws ),
		clientsServed( cs )
	{
		queueSemaphore = CreateSemaphore( NULL, 1, 1, NULL );
	}

	int getWaitingTime()
	{
		if( currentQueueSize == maxQueueSize ) return 99999999;
		return currentQueueSize * workingSpeed + workingSpeed;
	}

	HANDLE* getSemaphore()
	{
		return &queueSemaphore;
	}

	bool isFull()
	{
		return currentQueueSize == maxQueueSize;
	}

	void incrementQueue()
	{
		InterlockedIncrement( (LPLONG)&currentQueueSize );
	}

	void decrementQueue()
	{
		InterlockedIncrement( (LPLONG)&clientsServed );
		InterlockedDecrement( (LPLONG)&currentQueueSize );
	}

	int getClientsServed()
	{
		return clientsServed;
	}

	int getWorkingSpeed()
	{
		return workingSpeed;
	}
};

// ---------------globals------------------- 

DWORD WINAPI test_fun( LPVOID lpParam );
DWORD WINAPI waitingScreen( LPVOID lpParam );
int findShortestQueue();
int getRandomtime();
void printStats( clock_t timePassed );

HANDLE PO_Semaphore;
HANDLE entranceSemaphore;

PO_Window PO_Windows[3] = { PO_Window( 1, 0, 5, 300 ),
							PO_Window( 2, 0, 10, 150 ),
							PO_Window( 3, 0, 15, 100 ) };

// -----------main---------------
int main(int argc, char** argv)
{
	
	if( argc != 2 )
	{
		cout << "Argument not specified";
		return 1;
	}
	int clients = atoi( argv[1] );

	srand( (unsigned int)time( NULL ) );

	PO_Semaphore = CreateSemaphore( NULL, 10, 10, NULL );
	entranceSemaphore = CreateSemaphore( NULL, 1, 1, NULL );
	HANDLE* threads = new HANDLE[clients];

	int* waitingTimes = new int[clients];
	for( int i = 0; i < clients; i++ )
	{
		waitingTimes[i] = getRandomtime();
	}

	clock_t start = clock();

	CreateThread( NULL, 0, waitingScreen,NULL, 0, NULL );

	for( int i = 0; i < clients; i++ )
	{
		threads[i] = CreateThread( NULL, 0, test_fun,&waitingTimes[i], 0, NULL );
	}

	WaitForMultipleObjects( clients,threads,TRUE,INFINITE);

	clock_t stop = clock();

	printStats( stop - start );

	//-----------------cleanup---------------------------

	CloseHandle( PO_Semaphore );
	CloseHandle( entranceSemaphore );

	for( int i = 0; i < clients; i++ )
	{
		CloseHandle( threads[i] );
	}
	delete[] threads;
	delete[] waitingTimes;
	
}

DWORD WINAPI test_fun( LPVOID lpParam )
{
	WaitForSingleObject( PO_Semaphore, INFINITE ); // so only N clients can enter the building

		WaitForSingleObject( entranceSemaphore, INFINITE ); // so only one customers at a time chooses queue 
		int shortestQID = findShortestQueue();
		Sleep( *((int*)lpParam) );
		HANDLE* PO_WindowSemaphoreHandle = PO_Windows[shortestQID].getSemaphore();
		PO_Windows[shortestQID].incrementQueue();
		ReleaseSemaphore( entranceSemaphore, 1, NULL ); //exit queue choosing


		WaitForSingleObject( *PO_WindowSemaphoreHandle, INFINITE ); //Enter PO window
		//cout << "Client occupying for" << PO_Windows[shortestQID].getWorkingSpeed() << " at window " << shortestQID +1 << endl;
		Sleep( PO_Windows[shortestQID].getWorkingSpeed() );
		PO_Windows[shortestQID].decrementQueue();
		ReleaseSemaphore( *PO_WindowSemaphoreHandle, 1, NULL ); //exit PO window

	ReleaseSemaphore( PO_Semaphore, 1, NULL ); //exit building

	return NULL;
}

int getRandomtime()
{
	return rand() % 41 + 10;
}

int findShortestQueue()
{
	int windowID = 0;
	if( PO_Windows[0].getWaitingTime() > PO_Windows[1].getWaitingTime() ) windowID = 1;
	else if( PO_Windows[0].getWaitingTime() > PO_Windows[2].getWaitingTime() ) windowID = 2;

	return windowID;
}

void printStats( clock_t timePassed )
{
	system( "CLS" );
	cout << "Okienko 1 - obsluzono " << PO_Windows[0].getClientsServed() << " klientow." << endl;
	cout << "Okienko 2 - obsluzono " << PO_Windows[1].getClientsServed() << " klientow." << endl;
	cout << "Okienko 3 - obsluzono " << PO_Windows[2].getClientsServed() << " klientow." << endl;
	cout << "Calkowity czas dzialania - " << (double)timePassed / CLOCKS_PER_SEC << "s" << endl;
}

DWORD WINAPI waitingScreen( LPVOID lpParam )
{
	for( ;;)
	{
		cout << "Working";
		Sleep( 300 );
		cout << ".";
		Sleep( 300 );
		cout << ".";
		Sleep( 300 );
		cout << ".";
		Sleep( 300 );
		system( "CLS" );
	}
	return NULL;
}