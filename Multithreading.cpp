/*
NAME: BALAJI Varun Aditya, SID: 55304510, EID: avbalaji2
NAME: Denny-Thomas-Varghese, SID: 55506653, EID: dvarghese2  
OS Program A2
*/
#include <iostream>
#include <pthread.h>
#include <string>
#include<stdlib.h>
#include <unistd.h>
#include<iomanip>
using namespace std;



//QUEUE is implemented as class here.
class Cache
{
	private:
		double frame[8]; 										//store the flattened frame
		Cache *next;	
	
	public:
		Cache()
		{
			for(int i=0;i<8;i++)
			{
				frame[i] = 0;	
			}		
			next = NULL;
		}
			
		//change ith element of frame
		void change_element(int pos,double value)
		{
			frame[pos] = value;
		}
		
		//get the ith element of frame
		double get_element(int pos)
		{
			return frame[pos];
		}
		
		void display_all()
		{
			for(int i=0;i<8;i++)
			{
				cout<<fixed;
				cout<<setprecision(1);
				cout<<frame[i]<<" ";	
			}
		}
			
		//get the address of next element
		Cache* get_next()
		{
			return next;
		}
		
		//assign the address of the next element
		void assign_next(Cache* c)
		{
			next = c;
		}	
};



//global variables
pthread_t thread1,thread2;  
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 						//mutex
int cache_size = 5;        												//size of the cache
int max_num_frames = 10;  												//the maximum number of frames that can be generated 
Cache* cache = new Cache(); 											//queue to store the frames
int cache_ct = 0;           											//the number of elements in the cache at a partiuakr time 
int l = 8;



double* generate_frame_vector(int l);



//camera thread
void *camera(void *interval)
{
	long long *t = (long long*)interval;
	long long time_interval = *t;
	
	while(true)
	{
		double *new_frame;  											//store the generated frame		
		
		if(cache_ct>cache_size) 										//if the cache is full wait for interval seconds    
		{
			sleep(time_interval);
			continue;
		}
		
		else   															//if the cache is not full generate a vector and store it in the cache
		{
			new_frame= generate_frame_vector(l);
			if(new_frame == NULL)
			{
				pthread_exit(NULL);
			}
			else
			{
				//critical section
				pthread_mutex_lock(&lock);							//lock the mutex so that the quantiser function cannot use the cache
				Cache *temp = cache;								
				while(temp->get_next()!=NULL)
				{
					temp = temp->get_next();
				}
				Cache *newnode = new Cache();
				for(int i=0;i<l;i++)
				{
					
					newnode->change_element(i,*(new_frame + i));
				}
				
				if(cache_ct>0)
				temp->assign_next(newnode);
				else
				cache=newnode;
				cache_ct++;
				pthread_mutex_unlock(&lock);					//unlock the mutex so that quantsier thread can use that thread
				sleep(time_interval);					
			}
		}
	}
}



//quantiser thread 
void *quantizer(void *t)
{
	
	int exit_ct =0; 												//count when it reaches 3, stop the thread
	while(true)
	{
		if(exit_ct==3)   											//if the cache is empty check again. After 3 times,the thread stops
		{
			pthread_exit(NULL);
		}
		
		else if(cache_ct==0)
		{
			exit_ct++;
			sleep(1);
			continue;			
		}
		
		else
		{
			pthread_mutex_lock(&lock);								//lock the mutex so that the camera function cannot use the cache
			for(int i=0;i<l;i++)
			{
				if(cache->get_element(i)<=0.5)
				{
					cache->change_element(i,0.0);
				}
				else
				{
					cache->change_element(i,1.0);
				}	
			}
			cache->display_all();
			cout<<endl;
			Cache *temp = new Cache();
			if(cache_ct>1)
			{
				temp = cache->get_next();
				cache = temp;
			}
			cache_ct--;
			pthread_mutex_unlock(&lock);						//unlock the mutex so that the camera function can use the cache
			exit_ct=0;
			sleep(3);	
		}
			
	}
	
}



int main()
{
	long long interval; 										//interval in seconds
	cin>>interval;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	int rc1 = pthread_create(&thread1,&attr ,camera, &interval);	//create the camera thread 
	int rc2 = pthread_create(&thread2,&attr ,quantizer, NULL);		//create the quantiser thread
	
	if(rc1)
	{
		cout<<"Error: Cant create thread,"<< rc1<<endl;
	}
	
	if(rc2)
	{
		cout<<"Error: Cant create thread,"<< rc1<<endl;
	}
	
	//these statements are used to ensure that the main function exits only after these threads exit.
	pthread_join(thread1,NULL);									
	pthread_join(thread2,NULL);
	delete cache;
	exit(0);		
}









