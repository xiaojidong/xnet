#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
/*
*	 c simple threadpool
*	 by xs
*/

typedef struct XNetTask {
	
	void (*handle)(void *task);
	void* userdata;
	struct XNetTask* next;
	
}XNetTask;


typedef struct XNetWorkThread {
	
	pthread_t tid;
	int terminate;
	struct XNetThreadPool* threadpool;
	int pthread_order;
	
}XNetWorkThread;

// task Queue
typedef struct LinkQueue {
	
	XNetTask* front;
	XNetTask* tail;
	int size;
	
}LinkQueue;

typedef LinkQueue* LinkQueuePtr;


typedef struct XNetThreadPool {
	
	XNetWorkThread* 	works;
	LinkQueue*			wait_task_queue;
	pthread_mutex_t 	mutex;
	pthread_cond_t  	cond;
	int					thread_num;
	
}XNetThreadPool;


int create_task_queue(LinkQueuePtr* queue) {
	
	LinkQueuePtr ptr  = ( LinkQueuePtr ) malloc( sizeof( LinkQueue ) );
	if ( !ptr ) return -1;
	
	ptr->front = ptr->tail = NULL;
	ptr->size = 0;
	
	*queue = ptr;
	
	return 0;
}

void destory_task_queue(LinkQueuePtr* queue) {
	
	if ( (*queue) ) {
		free ( *queue );
		*queue = NULL;
	}
	
}

void push_task( LinkQueue * q, XNetTask * e ) {
	if ( !q || !e ) return;
	e->next = NULL;
	if ( !(q->tail) ) {
		q->tail = q->front = e;
	} else {
		q->tail->next = e;
		q->tail = e;
	}
	q->size ++;
}

XNetTask* pop_task( LinkQueue* q ) {
	
	if ( !q || !(q->front) )
		return NULL;
	
	XNetTask *pop = q->front;
	q->front = q->front->next;
	(q->size) -- ;
	
	return pop;
}

int get_task_num( LinkQueue * q ) {	
	
	if ( !q )
		return 0;
	
	return q->size;
}



static void* WorkHandler( void * args ) {
	
	XNetWorkThread *worker = ( XNetWorkThread* ) args;
	if ( !worker ) return NULL;
	
	LinkQueue* q = worker->threadpool->wait_task_queue;
	if ( !q ) return NULL;
	
	
	while(1) {
		
		pthread_mutex_lock( &( worker->threadpool->mutex ) );
		
		XNetTask* task = NULL;
		while( !( task = pop_task( q ) ) ) {
			if ( worker->terminate )
				break;
			pthread_cond_wait( &(worker->threadpool->cond), &( worker->threadpool->mutex) );
			
		}
		if ( worker->terminate ){
			pthread_mutex_unlock( &( worker->threadpool->mutex ) );
			break;
		}
		
		pthread_mutex_unlock( &( worker->threadpool->mutex ) );
		
		task->handle(task);
		
	}

	pthread_exit(NULL);
	return NULL;
}


XNetThreadPool* XNetCreateThreadPool( int thread_size ) {
	
	XNetThreadPool* pool = malloc( sizeof( XNetThreadPool ) );
	if ( !pool ) return NULL;
	
	if ( thread_size < 0 || ! pool ) return NULL;
	
	pthread_mutex_init( &( pool->mutex ), NULL );
	pthread_cond_init( &( pool->cond ), NULL );
	pool->thread_num = thread_size;
	pool->works = calloc(thread_size, sizeof(XNetWorkThread));
	
	if ( create_task_queue( &( pool->wait_task_queue ) )) return NULL;
	if ( !(pool->works) )
	{
		free(pool);
		return NULL;
	}
	
	for ( int i = 0; i < thread_size; i ++ ) {
				
		int ret = pthread_create( &( pool->works[i].tid ), NULL, WorkHandler, &( pool->works[i] ) );
		if ( ret ) return NULL;
		
		(pool->works[ i ]).threadpool = pool;
		(pool->works[ i ]).pthread_order = i;
	}
	
	return pool;
	
}

void destroy_thread_pool(XNetThreadPool** pool) {

	if ( !pool ) return;
	
	destory_task_queue( &(*pool)->wait_task_queue );
	free( (*pool)->works );
	(*pool)->works = NULL;
	free( *pool );
	*pool = NULL;
}


int XNetPushTask( XNetThreadPool* threadPool, XNetTask * task ) {
	
	if ( !threadPool || !task ) return -1;
	
	pthread_mutex_lock( &(threadPool->mutex) );
	
	push_task(threadPool->wait_task_queue, task);
	
	pthread_cond_broadcast( &threadPool->cond );
		
	pthread_mutex_unlock( &(threadPool->mutex) );
	
	return 0;
}

void handler(void * args) {
	
	XNetTask* task = (XNetTask* ) args;
	if ( !task ) return ;
	printf("work handler : %d \n", *(int*)task->userdata);
	
	free(task->userdata);
	free(task);
	
	return;
}


int main(int argc, char **argv )
{
	
	XNetThreadPool *pool = XNetCreateThreadPool(10);
	if ( !pool ) return -1;
	
	for( int i = 0; i < 100000; i ++ ) {
		XNetTask * t = (XNetTask*)malloc( sizeof( XNetTask ) );
		if ( !t ) continue;
		t->userdata = malloc( sizeof(int) );
		*(int *)t->userdata  = i;
		t->handle = handler;
		XNetPushTask( pool, t );
	}
	
	
	getchar();
	

	return 0;
}







