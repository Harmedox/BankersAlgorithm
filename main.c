#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3


int i = 0;
int j = 0;

//mutex lock for access to global variable
pthread_mutex_t mutex;

int initResource [NUMBER_OF_RESOURCES];

//available, max, allocation, need
int available [NUMBER_OF_RESOURCES];
int allocation [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int maximum [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

int requestResource(int processID,int requestVector[]);
int releaseResource(int processID,int releaseVector[]);
int ifGreaterThanNeed(int processID,int requestVector[]);
int ifEnoughToRelease(int processID,int releaseVector[]);
int ifInSafeMode();
int ifEnoughToAlloc();

void *customer(void* customer_num);

int main(int argc, char const *argv[])
{
	if(argc != NUMBER_OF_RESOURCES + 1)
	{
		printf("parameter list is not correct.\n");
		return -1;
	}
	for(i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		initResource[i] = atoi(argv[i+1]);
		available[i] = initResource[i];
		//printf("%d\n",available[i]);
	}


	//initialize maximum, allocation and need

	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		for (j = 0; j < NUMBER_OF_RESOURCES; ++j)
		{
			maximum[i][j] = rand() % (available[j] + 1);
			//initialize allocation to 0
			allocation[i][j] = 0;
			need[i][j] = maximum[i][j] - allocation[i][j];
		}
	}

	pthread_mutex_init(&mutex,NULL);
	pthread_attr_t attrDefault;
	pthread_attr_init(&attrDefault);
	pthread_t *tid = malloc(sizeof(pthread_t) * NUMBER_OF_CUSTOMERS);

	//generate customer number for banker's algorithm, different from pthread identifier
	int *pid = malloc(sizeof(int) * NUMBER_OF_CUSTOMERS);

	//initialize pid and create threads
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		*(pid + i) = i;
		pthread_create((tid+i), &attrDefault, customer, (pid+i));
	}
	//join threads
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		pthread_join(*(tid+i),NULL);
	}
	return 0;
}

void *customer(void* customer_num)
{
	int processID = *(int*)customer_num;

	while(1)
	{
		
		//request random number of resources
		sleep(1);
		int requestVector[NUMBER_OF_RESOURCES];

		//Because i is global variable, we should lock from here
		//lock mutex for accessing global variable 
		pthread_mutex_lock(&mutex);

		//initialize request
		int sumRequest = 0;
		while(sumRequest <= 0){
			for(i = 0; i < NUMBER_OF_RESOURCES; i++)
			{
				//requested resource cannot be greater than needed
				if(need[processID][i] != 0)
				{
					requestVector[i] = rand() % (need[processID][i] + 1);
				}
				else
				{
					requestVector[i] = 0;
				}
				sumRequest += requestVector[i];
			}

		}
		
		//requestResource() will still return -1 when it fail and return 0 when allocation is successful

		int requestSatisfaction=requestResource(processID,requestVector);
		
		//unlock
		pthread_mutex_unlock(&mutex);

		while(requestSatisfaction<0)
		{

			//display request rejection message
			//there are more than one reason for a request to be rejected which is why it is placed here rather than in the requestResource() function
			printf("Timestamp:[%u]; Customer: %d; request denied\n",(unsigned)time(NULL),processID);

			//if request is denied, then sleep for a random period less than 10 millisecond
			//then try again with the same request

			sleep(rand() % 10);
			requestSatisfaction=requestResource(processID,requestVector);
			
		}
		

		//display request acceptance message
		printf("Timestamp:[%u]; Customer: %d; request satisfied\n",(unsigned)time(NULL),processID);

		//since the request is granted, simulate run by sleeping for random time less than 100 millisecond
		//release random number of resources		
		sleep(rand() % 100);

		int releaseVector[NUMBER_OF_RESOURCES];
		//Because i is global variable, we should lock from here
		//lock mutex for accessing global variable 
		pthread_mutex_lock(&mutex);
		//initialize releaseVector
		for(i = 0; i < NUMBER_OF_RESOURCES; i++)
		{
			if(allocation[processID][i] != 0)
			{
				releaseVector[i] = rand() % allocation[processID][i];
			}
			else
			{
				releaseVector[i] = 0;
			}
		}

		//releaseResource() will still return -1 when it fail and return 0 when successful in allocation
		releaseResource(processID,releaseVector);
		//unlock
		pthread_mutex_unlock(&mutex);

		int sumNeed=0;
		while(sumNeed<=0){
			//reset maximum to a random value less than or equal to the initial resource for the customer
			for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
			{
				maximum[processID][i] = rand() % (initResource[i] + 1);
				//initialize/reset allocation to 0
				allocation[processID][i] = 0;
				need[processID][i] = maximum[processID][i] - allocation[processID][i];
				sumNeed += need[processID][i];
			}
		}
	}
}

