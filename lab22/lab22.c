#include <stdio.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define A "A created\n"
#define B "B created\n"
#define C "C created\n"
#define AB "AB created\n"
#define CR "CREW created\n"

sem_t sem_a, sem_b, sem_c, sem_ab;
pthread_t thread_a, thread_b, thread_c, thread_ab;

void sems_init() {
        sem_init(&sem_a, 0, 0);
	sem_init(&sem_b, 0, 0);
	sem_init(&sem_c, 0, 0);
	sem_init(&sem_ab, 0, 0);
}

void pthread_create_err_proc(pthread_t *thread,
        const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg) {
    int ret;
    do {
	    ret = pthread_create (thread, attr, start_routine, arg);
	    if (ret != 0)
	        fprintf(stderr, "Error: pthread_create() for %s failed.\n", strerror(ret));
	} while (ret != 0 && ret == EAGAIN);
}

int sem_wait_err_proc(sem_t *sem) {
    int ret = sem_wait(sem);
    if (ret != 0) {
        fprintf(stderr, "sem_wait() failed for %s\n", strerror(ret));
        fprintf(stderr, "Error : incorrect program execution \n");
    }
    return ret;
}

int sem_post_err_proc(sem_t *sem) {
    int ret = sem_post(sem);
    if (ret != 0) {
        fprintf(stderr, "sem_post() failed for %s\n", strerror(ret));
        fprintf(stderr, "Error : incorrect program execution \n");
    }
    return ret;
}

void pthread_join_err_proc(pthread_t thread, void **retval) {
    int ret = pthread_join(thread, retval);
    if (ret != 0) {
        fprintf(stderr, "Error: pthread_join() failed with %s", strerror(ret));
    }
}

void cancel_all_threads() {
    pthread_cancel(thread_a);
    pthread_cancel(thread_b);
    pthread_cancel(thread_c);
    pthread_cancel(thread_ab);
}


void* create_a(void* args) {
    while (1) {
        sleep(1);
        if (sem_post_err_proc(&sem_a) != 0) {
            cancel_all_threads();
            pthread_exit(-1);
        }
        write(1, A, strlen(A));
    }
}
void* create_b(void* args) {
    while (1) {
        sleep(2);
        if (sem_post_err_proc(&sem_b) != 0) {
            cancel_all_threads();
            pthread_exit(-1);
        }
        write(1, B, strlen(B));
    }
}
void* create_c(void* args) {
    while (1) {
        sleep(3);
        if (sem_post_err_proc(&sem_c) != 0) {
            cancel_all_threads();
            pthread_exit(-1);
        }
        write(1, C, strlen(C));
    }
}
void* create_ab(void* args) {
    while (1) {
        if (sem_wait_err_proc(&sem_a) != 0) {
            cancel_all_threads();
            pthread_exit(-1);
        }
        if (sem_wait_err_proc(&sem_b) != 0) {
            cancel_all_threads();
            pthread_exit(-1);
        }
        if (sem_post_err_proc(&sem_ab) != 0) {
            cancel_all_threads();
            pthread_exit(-1);
        }
        write(1, AB, strlen(AB));
    }
}

void pthreads_create() {
    pthread_create_err_proc(&thread_a, NULL, create_a, NULL);
	pthread_create_err_proc(&thread_b, NULL, create_b, NULL);
	pthread_create_err_proc(&thread_c, NULL, create_c, NULL);
	pthread_create_err_proc(&thread_ab, NULL, create_ab, NULL);
}

void create_screw() {
    while (1) {
        if (sem_wait_err_proc(&sem_ab) != 0) {
            cancel_all_threads();
            return;
        }
        if (sem_wait_err_proc(&sem_c) != 0) {
            cancel_all_threads();
            return;
        }
        write(1, CR, strlen(CR));
    }
}

void join_all_threads() {
    pthread_join_err_proc(thread_a, NULL);
    pthread_join_err_proc(thread_b, NULL);
    pthread_join_err_proc(thread_c, NULL);
    pthread_join_err_proc(thread_ab, NULL);
}

void sems_destroy() {
    sem_destroy(&sem_a);
    sem_destroy(&sem_b);
    sem_destroy(&sem_c);
    sem_destroy(&sem_ab);
}

int main() {
	sems_init();
	pthreads_create();
	create_screw();
	join_all_threads();
	sems_destroy();
	pthread_exit(0);
}
