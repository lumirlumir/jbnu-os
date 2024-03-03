/* Etc
    <할당한 메모리 공간을 다양한 포인터 형으로 캐스팅하여 사용하여야 함.>
    # 1 frame = 8 PTEs
    # pte* cur_pte = (pte*) &pas[frame_number];
    # pte cur_pte[8] 의 배열처럼 접근 가능하고, 혹은 cur_pte++로 순회 가능
*/

/* Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

/* Linked List */
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

/* Macro Declaration */
#define PAGESIZE (32)					//Page Size = Frame Size = 32B
#define PAS_FRAMES (256)                //fit for unsigned char frame in PTE
#define PAS_SIZE (PAGESIZE*PAS_FRAMES)  //32*256 = 8192 B
#define VAS_PAGES (64)					//number of VAS Pages
#define VAS_SIZE (PAGESIZE*VAS_PAGES)   //32*64 = 2048 B
#define PTE_SIZE (4)                    //sizeof(pte)
#define PAGETABLE_FRAMES (VAS_PAGES*PTE_SIZE/PAGESIZE) // 64*4/32 = 8 consecutive frames
#define PAGE_INVALID (0)
#define PAGE_VALID (1)
#define MAX_REFERENCES (256)

/* Struct */
typedef struct{
    unsigned char frame;    //allocated frame
    unsigned char vflag;    //valid-invalid bit
    unsigned char ref;      //reference bit
    unsigned char pad;      //padding
} pte; //Page Table Entry (total 4 Bytes, always)

typedef struct{
	int pid;
	int ref_len;
} process_buf; //Using it as Process Buffer.

typedef struct{
	/* Variables */
    int pid;
    int ref_len; 			//Less than 255
	int pc; 				//program counter
	/* Array */
    unsigned char *references;
	/* List */
    struct list_head pro;
} process;

typedef struct {
    unsigned char b[PAGESIZE];	//Size = 32B
} frame;

/* Function Declaration */
static inline process* create_process_node(struct list_head* head_job);			//process 구조체 node 생성. (head 앞쪽)		
static inline process* create_process_node_tail(struct list_head* head_job);	//process 구조체 node 생성. (head 뒷쪽)
static inline void load_process(struct list_head* head_job);	                //load processes from stdin.
static inline void terminate_process(struct list_head* head_job);				//free(terminate) processes.
static inline int count_existing_process(struct list_head* head_job);			//count existing processes num on pro_q.
static inline int count_ended_process(struct list_head* head_job);				//count ref ended processes num on pro_q.
static inline int count_l1pt_valid(frame *pas1, process *cur1); //count the number of valid in l1pt.
static inline int count_l2pt_valid(frame *pas1, process *cur1); //count the number of valid in l2pt.
static inline int count_references(frame *pas1, process* cur1);	//count the number of ref on each frames.

