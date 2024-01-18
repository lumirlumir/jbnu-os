#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
//=======================================C Header Files=======================================
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

struct list_head {
	struct list_head * next, * prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) } //초기화시 head는 prev와 next에 자기자신을 가리키고 있음.

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline void __list_add(struct list_head* new,
	struct list_head* prev,
	struct list_head* next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head* new, struct list_head* head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head* new, struct list_head* head) {
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head* prev, struct list_head* next) {
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head* entry) {
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline void list_del_init(struct list_head* entry) {
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static inline void list_move(struct list_head* list, struct list_head* head) {
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static inline void list_move_tail(struct list_head* list,
	struct list_head* head) {
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

static inline int list_empty(const struct list_head* head) {
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head);	\
       pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
		n = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#if 0    //DEBUG
#define debug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define debug(fmt, args...)
#endif
//=======================================Linked List(list_head)=======================================
typedef struct {        //buf 용도로 사용할 구조체
	int pid;            //ID.
	int arrival_time;   //도착시간.
	int code_bytes;     //코드길이(Byte).
} process_buf;

typedef struct {       	//node 용도로 사용할 구조체
	int pid;            //ID.
	int arrival_time;   //도착시간.
	int code_bytes;     //코드길이(Byte).
	unsigned char* operations; //code tuples가 저장된 위치.
	struct list_head job, ready, wait;
} process;
//=======================================structure declaration=======================================
static inline process* create_process_node(struct list_head* head_job);					//process 구조체 node 생성. (head 앞쪽)
static inline process* create_process_node_tail(struct list_head* head_job);			//process 구조체 node 생성. (head 뒷쪽)
static inline void load_process(struct list_head* head_job);							//stdin process load & head_job에 이어붙임.
static inline void load_process_buf_to_node_processINF(process_buf buf, process* node);	//buf to node cpy
static inline void terminate_process(struct list_head* head_job);						//job queue에 load된 process 동적할당 해제.
//=======================================queue functions declaration=======================================
LIST_HEAD(job_q); //초기화시 job_q 헤드는 prev와 next에 자기자신을 가리키고 있음.
LIST_HEAD(ready_q);
LIST_HEAD(wait_q);

int main(int argc, char* argv[]) {
	/* load process */
	load_process(&job_q);

	/* print output */
	process * cur;
	list_for_each_entry(cur, &job_q, job) {
		printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes);
		for(int i=0;i<cur->code_bytes/(2);i++)
			printf("%d %d\n", cur->operations[2*i], cur->operations[2*i+1]);
	}

	/* free process */
	terminate_process(&job_q);

	return 0;
}

static inline process* create_process_node(struct list_head* head_job) { //process 구조체 node 생성. (head 앞쪽=왼쪽)
	process* node = (process*)calloc(1, sizeof(process));
	if(node==NULL)
		return NULL;	//동적할당 실패시 NULL 반환.
	else
		list_add(&node->job, head_job); return node;	//동적할당으로 만들어진 node 구조체 변수의 주소값 반환.
}
static inline process* create_process_node_tail(struct list_head* head_job) { //process 구조체 node 생성. (head 뒷쪽=오른쪽)
	process* node = (process*)calloc(1, sizeof(process));
	if(node==NULL)
		return NULL;	//동적할당 실패시 NULL 반환.
	else
		list_add_tail(&node->job, head_job); return node;	//동적할당으로 만들어진 node 구조체 변수의 주소값 반환.
}
static inline void load_process(struct list_head* head_job) {
	/* Create Buffer(Stack) */
	process_buf local_buf_proINF = { 0 };          	//process 정보를 임시로 저장해둘 Buffer
	unsigned char local_buf_opINF[2] = { 0 };       //operation 정보를 임시로 저장해둘 Buffer([0]=Operation, [1]=Length)
	/*  <fread함수 설명>
		1. sizeof(process)는 12Byte 크기이다.
		2. sizeof(process)=12Byte 크기의 데이터 1개를 stdin으로부터
		   읽어 들여서 구조체변수 pro에 저장하라.
		3. fread함수는 실제로 읽어 들인 데이터의 갯수를 반환한다.
		   (읽어들인 바이트 수가 아니라 갯수이다.) 함수의 호출이 성공을
		   하고 요청한 분량의 데이터가 모두 읽혀지면 1이 반환된다. 
	*/
	while (fread(&local_buf_proINF, sizeof(process_buf), 1, stdin) == 1) {	//local_buf_proINF에 Process 정보 저장
		/* Create Node(Heap) */
		process* ptr_buf = create_process_node(head_job);					//head_job에 해당하는 queue '앞쪽=왼쪽'에 새로운 노드를 생성 후 이어붙임.
		ptr_buf->operations = (unsigned char*)calloc(local_buf_proINF.code_bytes, sizeof(unsigned char));	//operation 정보를 담아둘 배열 선언.

		/* Buffer to Node(Buffer->Node) */
		load_process_buf_to_node_processINF(local_buf_proINF, ptr_buf);		//버퍼에 있는 process 정보를 복사

		for (int i = 0; i < (local_buf_proINF.code_bytes) / (2); i++) {		//버퍼에 있는 op 정보를 복사. 코드길이는 짝수, 2로 나눠도 항상 정수이다.
			fread(local_buf_opINF, sizeof(unsigned char), 2, stdin);		//1Byte단위로 2개를 읽어들어야 한다.(cause, 1개의 Tuple = 2Byte)
			ptr_buf->operations[2 * i] = local_buf_opINF[0];				//[0]=Operation
			ptr_buf->operations[2 * i + 1] = local_buf_opINF[1];			//[1]=Length
		}

		/* Error Check */
		if (feof(stdin) != 0) break;      //파일의 끝에 도달한 경우
		else continue;
	}
}
static inline void load_process_buf_to_node_processINF(process_buf buf, process* node) {
	/* copy */
	node->pid = buf.pid;
	node->arrival_time = buf.arrival_time;
	node->code_bytes = buf.code_bytes;
}
static inline void terminate_process(struct list_head* head_job) {
	/* free */
	process *cur, *next;
	list_for_each_entry_safe(cur, next, head_job, job) {free(cur->operations);free(cur);} //메모리 할당 해제.
}