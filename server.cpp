/** This program illustrates the server end of the message queue **/
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <list>
#include <string>
#include <vector>

#include "msg.h"

using namespace std;

struct record
{
	/* The record id */
	int id;

	/* The first name */
	string firstName;

	/* The last name */
	string lastName;
};

/**
 * The structure of a hashtable cell
 */
class hashTableCell
{
public:
	hashTableCell()
	{
		pthread_mutex_init(&cellMutex, NULL);
	}

	~hashTableCell()
	{
		pthread_mutex_destroy(&cellMutex);
	}

	void lockCell()
	{
		pthread_mutex_lock(&cellMutex);
	}

	void unlockCell()
	{
		pthread_mutex_unlock(&cellMutex);
	}

	list<record> recordList;
	pthread_mutex_t cellMutex;
};

/* global variable for worker threads */
vector<pthread_t> workerThreads;

/* global variable for inserter threads */
vector<pthread_t> inserterThreads;

/* The number of cells in the hash table */
#define NUMBER_OF_HASH_CELLS 100

/* The number of inserter threads */
#define NUM_INSERTERS 5

/* The hash table */
vector<hashTableCell> hashTable(NUMBER_OF_HASH_CELLS);

/* The number of threads */
int numThreads = 0;

/* The shared request queue id */
int msqid = -1;

/* The ids that have yet to be looked up */
list<message> idsToLookUpList;

/* Protects idsToLookUpList */
pthread_mutex_t idsToLookUpListMutex = PTHREAD_MUTEX_INITIALIZER;

/* Used to implement the fetcher thread pool */
pthread_cond_t threadPoolCondVar = PTHREAD_COND_INITIALIZER;

/* Set by SIGINT so the server can shut down cleanly */
volatile sig_atomic_t shutdownRequested = 0;

/**
 * Prototype for createInserterThreads
 */
void createInserterThreads();

/**
 * A prototype for adding new records.
 */
void* addNewRecords(void* arg);

/**
 * A prototype for fetcher threads.
 */
void* threadPoolFunc(void* arg);

/**
 * Marks the server for shutdown when Ctrl-C is pressed.
 * @param sig - the signal
 */
void cleanUp(int sig)
{
	(void)sig;
	shutdownRequested = 1;
}

void installSignalHandler()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = cleanUp;
	sigemptyset(&sa.sa_mask);

	if(sigaction(SIGINT, &sa, NULL) < 0)
	{
		perror("sigaction");
		exit(-1);
	}
}

/**
 * Sends the message over the message queue
 * @param targetQueueId - the destination queue id
 * @param rec - the record to send
 */
void sendRecord(const int& targetQueueId, const record& rec)
{
	/* The message to send */
	message msg = {};

	/* Copy fields from the record into the message queue */
	msg.messageType = SERVER_TO_CLIENT_MSG;
	msg.id = rec.id;
	msg.replyQueueId = targetQueueId;
	snprintf(msg.firstName, MAX_NAME_LEN, "%s", rec.firstName.c_str());
	snprintf(msg.lastName, MAX_NAME_LEN, "%s", rec.lastName.c_str());

	/* Send the message */
	if(msgsnd(targetQueueId, &msg, sizeof(msg) - sizeof(long), 0) < 0)
	{
		if((errno != EIDRM) && (errno != EINVAL))
		{
			perror("msgsnd");
		}
	}
}

/**
 * Adds a record to hashtable
 * @param rec - the record to add
 */
void addToHashTable(const record& rec)
{
	int index = rec.id % NUMBER_OF_HASH_CELLS;

	hashTable.at(index).lockCell();
	hashTable.at(index).recordList.push_back(rec);
	hashTable.at(index).unlockCell();
}

/**
 * Looks up a record in the hashtable.
 * @param id the id of the record to retrieve
 * @return - the record from hashtable if exists;
 * otherwise returns a record with id field set to -1
 */
record getHashTableRecord(const int& id)
{
	hashTableCell* hashTableCellPtr = &hashTable.at(id % NUMBER_OF_HASH_CELLS);
	record rec = {-1, "", ""};

	hashTableCellPtr->lockCell();

	for(list<record>::iterator recIt = hashTableCellPtr->recordList.begin();
		recIt != hashTableCellPtr->recordList.end();
		++recIt)
	{
		if(recIt->id == id)
		{
			rec = *recIt;
			break;
		}
	}

	hashTableCellPtr->unlockCell();

	return rec;
}

/**
 * Loads the database into the hashtable
 * @param fileName - the file name
 * @return - the number of records left.
 */
int populateHashTable(const string& fileName)
{
	record rec;
	ifstream dbFile(fileName.c_str());

	if(!dbFile.is_open())
	{
		fprintf(stderr, "Could not open file %s\n", fileName.c_str());
		exit(-1);
	}

	while(dbFile >> rec.id >> rec.firstName >> rec.lastName)
	{
		addToHashTable(rec);
	}

	dbFile.close();
	return 0;
}

/**
 * Gets ids to process from work list
 * Caller must already hold idsToLookUpListMutex.
 * @return - the request to process, or
 * {-1, -1} if there is no work
 */
message getIdsToLookUp()
{
	message msg = {};
	msg.id = -1;
	msg.replyQueueId = -1;

	if(!idsToLookUpList.empty())
	{
		msg = idsToLookUpList.front();
		idsToLookUpList.pop_front();
	}

	return msg;
}