/* Main */
int main(int argc, char *argv[]) {
	/* Variables */
	bool is_oom = false; //'out of memory!' 발생 여부를 판단하는 flag.

	/* Load Processes */
    LIST_HEAD(pro_q); load_process(&pro_q); 
	process *cur; //순회 용도.

	/* PAS Creation */ 
	frame *pas = (frame*)calloc(PAS_FRAMES, sizeof(frame));	//Create Pysical Address Space. (Size=256)
	int pas_idx = 0; //Free Frame Index. (Max=255)

	/* L1PT Insertion */
	for(pas_idx; pas_idx < count_existing_process(&pro_q); pas_idx++) {
		pte* cur_pte = (pte*) &pas[pas_idx];
		for(int j = 0; j < PAGETABLE_FRAMES; j++)
			cur_pte[j].vflag = PAGE_INVALID;
	}
	
	/* Test */
	// list_for_each_entry_reverse(cur, &pro_q, pro) {
	// 	printf("%d %d\n", cur->pid, cur->ref_len);
	// 	for(int i=0;i<cur->ref_len;i++)
	// 		printf("%02d ", cur->references[i]);
	// 	printf("\n");
	// }

	/* Simulator */
	while(true) {
		/* Excution */
		list_for_each_entry_reverse(cur, &pro_q, pro) {
            /* Continue */
			if(cur->pc == cur->ref_len) continue;

            /* Test */
			//printf("[PID:%02d | REF:%03d] Page access %03d: ", cur->pid, cur->pc, cur->references[cur->pc]);

			/* Value Update */
            pte* cur_l1pt = (pte*) &pas[cur->pid];	//Level-1 Page Table의 주소값 설정.
            pte* cur_l2pt = (pte*) &pas[cur_l1pt[cur->references[cur->pc] / PAGETABLE_FRAMES].frame]; //Level-2 Page Table의 주소값 설정.
            int offset = (cur->references[cur->pc]) % 8; //offset 설정.

            /* L1PT */
            /* Case 1 : Abnormal (Page Fault) */
            if(cur_l1pt[cur->references[cur->pc] / PAGETABLE_FRAMES].vflag == PAGE_INVALID) {
                /* Test */
                //printf("(L1PT) PF, ");

                /* Exit if OOM */
                if(pas_idx >= PAS_FRAMES) {printf("Out of memory!!\n"); is_oom = true; break;}

                /* Test */
                //printf("Allocated Frame %03d -> %03d, ", cur->references[cur->pc] / PAGETABLE_FRAMES, pas_idx);

                /* Excution */
                cur_l1pt[cur->references[cur->pc] / PAGETABLE_FRAMES].frame = pas_idx;
                cur_l1pt[cur->references[cur->pc] / PAGETABLE_FRAMES].vflag = PAGE_VALID;
                pas_idx++;
            }
            /* Case 2 : Normal */
            else if(cur_l1pt[cur->references[cur->pc] / PAGETABLE_FRAMES].vflag == PAGE_VALID) {
                //printf("(L1PT) Frame %03d, ", cur_l1pt[cur->references[cur->pc] / PAGETABLE_FRAMES].frame);
            }
            /* Case 3 : Error */
            else printf("****L1PT Case Judgement Error!****\n");


            /* Value Update */
            cur_l1pt = (pte*) &pas[cur->pid];
            cur_l2pt = (pte*) &pas[cur_l1pt[cur->references[cur->pc] / PAGETABLE_FRAMES].frame];
            offset = (cur->references[cur->pc]) % 8;

            /* L2PT */
			/* Case 1 : Abnormal (Page Fault) */
			if(cur_l2pt[offset].vflag == PAGE_INVALID) {
				/* Test */
				//printf("(L2PT) PF, ");

                /* Exit if OOM */
                if(pas_idx >= PAS_FRAMES) {
                printf("Out of memory!!\n"); is_oom = true; break;}

                /* Test */
				//printf("Allocated Frame %03d\n", pas_idx);

                /* Excution */
				cur_l2pt[offset].frame = pas_idx;
				cur_l2pt[offset].vflag = PAGE_VALID;
				cur_l2pt[offset].ref ++;
				pas_idx++;
			}
			/* Case 2 : Normal */
			else if(cur_l2pt[offset].vflag == PAGE_VALID) {
				//printf("Frame %03d\n", cur_l2pt[offset].frame);
				cur_l2pt[offset].ref ++;
			}
			/* Case 3 : Error */
			else printf("****L2PT Case Judgement Error!****\n");

			/* Update PC */
			cur->pc++;			
		}

		/* Exit */
		if(is_oom == true) break;
		if(count_existing_process(&pro_q) == count_ended_process(&pro_q)) break;
	}

	/* Output */
	int sum_af = 0;
    int sum_pf = 0;
    int sum_ref = 0;
    int af, pf, ref;
	
	list_for_each_entry_reverse(cur, &pro_q, pro) {
		printf("** Process %03d: Allocated Frames=%03d PageFaults/References=%03d/%03d\n", 
			cur->pid, 
            af = 1 + count_l1pt_valid(pas, cur) + count_l2pt_valid(pas, cur), 
            pf = count_l1pt_valid(pas, cur) + count_l2pt_valid(pas, cur), 
            ref = count_references(pas, cur));

        pte* cur_l1pt = (pte*) &pas[cur->pid];

    	for(int i = 0; i < PAGETABLE_FRAMES; i++) {
            if(cur_l1pt[i].vflag == PAGE_VALID) {
                printf("(L1PT) %03d -> %03d\n", i, cur_l1pt[i].frame);

                pte* cur_l2pt = (pte*) &pas[cur_l1pt[i].frame];
                for(int j = 0; j < PAGETABLE_FRAMES; j++) {
                    if(cur_l2pt[j].vflag == PAGE_VALID)
                        printf("(L2PT) %03d -> %03d REF=%03d\n", i * PAGETABLE_FRAMES + j, cur_l2pt[j].frame, cur_l2pt[j].ref);
                }   
            }
        }

        sum_af += af;
	    sum_pf += pf;
		sum_ref += ref;
	}
	printf("Total: Allocated Frames=%03d Page Faults/References=%03d/%03d\n", sum_af, sum_pf, sum_ref);

    /* Free Processes */
    terminate_process(&pro_q);
	free(pas);

    return 0;
}

