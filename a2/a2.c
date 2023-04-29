#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>
#include "a2_helper.h"

#define P3_NR_THREADS 4
#define P6_NR_THREADS 50
#define P2_NR_THREADS 6

sem_t *t31_sem = NULL;
sem_t *t25_sem = NULL;

//---------------------------------------------------------------------------------------------------------------------------2.3
typedef struct {
    int tid;
    int pid;
    sem_t *sem;
} TH_P3_STRUCT;

void *thread_p3_function(void *param)
{
    TH_P3_STRUCT *s = (TH_P3_STRUCT*)param;

    if(s->tid ==1) {
        sem_wait(t31_sem);
            info(BEGIN, s->pid, s->tid);
            info(END, s->pid, s->tid);
        sem_post(t25_sem);
        return NULL;
    }
    if(s->tid == 4) {
        info(BEGIN, s->pid, s->tid);
            sem_post(&s->sem[2]);
            sem_wait(&s->sem[4]);
        info(END, s->pid, s->tid);
        return NULL;
    } 
    if(s->tid == 2) {
        sem_wait(&s->sem[2]);
            info(BEGIN, s->pid, s->tid);
            info(END, s->pid, s->tid);
        sem_post(&s->sem[4]);
        return NULL;
    }
    
    info(BEGIN, s->pid, s->tid);
    info(END, s->pid, s->tid);
    return NULL;
}

void p3_helper()
{
    pthread_t tids[P3_NR_THREADS+1];
    TH_P3_STRUCT params[P3_NR_THREADS+1];
    sem_t sem[P3_NR_THREADS+1];

    t31_sem = sem_open("/a2_t31_semaphore", O_CREAT, 0644, 0);
    t25_sem = sem_open("/a2_t25_semaphore", O_CREAT, 0644, 0);

    for(int i=1; i<=P3_NR_THREADS; i++){
        sem_init(&sem[i], 0, 0);
    }
    for(int i=1; i<=P3_NR_THREADS; i++){
        params[i].tid = i;
        params[i].pid = 3;
        params[i].sem = sem;
        pthread_create(&tids[i], NULL, thread_p3_function, &params[i]);
    }
    for(int i=1; i<=P3_NR_THREADS; i++){
        pthread_join(tids[i], NULL);
    }   

    for(int i=1; i<=P3_NR_THREADS; i++){
        sem_destroy(&sem[i]);
    }
    
    sem_close(t31_sem);
    sem_close(t25_sem);
    return;
}

//-------------------------------------------------------------------------------------------------------------------------------2.4
typedef struct {
    int tid;
    int pid;

} TH_P6_STRUCT;

int nrThreads = 0;
int am_iesit = 0;

sem_t max5_sem;

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *thread_p6_function(void *param)
{
    TH_P6_STRUCT *s = (TH_P6_STRUCT*)param;

    sem_wait(&max5_sem);
        info(BEGIN, s->pid, s->tid);
        
        pthread_mutex_lock(&lock);
        nrThreads++;
        
        if(s->tid == 13) {
            while(nrThreads < 5) {
                pthread_cond_wait(&cond1, &lock);
            }
            am_iesit = 1;
            info(END, s->pid, s->tid);
            pthread_cond_broadcast(&cond1);
        } else {
            while(nrThreads < 5 && !am_iesit) {
                if(nrThreads == 4) {
                    pthread_cond_signal(&cond2);
                }
                pthread_cond_wait(&cond1, &lock);
            }
            info(END, s->pid, s->tid);
        }
                
        nrThreads--;
        pthread_mutex_unlock(&lock);

    sem_post(&max5_sem);

    return NULL;
}

void p6_helper()
{
    pthread_t tids[P6_NR_THREADS+1];
    TH_P6_STRUCT params[P6_NR_THREADS+1];
    sem_init(&max5_sem, 0, 5);

    for(int i=1; i<=P6_NR_THREADS; i++){
        params[i].tid = i;
        params[i].pid = 6;
        pthread_create(&tids[i], NULL, thread_p6_function, &params[i]);
    }
    for(int i=1; i<=P6_NR_THREADS; i++){
        pthread_join(tids[i], NULL);
    }   

    sem_destroy(&max5_sem);
    return;
}

