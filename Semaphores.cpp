/******************************************************************************
                        PROGRAMMING ASSIGNMENT #3
Student 1:
Name: Balaji Varun Aditya
SID: 55304510

Student 2:
Name: Denny Thomas Varghese
SID: 55506653


*******************************************************************************/

#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include <unistd.h>
#include<iomanip>
#include<cmath>
#include <unistd.h>
#include<ctype.h>
#include<string.h>

using namespace std;



//implement the node of the queue
class Node
{
	double frame[8];
	Node *next;
	
	public:
	
	Node()												//constructor
	{
		next = NULL;	
	}
	
	void change_element(int i,double value)				//change the element at the ith position in the frame array.
	{
		frame[i] = value;
	}
	
	
	double get_value(int i)								//get the value at the ith position of the frame array			
	{
		return frame[i];	
	}
	
	
	void assign_next(Node *newnode)						//assign a pointer to next   
	{
		next = newnode;
	}
	
	Node* get_next()									//get the pointer next
	{
		return next;
	}
	
	double* get_frame()									//get the frame
	{
		return frame;	
	}	
};	



//implement the FIFO queue
class Queue
{
	Node *first;									//first node of queue
	int size;										//current size of queue
	Node *rear;										//last node of the queue
	
	public:
	Queue()
	{
		size=0;
		first = rear = NULL;		
	}
	
	int get_size()									//get the current size of the queue
	{
		return size;
	}
	
	Node* get_first()								//get the first node of the queue
	{
		return first;
	}
	
	Node *get_rear()								//get the last node of the queue
	{
		return rear;
	}
	
	void set_first(Node *newnode)					//assign a pointer to the firstnode of the queue
	{
		first = newnode;
	}
	
	void set_rear(Node *newnode)					//assign a node to the rear element of the queue 
	{
		rear = newnode;
	}
	
	void add_size()									//incerase the current size of the queue
	{
		size++;
	}
	
	void red_size()									//reduce the current size of the queue
	{
		size--;
	}
};


bool isvalid(string s)								//to check if the input is valid
{
	int i;
	for(i=0;i<s.size();i++)
	{
		if(!isdigit(s[i]))
		{
			return false;
		}
	}	
	return true;
}



//global variables
int l = 8;																		//length of the frame
pthread_t thread1,thread2,thread3;   											//ids of the 3 three threads
pthread_mutex_t lock_first = PTHREAD_MUTEX_INITIALIZER; 						//mutex for first element of cache	
pthread_mutex_t lock_rear = PTHREAD_MUTEX_INITIALIZER;							//mutex for last element of cache
pthread_mutex_t size_lock = PTHREAD_MUTEX_INITIALIZER;							//mutex for current size of cache
Queue *cache = new Queue();														//cache
int cache_size = 5;
sem_t camerasignal;																//to signal the camera thread
sem_t transformersignal;														//to signal the transformer thread
sem_t estimatorsignal;
sem_t restarttransformer;													    //to signal the estimator thread
double *temp_frame_recorder;													//temporary frame recorder
double temp_org[8];																//to temporarily store the original numbers
double* generate_frame_vector(int l);
bool is_done_cam = false;														//to check if the camera thread is done
bool is_done_trans = false;														//to check if the transformer thread is done



//camera thread
void *camera(void *interval)
{
	long long *t = (long long*)interval;
	long long time_interval = *t;
	while(true)
	{		       
		pthread_mutex_lock(&size_lock);											//lock the mutex for size
		if(cache->get_size()==cache_size)										//if the cache is full wait for estimators signal
		{
			pthread_mutex_unlock(&size_lock);									//unlock the mutex for size
			sem_wait(&camerasignal);				
		}
		
		pthread_mutex_unlock(&size_lock);										//unlock the mutex for size			
		double *newframe = generate_frame_vector(l);							
		if(newframe==NULL)														//if the newly generated frame is null then stop the thread
		{
			is_done_cam = true;
			pthread_exit(NULL);
		}
		else
		{
			Node *newnode = new Node();											
			for(int i=0;i<l;i++)
			{					
				newnode->change_element(i,*(newframe + i));							
			}
			
			pthread_mutex_lock(&lock_rear);										//lock the mutex for rear
			Node *temp_rear = cache->get_rear();
			if(temp_rear==NULL)													//if the queue is empty then change the first and rear element of node
			{
				pthread_mutex_lock(&lock_first);								//lock the mutex for first
				cache->set_first(newnode);						
				cache->set_rear(newnode);
				pthread_mutex_unlock(&lock_first);								//unlock the mutex for first
				pthread_mutex_lock(&size_lock);									//lock the mutex for size
				cache->add_size();
				pthread_mutex_unlock(&size_lock);								//unlock the mutex for size
			}
			else
			{
				temp_rear->assign_next(newnode);								
				cache->set_rear(newnode);
				pthread_mutex_lock(&size_lock);									//lock the mutex for size
				cache->add_size();
				pthread_mutex_unlock(&size_lock);								//unlock the mutex for size									
			}
			pthread_mutex_unlock(&lock_rear);									//unlock the mutex for size
			
						
		}
		sem_post(&transformersignal);											//send a signal to transformer thread
		sleep(time_interval);
	}
}



