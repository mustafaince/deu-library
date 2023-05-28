#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

int sArray[10][4]; // for keeping values of rooms
pthread_mutex_t lock; // for a small control
pthread_t students[100];
pthread_t roomKeepers[10];
sem_t cleanKeeper[10]; // for getting keeper start to cleaning
sem_t controlKeeper[10]; // for getting keeper open the working room
sem_t fullState[10]; // for exit state when room is full
sem_t rooms[10];
sem_t canPass; // Can students enter to anyroom.(is there anyroom available)
int roomStates[10]; // state of rooms	0: empty	1: not full or empty	2: full  and number of students
int lastUsedRoom=-1; // which room is fulled last
int stateControl=0; // is there any room that's state is 1
int allDone=0; // for control all students are serviced
int counter=0; // for count students are serviced

void printRooms()
{
	printf("\n");
	int i;
	for(i=0;i<10;i++) 
	{
        printf("Room(%d)[%d ,%d, %d, %d]   ", i+1, sArray[i][0]+1, sArray[i][1]+1, sArray[i][2]+1, sArray[i][3]+1);
    }
	printf("\n\n");
}

void controlState()
{
	int j;
	stateControl=0;
	for(j=0; j<10; j++)
	{
		if(roomStates[j]==1)
		{
			stateControl= 1;
			break;
		}
	}
}

int whichRoom()
{
	int i, j;
	int room=0;
	controlState();
	for(i=0;i<10;i++) 
	{
		if(stateControl== 0) // so all rooms are empty or full. So we should find the one empty room
		{
			i= (lastUsedRoom+1)%10;
			for(i=i%10; 1==1; i++)
			{
				if(roomStates[i]==0)
				{
					return i;
					break;
				}
			}
			
			
			room= (lastUsedRoom+1)%10;
		}
        else // there is only one room that's state is 1. So we should find it.
		{
			for(j=0; j<10; j++)
			{
				if(roomStates[j]==1)
				{
					room= j;
					break;
				}
			}
		}
    }
    
    return room;
}

void roomKeeper(void* keeper_Id)
{
	int i;
	int keeperId = (int)keeper_Id;
	
	while(allDone==0)
	{
		printf("RoomKeeper %d is cleaning\n", keeperId+1);
		for(i=0;i<4;i++) 
		{
			sem_wait(controlKeeper+keeperId); // when a student get into room, this gate is opened
        	if(i==0)
        	{
        		printf("RoomKeeper %d opened the working room \n", keeperId+1);
        		printf("RoomKeeper %d: The last %d students, let's get up! \n", keeperId+1, 3-i);
			}
     		else if(i==3)
        		printf("RoomKeeper %d: room is full \n", keeperId+1);
      		else
        		printf("RoomKeeper %d: The last %d students, let's get up! \n", keeperId+1, 3-i);
    	}
    	sem_wait(cleanKeeper+keeperId); // when room is full, start cleaning
	}
}

void student(void* studentId)
{
	int value;
	int rnd;
	int roomId;
	int id = (int)studentId;
	int i;
	
	rnd = (rand() % 100) /100;
	sleep(rnd);
	pthread_mutex_lock(&lock); // for locking critical area
	printf("Student %d came to library and is waiting for room\n", id+1);
	sem_wait(&canPass); // is there any available room
	
	roomId= whichRoom();
	sem_wait(rooms+roomId);
	counter++; // for count students who are serviced
	printf("Student %d got into room %d\n", id+1, roomId+1);
	sem_post(controlKeeper+roomId); // roomKeeper is triggered
	sem_getvalue(rooms+roomId, &value);
	if(value==3)
		roomStates[roomId]=1;
	sArray[roomId][3-value]= id;
	printRooms();
	
	if(value ==0)
	{
		lastUsedRoom= roomId;
		roomStates[roomId]=2;
		for(i=0;i<4;i++) 
		{
        	sem_post(fullState+roomId);
    	}
	}
	pthread_mutex_unlock(&lock);
	sem_wait(fullState+roomId); //when room is full, leave from library
	
	sleep(1);
	printf("Student %d is leaving from room %d\n", id+1, roomId+1);
	sem_post(rooms+roomId);
	
	if(counter== 100)
	{
		allDone=1;
	} 
	
	sem_getvalue(rooms+roomId, &value);
	if(value == 4)
	{
		roomStates[roomId]=0;
		sem_post(cleanKeeper+roomId);
		for(i=0;i<4;i++) 
		{
			sArray[roomId][i]= -2;
        	sem_post(&canPass);
    	}
	}
	if(allDone==1) // finish all students
	{
		for(i=0;i<10;i++) 
	    {
            pthread_cancel(roomKeepers[i]); // kill all threads
        }
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) 
{
	int i, j;
	
	for(i=0;i<10;i++) 
	{
		for(j=0;j<4;j++) 
		{
        	sArray[i][j]= -2;
    	}
    }
    
    for(i=0;i<10;i++) 
	{
	    roomStates[i]= 0;
    }
	
	for(i=0;i<10;i++) 
	{
        sem_init(controlKeeper+i, 0, 4);
    }
    for(i=0;i<10;i++) 
	{
        sem_init(cleanKeeper+i, 0, 1);
    }
	for(i=0;i<10;i++) 
	{
        sem_init(rooms+i, 0, 4);
    }
    for(i=0;i<10;i++) 
	{
        sem_init(fullState+i, 0, 4);
    }
    for(i=0;i<10;i++) 
	{
		for(j=0;j<4;j++) 
		{
        	sem_wait(fullState+i); // first time all rooms is empty
    	}
    }
    for(i=0;i<10;i++) 
	{
		for(j=0;j<4;j++) 
		{
        	sem_wait(controlKeeper+i); // first time all keeper is cleaning
    	}
    }
    for(i=0;i<10;i++) 
	{
        sem_wait(cleanKeeper+i);
    }
    sem_init(&canPass, 0, 40);
    pthread_mutex_init(&lock, NULL);
	
	for(i=0;i<100;i++) 
	{
        pthread_create(&students[i], NULL, student, (void*)i);
    }
    for(i=0;i<10;i++) 
	{
        pthread_create(&roomKeepers[i], NULL, roomKeeper, (void*)i);
    }

    for(i=0;i<100;i++) 
	{
        pthread_join(students[i], NULL);
    }
    for(i=0;i<10;i++) 
	{
        pthread_join(roomKeepers[i], NULL);
    }
    
    for(i=0;i<10;i++) 
	{
        printf("RoomKeeper %d is leaving from library\n", i+1);
    }
    
    printRooms();
    
    for(i=0;i<10;i++) 
	{
        sem_destroy(controlKeeper+i);
    }
    for(i=0;i<10;i++) 
	{
        sem_destroy(cleanKeeper+i);
    }
    for(i=0;i<10;i++) 
	{
        sem_destroy(rooms+i);
    }
    for(i=0;i<10;i++) 
	{
        sem_destroy(fullState+i);
    }
    sem_destroy(&canPass);
    pthread_mutex_destroy(&lock);

	return 0;
}