int requestResource(int processID,int requestVector[])
{
	//print request message
	printf("Timestamp:[%u]; Customer: %d; ",(unsigned)time(NULL),processID);
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		printf("%d requested resource type %d",requestVector[i],i+1);
		if(i<NUMBER_OF_RESOURCES-1)
			printf(", ");
		else
			printf("\n");
	}

	//check if request number of resources is greater than needed
	if (ifGreaterThanNeed(processID,requestVector) == -1)
	{
		
		return -1;
	}
	
	//check if there is enough resources to allocate
	if(ifEnoughToAlloc(requestVector) == -1)
	{
		//there isnt enough resources for this process
		
		return -1;
	}

	//pretend allocated
	//update allocation with requested resource
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		need[processID][i] -= requestVector[i];
		allocation[processID][i] += requestVector[i];
		available[i] -= requestVector[i];
	}
	
	
	//check if still in safe status
	if (ifInSafeMode() == 0)
	{
		
		return 0;
	}
	else
	{
		//rollback since it is not safe
		for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
		{
			need[processID][i] += requestVector[i];
			allocation[processID][i] -= requestVector[i];
			available[i] += requestVector[i];
		}
		//printf("modebe");
		return -1;
	}
}

int releaseResource(int processID,int releaseVector[])
{
	if(ifEnoughToRelease(processID,releaseVector) == -1)
	{
		//the process do not own enough resources to release
		return -1;
	}

	//enough to release
	for(i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		allocation[processID][i] -= releaseVector[i];
		need[processID][i] += releaseVector[i];
		available[i] += releaseVector[i];
	}
	//display successful resource release message
	printf("Timestamp:[%u]; Customer: %d; resource released\n",(unsigned)time(NULL),processID);
	return 0;
}


//checks if the process own enough resources to release
int ifEnoughToRelease(int processID,int releaseVector[])
{
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		if (releaseVector[i] <= allocation[processID][i])
		{
			continue;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

//check if requested resource is greater than need
int ifGreaterThanNeed(int processID,int requestVector[])
{
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		if (requestVector[i] <= need[processID][i])
		{
			continue;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}


//check if available resource is enough to satisfy request
int ifEnoughToAlloc(int requestVector[])
{
	//first element of requestVector is processID
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		if (requestVector[i] <= available[i])
		{
			continue;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

//checks if current state is a safe state
int ifInSafeMode()
{
	int ifFinish[NUMBER_OF_CUSTOMERS] = {0};

	//temporary available resources
	int work[NUMBER_OF_RESOURCES];

	for(i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		work[i] = available[i];
		//printf("%d ",available[i]);
	}
	int k;
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (ifFinish[i] == 0)
		{
			for(j = 0; j < NUMBER_OF_RESOURCES; ++j)
			{
				if(need[i][j] <= work[j])
				{
					if(j == NUMBER_OF_RESOURCES - 1)//means the whole vector is checked
					{
						ifFinish[i] = 1;
						for (k = 0; k < NUMBER_OF_RESOURCES; ++k)
						{
							work[k] += allocation[i][k];
							//execute and release resources
						}
						//if we break here, it will not check all processes, so we should reset i to let it check from beginning
						//If we cannot find any runnable process from beginning to the end in i loop, we can determine that
						//there is no any runnable process, but we cannot know if we do not reset i.
						i = -1; //at the end of this loop, i++, so -1++ = 0
						break; //in loop j, break to loop i and check next runnable process
					}
					else //not finished checking all resource, but this kind of resources is enough
					{
						continue;
					}
				}
				else //resources not enough, break to loop i for next process
				{
					//because no change happened, so we do not need to reset i in this condition.
					break;
				}
			}
		}
		else
		{
			continue;
		}
	}
	//there are two conditions if we finish loop i
	//1. there is no process that can run in this condition.
	//2. all processes are running, which means it is in safe status.
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (ifFinish[i] == 0)
		{
			//not all processes are running, so it is condition 1.
			return -1;
		}
		else
		{
			continue;
		}
	}
	//finished loop, so it is condition 2
	return 0;
}
