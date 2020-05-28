#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <malloc.h>

#define PTHREAD_NUM 2
#define DATA_NUM 100


typedef struct node_s node_t;
struct node_s
{
	struct node_s *prev;
	struct node_s *next;
	int data;
};

typedef struct list_s list_t;
struct list_s
{
	node_t *head;
	node_t *tail;
	int count = -1;
	sem_t mutex;
	sem_t mutex2;
	sem_t list_has_space;
	sem_t list_has_data;
};


list_t* list_init()
{
	list_t *list = (list_t*)malloc(sizeof(list_t));
	if(list == NULL){ printf("list init fail\n"); return NULL; }
	
	list->head = node_init(0);
	if(list->head == NULL){ printf("head node init fail\n"); list_destroy(list); return NULL; }
	list->tail = node_init(0);
	if(list->tail== NULL){ printf("taiol node init fail\n"); list_destroy(list); return NULL; }

	list->head->next = tail;
	list->head->prev = NULL;

	list->tail->next = NULL;
	list->tail->prev = head;
	
	// Initialize Semaphore
	if(sem_init(&(list->mutex), 0, 1) < 0){ perror("Fail to make mutex"); list_destroy(list); return NULL; }
	if(sem_init(&(list->mutex2), 0, 1) < 0){ perror("Fail to make mutex2"); list_destroy(list); return NULL; }
	if(sem_init(&(list->list_has_space), 0, 0) < 0){ perror("Fail to make list_has_space"); list_destroy(list); return NULL; }
	if(sem_init(&(list->list_has_data), 0, 0) < 0){ perror("Fail to make list_has_data"); list_destroy(list); return NULL; }
	
	return list;
}

void data_destroy(data_t *data)
{
	if(data->head)
	{
		node_destroy(data->head);
	}
	if(data->tail)
	{
		node_destroy(data->tail);
	}
	free(data);
}

node_t* node_init(int data)
{
	node_t *new_node = (node_t*)malloc(sizeof(node_t));
	if(new_node == NULL){ printf("node init fail\n"); return NULL; }
	new_node->data = data;
	return new_node;
}

void node_destroy(node_t *node)
{
	free(node);
}

int list_insert(list_t *list, int data)
{
	node_t *New = node_init(data);
	
	if(New == NULL){ return -1; }
	
	sem_wait(&(list->mutex2)); // critical section 방지 mutex2

	if(list->head->next == list->tail)
	{
		list->head->next = New;
		New->prev = list->head;
		New->next = list->tail;
		list->tail->prev = New;
	}
	else
	{
		list->tail->prev->next = New;
		New->prev = list->tail->prev;
		New->next = list->tail;
		list->tail->prev = New;
	}

	sem_post(&(list->mutex2));
	return 1;
}

int list_delete(list_t *list, node_t *target)
{
	sem_wait(&(list->mutex2)); // critical section 방지 mutex2

	if(target->next == NULL){ return -1; }

	node_t *del = target;

	del->prev->next = del->next;
	del->next->prev = del->prev;

	node_destroy(del);

	sem_post(&(list->mutex2)); // critical section 방지 mutex2
	return 1;
}


void* P(void* args) // 데이터를 최대 100개까지 Linked list에 축적하는 함수
{	
	int i, is_ins_fail;
	list_t *list = *(*args);

	for(i=0; i<1000; i++)
	{
		if(list->count == DATA_NUM)
		{
			//printf("List is fulled %d\n", i);
			sem_wait(&(list->list_has_space)); // 데이터가 100개로 가득차서 기다림
		}
		sem_wait(&(list->mutex)); // critical section 방지 mutex
		is_ins_fail = list_insert(list, i); // Linked list에 데이터 추가
		if(is_ins_fail == -1){ printf("insert fail\n"); break; }
		(list->count)++;
		sem_post(&(list->list_has_data)); // 데이터가 생겼다고 consumer에 신호 보냄
		sem_post(&(list->mutex)); // critical section 방지 mutex
	}
	pthread_exit((void*)0); // producer를 돌리는 Thread 종료
}

void* C(void* args) // Linked list에서 0개가 될 때까지 데이터를 제거해서 출력하는데 사용하는 함수
{
	int i, data, is_del_fail;
	list_t *list = *(*args);
	node_t* Now = list->head;

	for(i=0; i<1000; i++)
	{
		if(list->count == 0)
		{
			//printf("List is empty %d\n", i);
			Now = list->head;
			sem_wait(&(list->list_has_data)); // 꺼낼 데이터가 없으니까 기다림
		}
		sem_wait(&(list->mutex));
		Now = Now->next;
		data = Now->data;
		is_del_fail = list_delete(list, Now); // Linked list에서 데이터 삭제
		if(is_del_fail == -1){ printf("delete fail\n"); break; }
		(list->count)--;
		sem_post(&(list->mutex));
		sem_post(&(list->list_has_space)); // 데이터 공간이 생겼다고 producer에 신호 보냄
		printf("%d\n", data);
	}
	pthread_exit((void*)0); // consumer를 돌리는 Thread 종료
}


int  main()
{
	list_t *list = list_init();
	if(list == NULL){
		printf("Program error, list init fail\n");
		exit(-1);
	}
	
	int p_id[PTHREAD_NUM];
	pthread_t td[PTHREAD_NUM];
	if((p_id[0] = pthread_create(&td[0], NULL, P, (void*)(list))) < 0){
		perror("Fail to create Producer thread!"); exit(-1); }
	if((p_id[1] = pthread_create(&td[1], NULL, C, (void*)(list))) < 0){
		perror("Fail to create Consumer thread!"); exit(-1); }
	for(int i = 0; i < PTHREAD_NUM; i++){ pthread_join(td[i], NULL); }
	
	list_destroy(list);
}

