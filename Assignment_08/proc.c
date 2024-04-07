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

#define MAX_PAGES 1024

#define TO 10
#define FROM 20

#define PAGE_FAULT -1
#define INVALID_PAGE_NUMBER -2
#define PROCESS_DONE -9

int page_number[MAX_PAGES];
int total_pages;

struct mymsg {
    long msg_type;
    int id;
};

struct mmumsg_send {
    long msg_type;
    int id;
    int page_number;
};

struct mmumsg_recv {
    long msg_type;
    int frame_number;
};

void process_ref_string (char *ref_string) {
    const char s[2] = "|";
    char *tok;

    tok = strtok(ref_string, s);

    while (tok != NULL) {
        page_number[total_pages++] = atoi(tok);
        tok = strtok(NULL, s);
    }
}

int send_msg (int qid, struct mymsg *qbuf) {
    int ret;

    int len = sizeof(struct mymsg) - sizeof(long);

    if ((ret = msgsnd(qid, qbuf, len, 0)) == -1) {
        printf("send_msg of proc.c => ");
        perror("Error in sending message");
        exit(1);
    }

    return ret;
}

int read_msg (int qid, long type, struct mymsg *qbuf) {
    int ret;

    int len = sizeof(struct mymsg) - sizeof(long);

    if ((ret = msgrcv(qid, qbuf, len, type, 0)) == -1) {
        printf("read_msg of proc.c => ");
        perror("Error in receiving message");
        exit(1);
    }

    return ret;
}

int send_msg_mmu (int qid, struct mmumsg_send *qbuf) {
    int ret;

    int len = sizeof(struct mmumsg_send) - sizeof(long);

    if ((ret = msgsnd(qid, qbuf, len, 0)) == -1) {
        printf("send_msg_mmu in proc.c => ");
        perror("Error in sending message");
        exit(1);
    }

    return ret;
}

int read_msg_mmu (int qid, long type, struct mmumsg_recv *qbuf) {
    int ret;

    int len = sizeof(struct mmumsg_recv) - sizeof(long);

    if ((ret = msgrcv(qid, qbuf, len, type, 0)) == -1) {
        printf("read_msg_mmu in proc.c => ");
        perror("Error in receiving message");
        exit(1);
    }

    return ret;
}

int main(int argc, char * argv[]) {
    if(argc != 5) {
        printf("Usage: %s id rid mq3id ref_str\n", argv[0]);
        exit(1);
    }

    printf("Process %d: Started !!!\n", atoi(argv[1]));

    int id = atoi(argv[1]);
    int readyid = atoi(argv[2]);
    int msgq3id = atoi(argv[3]);
    char *ref_str = argv[4];

    total_pages = 0;

    process_ref_string(ref_str);

    struct mymsg msg_send;

    msg_send.id = id;
    msg_send.msg_type = TO;

    send_msg(readyid, &msg_send);

    struct mymsg msg_recv;
    read_msg(readyid, FROM + id, &msg_recv);

    struct mmumsg_send msg_mmu_send;
    struct mmumsg_recv msg_mmu_recv;
    int count = 0;

    printf("Process %d: Total Pages = %d\n", id, total_pages);

    while(count < total_pages) {
        printf("Process %d: Sending REQ for page number %d\n", id, page_number[count]);
        
        msg_mmu_send.id = id;
        msg_mmu_send.msg_type = TO;
        msg_mmu_send.page_number = page_number[count];
        send_msg_mmu(msgq3id, &msg_mmu_send);

        read_msg_mmu(msgq3id, FROM + id, &msg_mmu_recv);
        
        if(msg_mmu_recv.frame_number == PAGE_FAULT) {
            printf("Process %d: PAGE FAULT for page number %d\n", id, page_number[count]);
        } else if (msg_mmu_recv.frame_number == INVALID_PAGE_NUMBER) {
            printf("Process %d: Page number %d INVALID page reference\n", id, page_number[count]);

            break;
        } else {
            printf("Process %d: Page number %d loaded in frame number %d\n", id, page_number[count], msg_mmu_recv.frame_number);

            count++;
        }
    }

    printf("Process %d: Terminated !!!\n", id);

    msg_mmu_send.id = id;
    msg_mmu_send.page_number = PROCESS_DONE;
    msg_mmu_send.msg_type = TO;
    send_msg_mmu(msgq3id, &msg_mmu_send);

    exit(1);
    return 0;
}