/* Function Definition */
static inline process* create_process_node(struct list_head* head_job) { 
	process* node = (process*)calloc(1, sizeof(process));
	if(node==NULL)
		return NULL;	//동적할당 실패시 NULL 반환.
	else
		list_add(&node->pro, head_job); return node;	//동적할당으로 만들어진 node 구조체 변수의 주소값 반환.
}	
static inline process* create_process_node_tail(struct list_head* head_job) { 	
	process* node = (process*)calloc(1, sizeof(process));
	if(node==NULL)
		return NULL;	//동적할당 실패시 NULL 반환.
	else
		list_add_tail(&node->pro, head_job); return node;	//동적할당으로 만들어진 node 구조체 변수의 주소값 반환.
}
static inline void load_process(struct list_head* head_job) {
	/* Create Buffer(Stack) */
	process_buf local_buf_proINF = { 0 }; //process 정보를 임시로 저장해둘 Buffer.
	unsigned char local_buf_refINF = 0;   //reference 정보를 임시로 저장해둘 Buffer.

	/* Read */
	while (fread(&local_buf_proINF, sizeof(process_buf), 1, stdin) == 1) {	//local_buf_proINF에 Process 정보 저장
		/* Create Node(Heap) */
		process* ptr_buf = create_process_node(head_job);					//head_job에 해당하는 queue '앞쪽=왼쪽'에 새로운 노드를 생성 후 이어붙임.
		ptr_buf->references = (unsigned char*)calloc(local_buf_proINF.ref_len, sizeof(unsigned char)); //references 정보를 담아둘 배열 선언.

		/* Copy : Buffer to Node (Buffer->Node) */
		ptr_buf->pid = local_buf_proINF.pid;
		ptr_buf->ref_len = local_buf_proINF.ref_len;

		for (int i = 0; i < (ptr_buf->ref_len); i++) {						
			fread(&local_buf_refINF, sizeof(unsigned char), 1, stdin);
			ptr_buf->references[i] = local_buf_refINF;
		}

		/* Set Other Properties */
		ptr_buf->pc = 0;

		/* Error Check */
		if (feof(stdin) != 0) break;      //파일의 끝에 도달한 경우
		else continue;
	}
}
static inline void terminate_process(struct list_head* head_job) {
	/* Test */
	//printf("terminate_process() start\n");

	/* Terminate */ // 메모리 할당 해제.
	process *cur, *next;
	list_for_each_entry_safe(cur, next, head_job, pro) {free(cur->references);free(cur);}

	/* Test */
	//printf("terminate_process() end\n");
}
static inline int count_existing_process(struct list_head* head_job){
	int count = 0;
	process *cur;
	list_for_each_entry(cur, head_job, pro) count++;
	return count;
}
static inline int count_ended_process(struct list_head* head_job){
	int count = 0;
	process *cur;
	list_for_each_entry(cur, head_job, pro)
		if(cur->pc == cur->ref_len) count++;	
	return count;
}
static inline int count_l1pt_valid(frame *pas1, process *cur1){
    pte* cur_l1pt = (pte*) &pas1[cur1->pid];
    int count = 0;

	for(int i = 0; i < PAGETABLE_FRAMES; i++)
		if(cur_l1pt[i].vflag == PAGE_VALID) count++;

	return count;
}
static inline int count_l2pt_valid(frame *pas1, process *cur1){
    pte* cur_l1pt = (pte*) &pas1[cur1->pid];
    int count = 0;

	for(int i = 0; i < PAGETABLE_FRAMES; i++) {
        if(cur_l1pt[i].vflag == PAGE_VALID) {
            pte* cur_l2pt = (pte*) &pas1[cur_l1pt[i].frame];
            for(int j = 0; j < PAGETABLE_FRAMES; j++) {
                if(cur_l2pt[j].vflag == PAGE_VALID) count++;
            }
        }
    }
		
	return count;
}
static inline int count_references(frame *pas1, process* cur1){
    pte* cur_l1pt = (pte*) &pas1[cur1->pid];
    int count = 0;

	for(int i = 0; i < PAGETABLE_FRAMES; i++) {
        if(cur_l1pt[i].vflag == PAGE_VALID) {
            pte* cur_l2pt = (pte*) &pas1[cur_l1pt[i].frame];
            for(int j = 0; j < PAGETABLE_FRAMES; j++) {
                if(cur_l2pt[j].ref != 0) count += cur_l2pt[j].ref;
            }
        }
    }
		
	return count;
}