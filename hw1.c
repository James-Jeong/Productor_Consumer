#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <semaphore.h>
#include <stdlib.h>

#define PTHREAD_NUM 2
#define DATA_NUM 100

sem_t mutex, list_has_space, list_has_data;

int count = 0;

struct Node
{
	struct Node* prev;
	struct Node* next;
	int data;
};

struct Node* head;
struct Node* tail;

void InitL()
{
	head = (struct Node*)malloc(sizeof(struct Node));
	tail = (struct Node*)malloc(sizeof(struct Node));	

	head->next = tail;
	head->prev = NULL;

	tail->next = NULL;
	tail->prev = head;
}

struct Node* insertN(struct Node* target, struct Node* temp)
{
	struct Node* New;
	New = (struct Node*)malloc(sizeof(struct Node));
	*New = *temp;

	if(head->next == tail)
	{
		head->next = New;
		New->prev = head;
		New->next = tail;
		tail->prev = New;
	}
	else
	{
		tail->prev->next = New;
		New->prev = tail->prev;
		New->next = tail;
		tail->prev = New;
	}
	return New;
}

int deleteN(struct Node *target)
{
	if(target->next == NULL)
		return -1;

	struct Node* del;
	del = (struct Node*)malloc(sizeof(struct Node));
	del = target;

	del->prev->next = del->next;
	del->next->prev = del->prev;

	free(del);
	return 1;
}


void* P(void* args) // 데이터를 최대 100개까지 Linked list에 축적하는 함수
{	
	int i;
	struct Node* target;
	struct Node temp;
	target = head;

	for(i=1; i<=1000; i++)
	{

		if(count > DATA_NUM)
		{
			target = tail;
			//printf("List is fulled %d\n", i);
			sem_wait(&list_has_space);
		}
		if(count <= DATA_NUM){
			sem_wait(&mutex);
			count++;
			temp.data = i;
			target = insertN(target, &temp); // Linked list에 데이터 추가
			//printf("P produce %d\n", i);
			sem_post(&list_has_data);
			sem_post(&mutex);
		}
	}
	pthread_exit((void*)0); // producer를 돌리는 Thread 종료
}

void* C(void* args) // Linked list에서 0개가 될 때까지 데이터를 제거해서 출력하는데 사용하는 함수
{
	int i, data;
	struct Node* Now;
	Now = head;

	for(i=1; i<=1000; i++)
	{
		if(count <= 0)
		{
			Now = head;
			//printf("List is empty %d\n", i);
			sem_wait(&list_has_data);
		}
		if(count > 0){
			sem_wait(&mutex);
			count--;
			Now = Now->next;
			data = Now->data;
			deleteN(Now); // Linked list에서 데이터 삭제
			//printf("C consume %d\n", data);
			printf("%d\n", data);
			sem_post(&list_has_space);
			sem_post(&mutex);
		}
	}
	pthread_exit((void*)0); // consumer를 돌리는 Thread 종료
}


int main(){
	InitL();

	// Initialize Semaphore
	if(sem_init(&mutex, 0, 1) < 0){ perror("Fail to make mutex"); exit(-1); }
	if(sem_init(&list_has_space, 0, 0) < 0){ perror("Fail to make list_has_space"); exit(-1); }
	if(sem_init(&list_has_data, 0, 0) < 0){ perror("Fail to make list_has_data"); exit(-1); }

	int p_id[PTHREAD_NUM];
	pthread_t td[PTHREAD_NUM];
	if((p_id[0] = pthread_create(&td[0], NULL, P, NULL)) < 0){
		perror("Fail to create Producer thread!"); exit(-1); }
	if((p_id[1] = pthread_create(&td[1], NULL, C, NULL)) < 0){
		perror("Fail to create Consumer thread!"); exit(-1); }
	for(int i = 0; i < PTHREAD_NUM; i++){ pthread_join(td[i], NULL); }
}
