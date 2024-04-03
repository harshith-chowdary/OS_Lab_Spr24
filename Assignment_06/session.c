#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "event.h"

#define NUM_PATIENTS 25
#define NUM_SALES_REPS 3
#define NUM_REPORTERS 250

typedef struct {
    int time;
    int duration;
} wpatient;

typedef struct {
    int time;
    int duration;
} wreporter;

typedef struct {
    int time;
    int duration;
} wsales_rep;

wpatient waiting_patients[NUM_PATIENTS+1];
wreporter waiting_reporters[NUM_REPORTERS+1];
wsales_rep waiting_sales_reps[NUM_SALES_REPS+1];

eventQ eventQueue;

int current_time = 0;

bool doctor_firstcase = false;
bool doctor_free = false;
bool doctor_done = false;
bool doctor_enter = false;
bool doctor_met = false;

bool session_done = false;

int current_reporter = 0;
int current_patient = 0;
int current_sales_rep = 0;

int num_reporters = 0;
int num_patients = 0;
int num_sales_reps = 0;

int token_reporters = 0;
int token_patients = 0;
int token_sales_reps = 0;

pthread_t assistantThread, doctorThread;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_reporter = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_patient = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_sales_rep = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_doc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_doc_meet = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_doc_finish = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_enter = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_doctor = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_doctor_meet = PTHREAD_COND_INITIALIZER;
pthread_cond_t doctor_completed = PTHREAD_COND_INITIALIZER;
pthread_cond_t doctor_entered = PTHREAD_COND_INITIALIZER;

pthread_cond_t cond_reporter = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_patient = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_sales_rep = PTHREAD_COND_INITIALIZER;

void *assistant (void *);
void *doctor (void *);
void *reporter (void *);
void *patient (void *);
void *sales_rep (void *);

void print_time (int);

int main(){
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Initialize event queue
    // eventQueue = initEQ("arrival.txt");
    eventQueue = initEQ("ARRIVAL.txt");

    // eventQ dup = initEQ("arrival.txt");
    // while(!emptyQ(dup)) {
    //     event e = nextevent(dup);
    //     printf("%c %d %d\n", e.type, e.time, e.duration);
    //     dup = delevent(dup);
    // }

    // return 0;

    // Create thread for assistant
    pthread_create(&assistantThread, NULL, assistant, NULL);

    // Join threads
    pthread_join(assistantThread, NULL);

    return 0;
}

