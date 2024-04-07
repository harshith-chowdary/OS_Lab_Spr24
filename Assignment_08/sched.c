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

#define PROCESS_TERM 30

#define PAGE_FAULT_HANDLED 40

#define FROM 10
#define TO 20

int k;

struct mmutosch {
    long msg_type;
    char msg_buf[1];
};

struct mymsg {
    long msg_type;
    int id;
};

int send_msg (int qid, struct mymsg *qbuf) {
    int ret;

    int len = sizeof(struct mymsg) - sizeof(long);

    if ((ret = msgsnd(qid, qbuf, len, 0)) == -1) {
        printf("send_msg of sched.c => ");
        perror("Error in sending message");
        exit(1);
    }

    return ret;
}

int read_msg (int qid, long type, struct mymsg *qbuf) {
    int ret;

    int len = sizeof(struct mymsg) - sizeof(long);

    if ((ret = msgrcv(qid, qbuf, len, type, 0)) == -1) {
        printf("read_msg of sched.c => ");
        perror("Error in receiving message");
        exit(1);
    }

    return ret;
}

int read_mmu_msg (int qid, long type, struct mmutosch *qbuf) {
    int ret;

    int len = sizeof(struct mmutosch) - sizeof(long);

    if ((ret = msgrcv(qid, qbuf, len, type, 0)) == -1) {
        printf("read_mmu_msg of sched.c => ");
        perror("Error in receiving message");
        exit(1);
    }

    return ret;
}

int main (int argc, char * argv[]) {
    if(argc != 5) {
        printf("Usage: %s k rid m2id pid\n", argv[0]);
        exit(1);
    }

    printf("Scheduler: Started\n");

    int k = atoi(argv[1]);
    int readyid = atoi(argv[2]);
    int msgq2id = atoi(argv[3]);
    int master_pid = atoi(argv[4]);

    int term_count = 0;

    struct mymsg msg_send, msg_recv;

    while(1) {
        read_msg(readyid, FROM, &msg_recv);

        int id = msg_recv.id;

        msg_send.msg_type = TO + id;
        send_msg(readyid, &msg_send);

        struct mmutosch mmu_recv;
        read_mmu_msg(msgq2id, 0, &mmu_recv);

        if(mmu_recv.msg_type == PAGE_FAULT_HANDLED) {
            msg_send.id = id;
            msg_send.msg_type = FROM;
            send_msg(readyid, &msg_send);
        }
        else if (mmu_recv.msg_type == PROCESS_TERM) {
            term_count++;

            printf("Scheduler: Process %d terminated\n", id);
            if(term_count == k) {
                break;
            }
        }
        else {
            printf("Invalid message type from MMU\n");
            exit(1);
        }
    }

    kill(master_pid, SIGUSR1);
    pause();

    printf("Scheduler: Exiting !!!\n");
    exit(1);
    return 0;
}