/**
 * Caller must already hold idsToLookUpListMutex.
 * Add a record lookup request to the work list.
 * @param msg - the request to process
 */
void addIdsToLookUp(const message& msg)
{
	idsToLookUpList.push_back(msg);
}

/**
 * The thread pool function
 * @param arg - unused
 */
void* threadPoolFunc(void* arg)
{
	(void)arg;

	while(true)
	{
		pthread_mutex_lock(&idsToLookUpListMutex);
		message msg = {};
		msg.id = -1;
		msg.replyQueueId = -1;

		while(idsToLookUpList.empty() && !shutdownRequested)
		{
			pthread_cond_wait(&threadPoolCondVar, &idsToLookUpListMutex);
		}

		if(!idsToLookUpList.empty())
		{
			msg = getIdsToLookUp();
		}

		pthread_mutex_unlock(&idsToLookUpListMutex);

		if((msg.id == -1) && shutdownRequested)
		{
			break;
		}

		record rec = getHashTableRecord(msg.id);
		sendRecord(msg.replyQueueId, rec);
	}

	return NULL;
}

/**
 * Wakes up a thread from the thread pool
 */
void wakeUpThread()
{
	pthread_cond_signal(&threadPoolCondVar);
}

/**
 * Creates the threads for looking up ids
 * @param numThreads - the number of threads to create
 */
void createThreads(const int& numThreads)
{
	workerThreads.resize(numThreads);

	for(int i = 0; i < numThreads; ++i)
	{
		if(pthread_create(&workerThreads[i], NULL, threadPoolFunc, NULL) != 0)
		{
			perror("pthread_create");
			exit(-1);
		}
	}
}

/**
 * Creates threads that update the database
 * with randomly generated records
 */
void createInserterThreads()
{
	inserterThreads.resize(NUM_INSERTERS);

	for(int i = 0; i < NUM_INSERTERS; ++i)
	{
		if(pthread_create(&inserterThreads[i], NULL, addNewRecords, NULL) != 0)
		{
			perror("pthread_create");
			exit(-1);
		}
	}
}

/**
 * Called by parent thread to process incoming messages
 */
void processIncomingMessages()
{
	message msg;

	while(!shutdownRequested)
	{
		memset(&msg, 0, sizeof(msg));

		if(msgrcv(msqid, &msg, sizeof(msg) - sizeof(long), CLIENT_TO_SERVER_MSG, 0) < 0)
		{
			if((errno == EINTR) && shutdownRequested)
			{
				break;
			}

			perror("msgrcv");
			shutdownRequested = 1;
			break;
		}

		pthread_mutex_lock(&idsToLookUpListMutex);
		addIdsToLookUp(msg);
		wakeUpThread();
		pthread_mutex_unlock(&idsToLookUpListMutex);
	}
}

/**
 * Generates a random record
 * @param seed - thread-local random seed
 * @return - a random record
 */
record generateRandomRecord(unsigned int& seed)
{
	record rec;
	rec.id = rand_r(&seed) % NUMBER_OF_HASH_CELLS;
	rec.firstName = "Random";
	rec.lastName = "Record";
	return rec;
}

/**
 * Threads inserting new records to the database
 * @param arg - some argument (unused)
 */
void* addNewRecords(void* arg)
{
	(void)arg;

	unsigned int seed = (unsigned int)time(NULL) + (unsigned int)pthread_self();

	while(!shutdownRequested)
	{
		record rec = generateRandomRecord(seed);
		addToHashTable(rec);
		usleep(5);
	}

	return NULL;
}

/**
 * Releases server resources and waits for threads to finish.
 */
void shutdownServer()
{
	shutdownRequested = 1;

	pthread_mutex_lock(&idsToLookUpListMutex);
	pthread_cond_broadcast(&threadPoolCondVar);
	pthread_mutex_unlock(&idsToLookUpListMutex);

	if(msqid >= 0)
	{
		if((msgctl(msqid, IPC_RMID, NULL) < 0) && (errno != EIDRM) && (errno != EINVAL))
		{
			perror("msgctl");
		}
		msqid = -1;
	}

	for(int i = 0; i < (int)workerThreads.size(); ++i)
	{
		pthread_join(workerThreads[i], NULL);
	}

	for(int i = 0; i < (int)inserterThreads.size(); ++i)
	{
		pthread_join(inserterThreads[i], NULL);
	}

	pthread_mutex_destroy(&idsToLookUpListMutex);
	pthread_cond_destroy(&threadPoolCondVar);
}

int main(int argc, char** argv)
{
	if(argc < 3)
	{
		fprintf(stderr, "USAGE: %s <DATABASE FILE NAME> <NUMBER OF THREADS>\n", argv[0]);
		exit(-1);
	}

	installSignalHandler();
	populateHashTable(argv[1]);

	numThreads = atoi(argv[2]);
	if(numThreads <= 0)
	{
		fprintf(stderr, "The number of threads must be positive.\n");
		exit(-1);
	}

	key_t key = ftok("/bin/ls", 'O');
	if(key < 0)
	{
		perror("ftok");
		exit(-1);
	}

	msqid = createMessageQueue(key);
	createThreads(numThreads);
	createInserterThreads();
	processIncomingMessages();
	shutdownServer();

	return 0;
}
