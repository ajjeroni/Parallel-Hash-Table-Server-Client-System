/** This program illustrates the client end of the message queue **/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "msg.h"

/* Queue used for responses destined to this client */
int clientReplyQueueId = -1;

/**
 * Removes the private reply queue created by this client.
 */
void removePrivateQueue()
{
	if(clientReplyQueueId >= 0)
	{
		msgctl(clientReplyQueueId, IPC_RMID, NULL);
		clientReplyQueueId = -1;
	}
}

/**
 * Cleans up the private reply queue on Ctrl-C.
 * @param sig - unused signal number
 */
void cleanUp(int sig)
{
	(void)sig;
	exit(0);
}

int main()
{
	atexit(removePrivateQueue);
	signal(SIGINT, cleanUp);

	key_t key = ftok("/bin/ls", 'O');
	if(key < 0)
	{
		perror("ftok");
		exit(-1);
	}

	int msqid = connectToMessageQueue(key);

	clientReplyQueueId = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
	if(clientReplyQueueId < 0)
	{
		perror("msgget");
		exit(-1);
	}

	srand(static_cast<unsigned int>(time(NULL)) ^ static_cast<unsigned int>(getpid()));

	while(true)
	{
		message msg = {};

		msg.id = rand() % 1000;
		msg.replyQueueId = clientReplyQueueId;
		msg.messageType = CLIENT_TO_SERVER_MSG;

		sendMessage(msqid, msg);
		recvMessage(clientReplyQueueId, msg, SERVER_TO_CLIENT_MSG);

		if(msg.id != -1)
		{
			msg.print(stderr);
		}
	}

	return 0;
}
