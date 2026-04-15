#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>

/* Declare and initialize the mutex and the condition variable
 * NOTE you can also initialize them using pthread_mutex_init()
 * and pthread_cond_init() functions discussed in class
 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* The condition flag */
int condition = 0;

/* The item produced */
int count = 0;


/**
 * The consumer function
 * @param arg - pointer to the thread local data - unused
 */
void* consume( void* arg )
{
	/* Consume things forever */
	while( 1 )
	{	
		/* Lock the mutex */
		if(pthread_mutex_lock( &mutex ) < 0)
		{
			perror("pthread_mutex_lock");
			exit(-1);
		}
		
		/* If there is nothing to consume, then sleep */	
		while( condition == 0 )
			
			/* Sleep on a condition variable until the
			 * producer wakes us up.
			 */
			if(pthread_cond_wait( &cond, &mutex ) < 0)
			{
				perror("pthread_cond_wait");
				exit(-1);
			}


		printf( "Consumed %d\n", count );
		
		/* Consume the item */
		condition = 0;
			
		/* Wake up the sleeping producer */	
		if(pthread_cond_signal( &cond ) < 0)
		{
			perror("pthread_cond_signal");
			exit(-1);
		}
		
		/* Unlock the mutex */
		if(pthread_mutex_unlock( &mutex ) < 0)
		{
			perror("pthread_mutex_unlock");
			exit(-1);
		}
	}

}

/**
 * The producer thread function
 * @param arg - pointer to the thread local data - unused
 */
void*  produce( void * arg )
{
	/* Produce things forever */
	while( 1 )
	{
		/* Lock the mutex to protect the condition variable */
		if(pthread_mutex_lock( &mutex ) < 0)
		{
			perror("pthread_mutex_lock");
			exit(-1);
		}
		
		/* I have produced something, that has not been
		 * yet consumed - sleep until the consumer 
		 * wakes us up.
		 */
		while( condition == 1 )
		{
			/* Sleep on a condition variable until the
			 * the producer wakes us up.
			 */
			if(pthread_cond_wait( &cond, &mutex ) < 0)
			{
				perror("pthread_cond_wait");
				exit(-1);
			}
		}

		/* Produce an item */
		printf( "Produced %d\n", count++ );
		
		/* I have produced something */
		condition = 1;
		
		/* Wake up the sleeping consumer */	
		if(pthread_cond_signal( &cond ) < 0)
		{
			perror("pthread_cond_signal");
			exit(-1);
		}
		
		/* Release the lock */
		if(pthread_mutex_unlock( &mutex ) < 0)
		{
			perror("pthread_mutex_unlock");
			exit(-1);
		}
	}

	return 0;
}

int main()
{
	/* The producer and consumer thread variables */
	pthread_t producerThread, consumerThread;

	/* Create a producer thread */
	if(pthread_create(&producerThread, NULL, &produce, NULL ) < 0)
	{
		perror("pthread_create");
		exit(-1);
	}

	/* Create a producer thread */
	if(pthread_create(&consumerThread, NULL, consume, NULL ) < 0)
	{
		perror("pthread_create");
		exit(-1);
	}
	
	/** These are included for completeness, since the threads
	  * run forever. Feel free to test these functions by making
	  * producer/consumer produce/consume a finite # of items
	  */

	/* Wait for the threads to exit (similar to the wait() system call for processes */
	if(pthread_join(producerThread, NULL) < 0)
	{
		perror("pthread_join");
		exit(-1);
	}

	if(pthread_join(consumerThread, NULL) < 0)
	{
		perror("pthread_join");
		exit(-1);
	}	

	return 0;
}