//transformer thread
void *transformer(void *t)
{
	while(true)
	{		
		sem_wait(&restarttransformer);
		sem_wait(&transformersignal);										//wait for a signal from transfomer
		sleep(3);
		pthread_mutex_lock(&lock_first);									//lock the mutex for first
		temp_frame_recorder = cache->get_first()->get_frame();		
		pthread_mutex_unlock(&lock_first);		
		for(int i=0;i<l;i++)												//compress and decompress every element
		{
			long double element = *(temp_frame_recorder +i);						
			temp_org[i] = element;			
			element = round(element*10.0);
			element = element/10.0;			
			*(temp_frame_recorder+i) = element;
		}				
		sem_post(&estimatorsignal);		
		if(is_done_cam && cache->get_size()==1)									//if we have reached the frame limit then exit the thread
		{
			is_done_trans = true;
			pthread_exit(NULL);
		}	
										
	}
}



//estimator thread
void *estimator(void *t)
{
		while(true)
		{		
			sem_wait(&estimatorsignal);					
			pthread_mutex_lock(&size_lock);			
			if(cache->get_size()==cache_size)											//if the cache is full then signal the camera after we are done displaying mse
			{
				pthread_mutex_unlock(&size_lock);				
				
				long double mse = 0.0;
				
				//calculate the mse
				for(int i=0;i<l;i++)
				{
					double compelement = *(temp_frame_recorder + i);											
					mse = mse + (temp_org[i]-compelement)*(temp_org[i] - compelement);			
				}
				
				cout<<setprecision(6)<<fixed<<"mse = "<<mse/l<<endl;	
			
				pthread_mutex_lock(&lock_first);									//lock the mutex for first			
				cache->set_first(cache->get_first()->get_next());			    	//if the queue has more than one element			
				pthread_mutex_unlock(&lock_first);									//unlock the mutex for first
				pthread_mutex_lock(&size_lock);										//lock the mutex for size		
				cache->red_size();													//reduce the size of the cache after processing
				pthread_mutex_unlock(&size_lock);									//unlock the mutex for size															
				
				if(is_done_trans)																		
				{
					pthread_exit(NULL);
				}
				sem_post(&camerasignal);
				sem_post(&restarttransformer);							
			}
			
			else
			{			
				pthread_mutex_unlock(&size_lock);
				long double mse = 0.0;
				
				//calculate the mse
				for(int i=0;i<l;i++)
				{
					double compelement = *(temp_frame_recorder + i);									
					mse = mse + (temp_org[i]-compelement)*(temp_org[i] - compelement);								
				}			
				cout<<setprecision(6)<<fixed<<"mse = "<<mse/l<<endl;				
				
				pthread_mutex_lock(&size_lock);		
				cache->red_size();													//reduce the size of the cache after processing
				pthread_mutex_unlock(&size_lock);
				pthread_mutex_lock(&lock_first);	 							     //lock the mutex for first						
				if(cache->get_first()->get_next()==NULL)							//if the queue has only one element set the rear and first to null
				{	
					pthread_mutex_lock(&lock_rear);									//lock the mutex for rear
					cache->set_rear(NULL);
					cache->set_first(NULL);
					pthread_mutex_unlock(&lock_rear);								//unlock the mutex for rear
				}
				else
				{	
					cache->set_first(cache->get_first()->get_next());				//if the queue has more than one element			
				}
				pthread_mutex_unlock(&lock_first);									//unlock the mutex for first
				
				
				if(is_done_trans)										//if sufficient number of frames have been generated then exit the thread
				{
					pthread_exit(NULL);
				}
				sem_post(&restarttransformer);								
			}   
		
	    }
}  



//main 
int main(int argc, char **argv)
{
	long long interval;	
	if(isvalid(argv[1])) 										//if the input is valid assgn input(seconds) to interval 
	interval = atoi(argv[1]);	
	
	else														//if the input is invalid disply message and exit program
	{
		cout<<"Input is invalid"<<endl;
		exit(0);
	}
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	
	//intializing semaphores
	sem_init(&camerasignal, 0, 0); 							
	sem_init(&transformersignal, 0, 0); 
	sem_init(&estimatorsignal, 0, 0);
	sem_init(&restarttransformer, 0, 1);
	
	  
	int rc1 = pthread_create(&thread1,&attr ,camera, &interval);	//create the camera thread 
	int rc2 = pthread_create(&thread2,&attr ,transformer, NULL);	//create the transformer thread
	int rc3 = pthread_create(&thread3,&attr ,estimator, NULL);		//create the estimator thread
	
		
	//error handling
	if(rc1)
	{
		cout<<"Error: Cannot create thread,"<< rc1<<endl;
	} 
	
	if(rc2)
	{
		cout<<"Error: Cannot create thread,"<< rc2<<endl;
	}
	
	if(rc3)
	{
		cout<<"Error: Cannot create thread,"<< rc3<<endl;
	}
	
	//these statements are used to ensure that the main function exits only after these threads exit.
	pthread_join(thread1,NULL);									
	pthread_join(thread2,NULL);
	pthread_join(thread3,NULL);	
	exit(0);		
}
 