void *assistant(void *arg){
    pthread_create(&doctorThread, NULL, doctor, NULL);

    event temp = {'D', 0, 0};
    eventQueue = addevent(eventQueue, temp);

    while(true){
        while(!emptyQ(eventQueue)){
            event next = nextevent(eventQueue);
            if(next.type == 'D') break;

            current_time = next.time;

            if(next.type == 'R'){
                pthread_mutex_lock(&mutex);
                token_reporters++;
                int * token = (int *)malloc(sizeof(int));
                *token = token_reporters;
                pthread_mutex_unlock(&mutex);

                printf("\t[");
                print_time(next.time);
                printf("] Reporter %d arrives\n", token_reporters);

                if(session_done){
                    printf("\t[");
                    print_time(next.time);
                    printf("] Reporter %d leaves (session done)\n", token_reporters);
                }
                else{
                    pthread_t reporterThread;
                    pthread_create(&reporterThread, NULL, reporter, (void *)token);
                    
                    waiting_reporters[token_reporters].time = next.time;
                    waiting_reporters[token_reporters].duration = next.duration;

                    num_reporters++;
                }
            }
            else if(next.type == 'P'){
                pthread_mutex_lock(&mutex);
                token_patients++;
                int * token = (int *)malloc(sizeof(int));
                *token = token_patients;
                pthread_mutex_unlock(&mutex);

                printf("\t[");
                print_time(next.time);
                printf("] Patient %d arrives\n", token_patients);

                if(session_done){
                    printf("\t[");
                    print_time(next.time);
                    printf("] Patient %d leaves (session done)\n", token_patients);
                }
                else if(token_patients > NUM_PATIENTS){
                    printf("\t[");
                    print_time(next.time);
                    printf("] Patient %d leaves (quota full)\n", token_patients);
                }
                else{
                    pthread_t patientThread;
                    pthread_create(&patientThread, NULL, patient, (void *)token);
                    
                    waiting_patients[token_patients].time = next.time;
                    waiting_patients[token_patients].duration = next.duration;

                    num_patients++;
                }
            }
            else if(next.type == 'S'){
                pthread_mutex_lock(&mutex);
                token_sales_reps++;
                int * token = (int *)malloc(sizeof(int));
                *token = token_sales_reps;
                pthread_mutex_unlock(&mutex);

                printf("\t[");
                print_time(next.time);
                printf("] Sales representative %d arrives\n", token_sales_reps);

                if(session_done){
                    printf("\t[");
                    print_time(next.time);
                    printf("] Sales representative %d leaves (session done)\n", token_sales_reps);
                }
                else if(token_sales_reps > NUM_SALES_REPS){
                    printf("\t[");
                    print_time(next.time);
                    printf("] Sales representative %d leaves (quota full)\n", token_sales_reps);
                }
                else{
                    pthread_t sales_repThread;
                    pthread_create(&sales_repThread, NULL, sales_rep, (void *)token);
                    
                    waiting_sales_reps[token_sales_reps].time = next.time;
                    waiting_sales_reps[token_sales_reps].duration = next.duration;

                    num_sales_reps++;
                }
            }

            eventQueue = delevent(eventQueue);
        }

        if(emptyQ(eventQueue)) break;
        
        event next = nextevent(eventQueue);
        eventQueue = delevent(eventQueue);

        current_time = next.time;

        pthread_mutex_lock(&mutex);
        if(num_reporters == 0 && (current_patient >= NUM_PATIENTS && current_sales_rep >= NUM_SALES_REPS)) session_done = true;
        pthread_mutex_unlock(&mutex);

        if(!doctor_firstcase && session_done){
            doctor_firstcase = true;

            pthread_mutex_lock(&mutex_doc);
            doctor_free = true;
            pthread_cond_signal(&cond_doctor);
            pthread_mutex_unlock(&mutex_doc);

            pthread_mutex_lock(&mutex_doc_finish);
            while(!doctor_done){
                pthread_cond_wait(&doctor_completed, &mutex_doc_finish);
            }
            doctor_done = false;
            pthread_mutex_unlock(&mutex_doc_finish);
        }

        pthread_mutex_lock(&mutex_doc);
        if (num_patients > 0 || num_reporters > 0 || num_sales_reps > 0) {
            doctor_free = true;
        }
        pthread_mutex_unlock(&mutex_doc);

        event temp = {'D', 0, 0};

        if(num_reporters > 0){
            temp.time = current_time + waiting_reporters[current_reporter + 1].duration;

            pthread_mutex_lock(&mutex);
            num_reporters--;
            current_reporter++;
            pthread_mutex_unlock(&mutex);

            pthread_cond_signal(&cond_doctor);
            pthread_cond_signal(&cond_reporter);
        } else if (num_patients > 0) {
            temp.time = current_time + waiting_patients[current_patient + 1].duration;

            pthread_mutex_lock(&mutex);
            num_patients--;
            current_patient++;
            pthread_mutex_unlock(&mutex);

            pthread_cond_signal(&cond_doctor);
            pthread_cond_signal(&cond_patient);
        } else if (num_sales_reps > 0) {
            temp.time = current_time + waiting_sales_reps[current_sales_rep + 1].duration;

            pthread_mutex_lock(&mutex);
            num_sales_reps--;
            current_sales_rep++;
            pthread_mutex_unlock(&mutex);

            pthread_cond_signal(&cond_doctor);
            pthread_cond_signal(&cond_sales_rep);
        }

        if(temp.time == 0){
            if(emptyQ(eventQueue)) break;
            temp.time = nextevent(eventQueue).time;
        }
        else {
            pthread_mutex_lock(&mutex_doc_finish);
            while(!doctor_done){
                pthread_cond_wait(&doctor_completed, &mutex_doc_finish);
            }
            doctor_done = false;
            pthread_mutex_unlock(&mutex_doc_finish);
        }
        
        eventQueue = addevent(eventQueue, temp);
    }

    if(!session_done){
        pthread_mutex_lock(&mutex_doc);
        doctor_free = false;
        session_done = true;
        pthread_cond_signal(&cond_doctor);
        pthread_mutex_unlock(&mutex_doc);
    }
    
    pthread_join(doctorThread, NULL);

    pthread_exit(0);
}

