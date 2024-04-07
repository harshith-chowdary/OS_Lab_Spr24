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

#define FROM 10
#define TO 20
#define PROCESS_TERM 30

#define PAGE_FAULT_HANDLED 40

#define PAGE_FAULT -1
#define INVALID_PAGE_NUMBER -2
#define PROCESS_DONE -3

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

struct msg {
    long msg_type;
    int id;
    char page_number;
};

struct msgtoproc {
    long msg_type;
    int frame_number;
};

struct msgtosch {
    long msg_type;
    char msg_buf[1];
};

int pagetableid, freeid, readyid, pcbid;
int msgq2id, msgq3id;

int k, m, f;

int read_request(int * id);
void send_reply(int id, int frame_number);

void notify_scheduler(int type);

int page_fault_handler(int id, int page_number);
void free_pages(int id);

int * page_faults;
FILE * result;
int time_stamp = 0;

int terminated = 0;

pcb * pcb_ptr;
page_table_entry * page_table_ptr;
free_list * free_list_ptr;

int prev = -1;

void serve_request() {
    pcb_ptr = (pcb *) shmat(pcbid, NULL, 0);
    page_table_ptr = (page_table_entry *) shmat(pagetableid, NULL, 0);
    free_list_ptr = (free_list *) shmat(freeid, NULL, 0);

    int id = -1;
  
    int page_number = read_request(&id);

    if(id!=prev) {
        printf("MMU: Request => Process ID: %d --------------------\n", id);
        fprintf(result, "MMU: Request => Process ID: %d --------------------\n", id);

        prev = id;
    }

    if(page_number == -1 && id == -1) {
        shmdt(pcb_ptr);
        shmdt(page_table_ptr);
        shmdt(free_list_ptr);
        return;
    }

    if(page_number == PROCESS_DONE) {
        free_pages(id);

        printf("MMU: Process %d Done, Notifying Sched\n", id);
        notify_scheduler(PROCESS_TERM);

        terminated++;

        return;
    }

    time_stamp++;

    printf("MMU: Request => Process ID: %d, Page Number: %d, Time Stamp: %d\n", id, page_number, time_stamp);
    // fprintf(result, "MMU: Request => Process ID: %d, Page Number: %d, Count: %d\n", id, page_number, time_stamp);
            fprintf(result, "Page Reference : ( %d, %d, %d )\n", time_stamp, id, page_number);

    if(page_number < 0 || pcb_ptr[id].m < page_number) {
        printf("MMU: INVALID Page Number => Process ID: %d, Page Number: %d\n", id, page_number);
        // fprintf(result, "MMU: Invalid Page Number => Process ID: %d, Page Number: %d\n", id, page_number);
        fprintf(result, "INVALID Page Reference : ( %d, %d )\n\n", id, page_number);

        send_reply(id, INVALID_PAGE_NUMBER);

        free_pages(id);
        notify_scheduler(PROCESS_TERM);
    }
    else {
        if (!page_table_ptr[id * m + page_number].valid) {
            printf("MMU: PAGE FAULT => Process ID: %d, Page Number: %d\n", id, page_number);
            // fprintf(result, "MMU: Page Fault => Process ID: %d, Page Number: %d\n", id, page_number);
            fprintf(result, "PAGE FAULT     : ( %d, %d )\n", id, page_number);

            page_faults[id]++;

            send_reply(id, -1);

            int frame_number = page_fault_handler(id, page_number);

            page_table_ptr[id * m + page_number].valid = 1;
            page_table_ptr[id * m + page_number].frame_number = frame_number;
            page_table_ptr[id * m + page_number].count = time_stamp;

            notify_scheduler(PAGE_FAULT_HANDLED);
        }
        else{
            send_reply(id, page_table_ptr[id * m + page_number].frame_number);
            page_table_ptr[id * m + page_number].count = time_stamp;
        }
    }

    shmdt(pcb_ptr);
    shmdt(page_table_ptr);
    shmdt(free_list_ptr);
}

