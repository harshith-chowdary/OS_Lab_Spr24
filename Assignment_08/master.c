// To remove all ipcs use $ ipcrm -a
// To view all ipcs use $ ipcs, and -m for memory and -s for sems and -q for msgqs

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>

typedef struct {
    bool valid;
    int frame_number;
    int count;
} page_table_entry;

typedef struct {
    int current;
    int freelist[];
} free_list;

typedef struct {
    pid_t pid;
    int m;
    int frame_count;
    int frame_allocated;
} pcb;

#define PAGE_TABLE_KEY 1024
#define FREE_KEY 1025
#define READY_KEY 1026
#define PCB_KEY 1027
#define MSGQ2_KEY 1028
#define MSGQ3_KEY 1029

#define INVALID_PAGE_PROBABILITY 0.02

int k, m, f;

key_t pagetablekey, freekey, readykey, pcbkey;
key_t msgq2key, msgq3key;

bool flag = 0;

int pagetableid, freeid, readyid, pcbid;
int msgq2id, msgq3id;

void create_page_tables();
void create_free_list();
void create_pcb();
void create_queues();
void create_processes();

void clear_resources();

void sigusr1_handler(int sig);
void cexit(int status);

int pid, sch_pid, mmu_pid;

void sigusr1_handler(int sig) {
    sleep(2);

    kill(sch_pid, SIGTERM);
    kill(mmu_pid, SIGUSR2);
    sleep(2);

    flag = 1;
}

void cexit(int status) {
    clear_resources();
    
    printf("\nMaster: Cleared Resources !!!\n");
    printf("Master: Exiting !!!\n");

    exit(status);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    signal(SIGINT, cexit);
    signal(SIGUSR1, sigusr1_handler);

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <k> <m> <f>\n", argv[0]);
        cexit(1);
    }

    k = atoi(argv[1]);
    m = atoi(argv[2]);
    f = atoi(argv[3]);

    if (k < 1 || m < 1 || f < 1 || k > f) {
        fprintf(stderr, "Invalid arguments\n");
        cexit(1);
    }

    pid = getpid();

    printf("Master: Started => PID = %d\n", pid);

    create_queues();
    create_page_tables();
    create_free_list();
    create_pcb();

    usleep(1000);

    if((sch_pid = fork()) == 0) {
        char arg_k[20], arg_pid[20], arg_readyid[20], arg_msgq2id[20];

        sprintf(arg_k, "%d", k);
        sprintf(arg_pid, "%d", pid);
        sprintf(arg_readyid, "%d", readyid);
        sprintf(arg_msgq2id, "%d", msgq2id);

        execlp("./scheduler", "./scheduler", arg_k, arg_readyid, arg_msgq2id, arg_pid, NULL);

        printf("Master: Scheduler failed to exec\n");
        cexit(1);
    }

    if((mmu_pid = fork()) == 0) {
        char arg_k[20], arg_m[20], arg_pagetableid[20], arg_freeid[20], arg_pcbid[20], arg_msgq2id[20], arg_msgq3id[20];

        sprintf(arg_k, "%d", k);
        sprintf(arg_m, "%d", m);
        sprintf(arg_pagetableid, "%d", pagetableid);
        sprintf(arg_freeid, "%d", freeid);
        sprintf(arg_pcbid, "%d", pcbid);
        sprintf(arg_msgq2id, "%d", msgq2id);
        sprintf(arg_msgq3id, "%d", msgq3id);

        execlp("xterm", "xterm", "-T", "MMU", "-e", "./mmu", arg_k, arg_m, arg_pagetableid, arg_freeid, arg_pcbid, arg_msgq2id, arg_msgq3id, NULL);
        // execlp("./mmu", "./mmu", arg_k, arg_m, arg_pagetableid, arg_freeid, arg_pcbid, arg_msgq2id, arg_msgq3id, NULL);

        printf("Master: MMU failed to exec\n");
        cexit(1);
    }

    create_processes();

    if(!flag) {
        // sleep(10);
        pause();
    }

    sleep(60);

    clear_resources();

    return 0;
}

void create_page_tables () {
    pagetablekey = ftok("master.c", PAGE_TABLE_KEY);

    if ((pagetableid = shmget(pagetablekey, m * sizeof(page_table_entry) * k, IPC_CREAT | 0666 )) < 0) {
        perror("shmget");
        cexit(1);
    }

    page_table_entry *page_table;

    if((page_table = (page_table_entry *) shmat(pagetableid, NULL, 0)) < 0) {
        perror("shmat");
        cexit(1);
    }

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < k; j++) {
            page_table[i * k + j].valid = 0;
            page_table[i * k + j].frame_number = -1;
            page_table[i * k + j].count = 0;
        }
    }

    if(shmdt(page_table)) {
        perror("shmdt");
        cexit(1);
    }
}