void *doctor(void * args){
    while(true){
        pthread_mutex_lock(&mutex_doc);
        while(!doctor_free){
            pthread_cond_wait(&cond_doctor, &mutex_doc);
        }

        doctor_free = false;
        pthread_mutex_unlock(&mutex_doc);

        printf("[");
        print_time(current_time);

        if(session_done){
            pthread_mutex_lock(&mutex_doc_finish);
            doctor_done = true;
            pthread_cond_signal(&doctor_completed);
            pthread_mutex_unlock(&mutex_doc_finish);
            printf("] Doctor leaves [End of session]\n");
            break;
        }

        printf("] Doctor has next visitor\n");

        pthread_mutex_lock(&mutex_enter);
        doctor_enter = true;
        pthread_cond_signal(&doctor_entered);
        pthread_mutex_unlock(&mutex_enter);

        pthread_mutex_lock(&mutex_doc_meet);
        while(!doctor_met){
            pthread_cond_wait(&cond_doctor_meet, &mutex_doc_meet);
        }
        doctor_met = false;
        pthread_mutex_unlock(&mutex_doc_meet);

        pthread_mutex_lock(&mutex_doc_finish);
        doctor_done = true;
        pthread_cond_signal(&doctor_completed);
        pthread_mutex_unlock(&mutex_doc_finish);
    }

    pthread_exit(0);
}

void *reporter (void * args){
    int token = *((int *) args);

    pthread_mutex_lock(&mutex_reporter);
    while(token != current_reporter){
        pthread_cond_wait(&cond_reporter, &mutex_reporter);
    }
    pthread_mutex_unlock(&mutex_reporter);

    pthread_mutex_lock(&mutex_enter);
    while(!doctor_enter){
        pthread_cond_wait(&doctor_entered, &mutex_enter);
    }
    doctor_enter = false;
    pthread_mutex_unlock(&mutex_enter);

    printf("[");
    print_time(current_time);
    printf(" - ");
    print_time(current_time + waiting_reporters[token].duration);
    printf("] Reporter %d is in doctor's chamber\n", token);

    pthread_mutex_lock(&mutex_doc_meet);
    doctor_met = true;
    pthread_cond_signal(&cond_doctor_meet);
    pthread_mutex_unlock(&mutex_doc_meet);

    pthread_exit(0);
}

void *patient (void * args){
    int token = *((int *) args);

    pthread_mutex_lock(&mutex_patient);
    while(token != current_patient){
        pthread_cond_wait(&cond_patient, &mutex_patient);
    }
    pthread_mutex_unlock(&mutex_patient);

    pthread_mutex_lock(&mutex_enter);
    while(!doctor_enter){
        pthread_cond_wait(&doctor_entered, &mutex_enter);
    }
    doctor_enter = false;
    pthread_mutex_unlock(&mutex_enter);

    printf("[");
    print_time(current_time);
    printf(" - ");
    print_time(current_time + waiting_patients[token].duration);
    printf("] Patient %d is in doctor's chamber\n", token);

    pthread_mutex_lock(&mutex_doc_meet);
    doctor_met = true;
    pthread_cond_signal(&cond_doctor_meet);
    pthread_mutex_unlock(&mutex_doc_meet);

    pthread_exit(0);
}

void *sales_rep (void * args){
    int token = *((int *) args);

    pthread_mutex_lock(&mutex_sales_rep);
    while(token != current_sales_rep){
        pthread_cond_wait(&cond_sales_rep, &mutex_sales_rep);
    }
    pthread_mutex_unlock(&mutex_sales_rep);

    pthread_mutex_lock(&mutex_enter);
    while(!doctor_enter){
        pthread_cond_wait(&doctor_entered, &mutex_enter);
    }
    doctor_enter = false;
    pthread_mutex_unlock(&mutex_enter);

    printf("[");
    print_time(current_time);
    printf(" - ");
    print_time(current_time + waiting_sales_reps[token].duration);
    printf("] Sales representative %d is in doctor's chamber\n", token);

    pthread_mutex_lock(&mutex_doc_meet);
    doctor_met = true;
    pthread_cond_signal(&cond_doctor_meet);
    pthread_mutex_unlock(&mutex_doc_meet);

    pthread_exit(0);
}

void print_time(int time) {
    // Normalize time to be within 24-hour cycle
    time = (time + 1980) % 1440;

    // Calculate hours and minutes
    int hours = time / 60;
    int minutes = time % 60;

    // Determine AM or PM
    const char *amorpm = (hours < 12) ? "am" : "pm";

    // Adjust hours for 12-hour format
    if (hours == 0) {
        hours = 12;  // 12:00 am
    } else if (hours > 12) {
        hours -= 12;
    }

    // Print time in 12-hour format
    printf("%02d:%02d%s", hours, minutes, amorpm);
}