//-------------------------------------------------------------------------------------------------------------------------------2.5
typedef struct {
    int tid;
    int pid;

} TH_P2_STRUCT;

void *thread_p2_function(void *param)
{
    TH_P2_STRUCT *s = (TH_P2_STRUCT*)param;

    if(s->tid == 2) {
        info(BEGIN, s->pid, s->tid);
        info(END, s->pid, s->tid);
        sem_post(t31_sem);
        return NULL;
    } 
    
    if(s->tid == 5) {
        sem_wait(t25_sem);
        
        info(BEGIN, s->pid, s->tid);
        info(END, s->pid, s->tid);
        return NULL;
    }

    info(BEGIN, s->pid, s->tid);
    info(END, s->pid, s->tid);
    return NULL;
}

void p2_helper()
{
    pthread_t tids[P2_NR_THREADS+1];
    TH_P2_STRUCT params[P2_NR_THREADS+1];

    t31_sem = sem_open("/a2_t31_semaphore", O_CREAT, 0644, 0);
    t25_sem = sem_open("/a2_t25_semaphore", O_CREAT, 0644, 0);

    for(int i=1; i<=P2_NR_THREADS; i++){
        params[i].tid = i;
        params[i].pid = 2;
        pthread_create(&tids[i], NULL, thread_p2_function, &params[i]);
    }
    for(int i=1; i<=P2_NR_THREADS; i++){
        pthread_join(tids[i], NULL);
    }   

    sem_close(t31_sem);
    sem_close(t25_sem);
    return;
}

//-----------------------------------------------------------------------------------------------------------------------------2.2
int create_hierarchy_process (pid_t *pids)
{
    //----------------------------------------------------P1
    pids[1] = getpid();

    if((pids[2] = fork()) == -1) {
        return -1;
    } else if(pids[2] == 0) {
    //-----------------------------------------------------P2
        info(BEGIN, 2, 0);
        if((pids[8] = fork()) == -1) {
            return -1;
        } else if(pids[8] == 0) {
        //-----------------------------P8
            info(BEGIN, 8, 0);
            info(END, 8, 0);
            exit(0);
        //-----------------------------P8
        }
        p2_helper();
        waitpid(pids[8], NULL, 0);
        info(END, 2, 0);
        exit(0);
    //-----------------------------------------------------P2
    }

    if((pids[3] = fork()) == -1) {
        return -1;
    } else if(pids[3] == 0) {
    //-----------------------------------------------------P3
        info(BEGIN, 3, 0);
        if((pids[5] = fork()) == -1) {
            return -1;
        } else if(pids[5] == 0) {
        //-----------------------------P5
            info(BEGIN, 5, 0);
            p3_helper();
            info(END, 5, 0);
            exit(0);
        //-----------------------------P5
        }
        if((pids[7] = fork()) == -1) {
            return -1;
        } else if(pids[7] == 0) {
        //-----------------------------P7
            info(BEGIN, 7, 0);
            info(END, 7, 0);
            exit(0);
        //-----------------------------P7
        }
        waitpid(pids[5], NULL, 0);
        waitpid(pids[7], NULL, 0);
        info(END, 3, 0);
        exit(0);
    //-----------------------------------------------------P3
    }

    if((pids[4] = fork()) == -1) {
        return -1;
    } else if(pids[4] == 0) {
    //-----------------------------------------------------P4
        info(BEGIN, 4, 0);
        if((pids[6] = fork()) == -1) {
            return -1;
        } else if(pids[6] == 0) {
        //---------------------------P6
            info(BEGIN, 6, 0);
            p6_helper();
            info(END, 6, 0);
            exit(0);
        //---------------------------P6
        }
        waitpid(pids[6], NULL, 0);
        info(END, 4, 0);
        exit(0);
    //-----------------------------------------------------P4
    }

    waitpid(pids[2], NULL, 0);
    waitpid(pids[3], NULL, 0);
    waitpid(pids[4], NULL, 0);

    //----------------------------------------------------P1
    return 0;
}

int main()
{
    //-------------------------------------------P1
    init();
    info(BEGIN, 1, 0);
    
    pid_t pids[9];

    if(create_hierarchy_process(pids) < 0){
        fprintf(stderr, "Could not create hierarchy process \n");
        info(END, 1, 0);
        return -1;
    }

    info(END, 1, 0);
    return 0;
}