void create_free_list () {
    freekey = ftok("master.c", FREE_KEY);

    if ((freeid = shmget(freekey, (f + 1) * sizeof(int), IPC_CREAT | 0666 )) < 0) {
        perror("shmget");
        cexit(1);
    }

    free_list *free_list_ptr;

    if((free_list_ptr = (free_list *) shmat(freeid, NULL, 0)) < 0) {
        perror("shmat");
        cexit(1);
    }

    free_list_ptr->current = f - 1;

    for (int i = 0; i < f; i++) {
        free_list_ptr->freelist[i] = i;
    }

    if(shmdt(free_list_ptr)) {
        perror("shmdt");
        cexit(1);
    }
}

void create_pcb () {
    pcbkey = ftok("master.c", PCB_KEY);

    if ((pcbid = shmget(pcbkey, sizeof(pcb), IPC_CREAT | 0666 )) < 0) {
        perror("shmget");
        cexit(1);
    }

    pcb *pcb_ptr;

    if((pcb_ptr = (pcb *) shmat(pcbid, NULL, 0)) < 0) {
        perror("shmat");
        cexit(1);
    }

    int sum_pages = 0;
    for (int i = 0; i < k; i++) {
        pcb_ptr[i].pid = -1;
        pcb_ptr[i].m = 1 + rand() % m;
        pcb_ptr[i].frame_count = 0;
        pcb_ptr[i].frame_allocated = 0;
        sum_pages += pcb_ptr[i].m;
    }

    int sum_allocated_frames = 0, maxi = 0, max = 0;
    for (int i = 0; i < k; i++) {
        int cur_allocated_frames = 1 + ((int) round((pcb_ptr[i].m * (f-k))/(float)sum_pages)) ;
        pcb_ptr[i].frame_count = cur_allocated_frames;
        sum_allocated_frames += cur_allocated_frames;

        if(pcb_ptr[i].m > maxi) {
            maxi = i;
            max = pcb_ptr[i].m;
        }
    }

    pcb_ptr[maxi].frame_count += f - sum_allocated_frames;

    if(shmdt(pcb_ptr)) {
        perror("shmdt");
        cexit(1);
    }
}

void create_queues () {
    readykey = ftok("master.c", READY_KEY);

    if ((readyid = msgget(readykey, IPC_CREAT | 0666 )) < 0) {
        perror("msgget");
        cexit(1);
    }

    msgq2key = ftok("master.c", MSGQ2_KEY);

    if ((msgq2id = msgget(msgq2key, IPC_CREAT | 0666 )) < 0) {
        perror("msgget");
        cexit(1);
    }

    msgq3key = ftok("master.c", MSGQ3_KEY);

    if ((msgq3id = msgget(msgq3key, IPC_CREAT | 0666 )) < 0) {
        perror("msgget");
        cexit(1);
    }
}

void create_processes () {
    pcb *pcb_ptr;

    if((pcb_ptr = (pcb *) shmat(pcbid, NULL, 0)) < 0) {
        perror("shmat");
        cexit(1);
    }

    for (int i = 0; i < k; i++) {

        int ref_str_len = 1 + 2 * pcb_ptr[i].m + rand() % (8 * pcb_ptr[i].m);
        char ref_str[m*20*40];

        printf("Master: ref_str_len = %d\n", ref_str_len);

        int off_set = 0;
        for (int j = 0; j < ref_str_len; j++) {
            int page = rand() % pcb_ptr[i].m;
            
            if((1+rand()%100)/100.0 < INVALID_PAGE_PROBABILITY) {
                page = pcb_ptr[i].m + rand() % (10 * m);
            }

            off_set += sprintf(ref_str + off_set, "%d|", page);
        }

        if (fork() == 0) {
            pcb_ptr[i].pid = getpid();
            
            char arg_i[20], arg_readyid[20], arg_msgq3id[20];

            sprintf(arg_i, "%d", i);
            sprintf(arg_readyid, "%d", readyid);
            sprintf(arg_msgq3id, "%d", msgq3id);

            printf("Master: Process %d has reference string %s\n", i, ref_str);

            execlp("./process", "./process", arg_i, arg_readyid, arg_msgq3id, ref_str, NULL);

            printf("Master: Process %d failed to exec\n", i);
            cexit(1);
        }

        usleep(250*1000);
    }

    if(shmdt(pcb_ptr)) {
        perror("shmdt");
        cexit(1);
    }
}

void clear_resources () {
    if (shmctl(pagetableid, IPC_RMID, NULL) < 0) {
        perror("shmctl");
        exit(1);
    }

    if (shmctl(freeid, IPC_RMID, NULL) < 0) {
        perror("shmctl");
        exit(1);
    }

    if (shmctl(pcbid, IPC_RMID, NULL) < 0) {
        perror("shmctl");
        exit(1);
    }

    if (msgctl(readyid, IPC_RMID, NULL) < 0) {
        perror("msgctl");
        exit(1);
    }

    if (msgctl(msgq2id, IPC_RMID, NULL) < 0) {
        perror("msgctl");
        exit(1);
    }

    if (msgctl(msgq3id, IPC_RMID, NULL) < 0) {
        perror("msgctl");
        exit(1);
    }
}