int main (int argc, char *argv[]) {

    if (argc != 8) {
        printf("Usage: %s k m ptbid fid pcbid m2id m3id\n", argv[0]);
        exit(1);
    }

    printf("MMU: Started\n");

    k = atoi(argv[1]);
    m = atoi(argv[2]);

    pagetableid = atoi(argv[3]);
    freeid = atoi(argv[4]);
    pcbid = atoi(argv[5]);
    msgq2id = atoi(argv[6]);
    msgq3id = atoi(argv[7]);

    page_faults = (int *) malloc(sizeof(int) * k);

    for(int i = 0; i < k; i++) {
        page_faults[i] = 0;
    }

    result = fopen("result.txt", "w");

    fprintf(result, "Page Reference   : ( Time Stamp, Process ID, Page Number )\n");
    fprintf(result, "Invalid Page Ref : ( Process ID, Page Number )\n");
    fprintf(result, "Page Fault       : ( Process ID, Page Number )\n");
    fprintf(result, "-----------------------------------------------------\n\n");

    while(terminated < k) {
        serve_request();
    }

    printf("Page Faults => \n");
    printf("Process ID\tPage Faults\n");

    fprintf(result, "Page Faults => \n");
    fprintf(result, "Process ID\tPage Faults\n");

    for(int i = 0; i < k; i++) {
        printf("%d\t\t%d\n", i, page_faults[i]);
        fprintf(result, "%d\t\t\t%d\n", i, page_faults[i]);
    }

    printf("\n_._._._._._._._._._._._._._._.__._._._._._._._._._._._._._._._\n");
    fprintf(result, "_._._._._._._._._._._._._._._.__._._._._._._._._._._._._._._._\n");

    fclose(result);

    printf("\nMMU: Exiting !!!\n");
    sleep(30);
    exit(1);
    return 0; 
}

int read_request(int * id) {
    struct msg msg;

    int len = sizeof(struct msg) - sizeof(long);

    int ret = msgrcv(msgq3id, &msg, len, FROM, 0);

    if(ret == -1) {
        if(errno == EINTR) {
            return -1;
        }
        perror("msgrcv");
        exit(1);
    }

    *id = msg.id;
    return msg.page_number;
}

void send_reply(int id, int frame_number) {
    struct msgtoproc msg;
    msg.msg_type = id + TO;
    msg.frame_number = frame_number;

    int len = sizeof(struct msgtoproc) - sizeof(long);

    int ret = msgsnd(msgq3id, &msg, len, 0);

    if(ret == -1) {
        perror("msgsnd");
        exit(1);
    }
}

void notify_scheduler(int type) {
    struct msgtosch msg;
    msg.msg_type = type;

    int len = sizeof(struct msgtosch) - sizeof(long);

    int ret = msgsnd(msgq2id, &msg, len, 0);

    if(ret == -1) {
        perror("msgsnd");
        exit(1);
    }
}

int page_fault_handler(int id, int page_number) {
    if(free_list_ptr->current == -1 || pcb_ptr[id].frame_allocated >= pcb_ptr[id].frame_count) {
        int min = INT_MAX, mini = -1;

        int frame_number = -1;

        for(int i = 0; i < pcb_ptr[i].m; i++) {
            if(page_table_ptr[i].valid && page_table_ptr[id * m + page_number].count < min) {
                min = page_table_ptr[id * m + page_number].count;
                mini = i;

                frame_number = page_table_ptr[id * m + page_number].frame_number;
            }
        }

        page_table_ptr[id*m + mini].valid = 0;
        return frame_number;
    }

    int frame_number = free_list_ptr->freelist[free_list_ptr->current];
    free_list_ptr->current--;

    return frame_number;
}

void free_pages(int id) {
    for(int i = 0; i < m; i++) {
        if(page_table_ptr[id * m + i].valid) {
            free_list_ptr->current++;
            free_list_ptr->freelist[free_list_ptr->current] = page_table_ptr[id * m + i].frame_number;
            page_table_ptr[id * m + i].valid = 0;
        }
    }
}