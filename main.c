#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3


int i = 0;
int j = 0;

//mutex lock for access to global variable
pthread_mutex_t mutex;

int initResourceVector [NUMBER_OF_RESOURCES];

//available, max, allocation, need
int available [NUMBER_OF_RESOURCES];
int allocation [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {{1,1,0},{1,3,0},{0,0,2},{0,1,1},{0,2,0}};
int maximum [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {{5,5,5},{3,3,6},{3,5,3},{7,1,4},{7,2,2}};
int need [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

int requestResource(int processID,int requestVector[]);
int releaseResource(int processID,int releaseVector[]);
int ifGreaterThanNeed(int processID,int requestVector[]);
int ifEnoughToRelease(int processID,int releaseVector[]);
int ifInSafeMode();
int ifEnoughToAlloc();
void printNeed();
void printAllocation();
void printAvailable();
void printReqOrRelVector(int vec[]);
void *customer(void* customer_num);

int main(int argc, char const *argv[])
{
	if(argc != NUMBER_OF_RESOURCES + 1)
	{
		printf("Quantity of parameter is not correct.\n");
		return -1;
	}
	for(i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		initResourceVector[i] = atoi(argv[i+1]);//argv[0] is name of program
		available[i] = initResourceVector[i];
	}

	//initialize need
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		for (j = 0; j < NUMBER_OF_RESOURCES; ++j)
		{
			need[i][j] = maximum[i][j] - allocation[i][j];
		}
	}

	printf("Available resources vector is:\n");
	printAvailable();

	printf("Initial allocation matrix is:\n");
	printAllocation();

	printf("Initial need matrix is:\n");
	printNeed();

	pthread_mutex_init(&mutex,NULL);//declared at head of code
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
		//lock mutex for accessing global variable and printf
		pthread_mutex_lock(&mutex);
		//initialize requestVector
		for(i = 0; i < NUMBER_OF_RESOURCES; i++)
		{
			if(need[processID][i] != 0)
			{
				requestVector[i] = rand() % need[processID][i];
			}
			else
			{
				requestVector[i] = 0;
			}
		}


		printf("Customer %d is trying to request resources:\n",processID);
		printReqOrRelVector(requestVector);
		//requestResource() will still return -1 when it fail and return 0 when succeed in allocate
		
		int requestSatisfaction=requestResource(processID,requestVector);
		
		//unlock
		pthread_mutex_unlock(&mutex);

		while(requestSatisfaction<0)
		{
			//if request is denied, then sleep for a random period less than 10 millisecond
			//then try again with the same request

			sleep(rand() % 10);
			requestSatisfaction=requestResource(processID,requestVector);
			
		}
		
	
		//since the request is granted, simulate run by sleeping for random time less than 100 millisecond
		//release random number of resources		
		sleep(rand() % 100);

		int releaseVector[NUMBER_OF_RESOURCES];
		//Because i is global variable, we should lock from here
		//lock mutex for accessing global variable and printf
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
		printf("Customer %d is trying to release resources:\n",processID);
		printReqOrRelVector(releaseVector);
		//releaseResource() will still return -1 when it fail and return 0 when succeed in allocate, like textbook says
		//altough I put the error message output part in the releaseResource function
		releaseResource(processID,releaseVector);
		//unlock
		pthread_mutex_unlock(&mutex);
	}
}

int requestResource(int processID,int requestVector[])
{
	//whether request number of resources is greater than needed
	if (ifGreaterThanNeed(processID,requestVector) == -1)
	{
		printf("requested resources is bigger than needed.\n");
		return -1;
	}
	printf("Requested resources are not more than needed.\nPretend to allocate...\n");

	//whether request number of resources is greater than needed
	if(ifEnoughToAlloc(requestVector) == -1)
	{
		printf("There is not enough resources for this process.\n");
		return -1;
	}

	//pretend allocated
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		need[processID][i] -= requestVector[i];
		allocation[processID][i] += requestVector[i];
		available[i] -= requestVector[i];
	}
	printf("Checking if it is still safe...\n");
	
	//check if still in safe status
	if (ifInSafeMode() == 0)
	{
		printf("Safe. Allocated successfully.\nNow available resources vector is:\n");
		printAvailable();
		printf("Now allocated matrix is:\n");
		printAllocation();
		printf("Now need matrix is:\n");
		printNeed();
		return 0;
	}
	else
	{
		printf("It is not safe. Rolling back.\n");
		for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
		{
			need[processID][i] += requestVector[i];
			allocation[processID][i] -= requestVector[i];
			available[i] += requestVector[i];
		}
		printf("Rolled back successfully.\n");
		return -1;
	}
}

int releaseResource(int processID,int releaseVector[])
{
	if(ifEnoughToRelease(processID,releaseVector) == -1)
	{
		printf("The process do not own enough resources to release.\n");
		return -1;
	}

	//enough to release
	for(i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		allocation[processID][i] -= releaseVector[i];
		need[processID][i] += releaseVector[i];
		available[i] += releaseVector[i];
	}
	printf("Release successfully.\nNow available resources vector is:\n");
	printAvailable();
	printf("Now allocated matrix is:\n");
	printAllocation();
	printf("Now need matrix is:\n");
	printNeed();
	return 0;
}

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

void printNeed()
{
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		printf("{ ");
		for (j = 0; j < NUMBER_OF_RESOURCES; ++j)
		{
			printf("%d, ", need[i][j]);
		}
		printf("}\n");
	}
	return;
}

void printAllocation()
{
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		printf("{ ");
		for (j = 0; j < NUMBER_OF_RESOURCES; ++j)
		{
			printf("%d, ", allocation[i][j]);
		}
		printf("}\n");
	}
	return;
}

void printAvailable()
{
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		printf("%d, ",available[i]);
	}
	printf("\n");
	return;
}

void printReqOrRelVector(int vec[])
{
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
	{
		printf("%d, ",vec[i]);
	}
	printf("\n");
	return;
}
int ifInSafeMode()
{
	int ifFinish[NUMBER_OF_CUSTOMERS] = {0};//there is no bool type in old C
	int work[NUMBER_OF_RESOURCES];//temporary available resources vector
	for(i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		work[i] = available[i];
	}
	int k;
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (ifFinish[i] == 0)
		{
			for(j = 0; j < NUMBER_OF_RESOURCES; j++)
			{
				if(need[i][j] <= work[j])
				{
					if(j == NUMBER_OF_RESOURCES - 1)//means we checked whole vector, so this process can execute
					{
						ifFinish[i] = 1;
						for (k = 0; k < NUMBER_OF_RESOURCES; ++k)
						{
							work[k] += allocation[i][k];
							//execute and release resources
						}
						//if we break here, it will not check all process, so we should reset i to let it check from beginning
						//If we cannot find any runnable process from beginning to the end in i loop, we can determine that
						//there is no any runnable process, but we cannot know if we do not reset i.
						i = -1;//at the end of this loop, i++, so -1++ = 0
						break;//in loop j, break to loop i and check next runnable process
					}
					else//not finished checking all resource, but this kind resources is enough
					{
						continue;
					}
				}
				else//resources not enough, break to loop i for next process
				{
					//because there is no change happened, so we do not need to reset i in this condition.
					break;
				}
			}
		}
		else
		{
			continue;
		}
	}
	//there are two condition if we finish loop i
	//1. there is no process can run in this condition.
	//2. all processes are runned, which means it is in safe status.
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (ifFinish[i] == 0)
		{
			//not all processes are runned, so it is condition 1.
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
