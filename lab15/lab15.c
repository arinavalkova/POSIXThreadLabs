#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#define LINE_COUNT 10

sem_t *sem_open_err_proc(const char *name, unsigned int value);
void sem_close_err_proc(sem_t *sem);
void sem_unlink_err_proc(const char *name);
pid_t fork_err_proc();
int sem_wait_err_proc(sem_t *sem);
int sem_post_err_proc(sem_t *sem);

int main(int argc, char **argv) {
    char sem1_name[] = "/sem11";
    char sem2_name[] = "/sem22";
    sem_t *sem1 = sem_open_err_proc(sem1_name, 0);
    sem_t *sem2 = sem_open_err_proc(sem2_name, 1);
    if (sem1 == NULL || sem2 == NULL) {
        exit(EXIT_FAILURE);
    }

    pid_t fork_stat = fork_err_proc();
    if (fork_stat == -1) {
        exit(EXIT_FAILURE);
    } else if(fork_stat == 0) {
       for(int i = 0; i < LINE_COUNT; i++) {
           if(sem_wait_err_proc(sem2) == -1) {
               exit(EXIT_FAILURE);
           }
           printf("I am a child...\n");
           if(sem_post_err_proc(sem1) == -1) {
               exit(EXIT_FAILURE);
           }
       }
        sem_close_err_proc(sem1);
        sem_close_err_proc(sem2);
    } else {
        for(int i = 0; i < LINE_COUNT; i++) {
            if(sem_wait_err_proc(sem1) == -1) {
                exit(EXIT_FAILURE);
            }
            printf("I am a parent...\n");
            if(sem_post_err_proc(sem2) == -1) {
                exit(EXIT_FAILURE);
            }
        }

        sem_close_err_proc(sem1);
        sem_close_err_proc(sem2);

        sem_unlink_err_proc(sem1_name);
        sem_unlink_err_proc(sem2_name);
    }
    exit(EXIT_SUCCESS);
}

sem_t *sem_open_err_proc(const char *name, unsigned int value) {
    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, O_RDWR, value);
    if (sem == SEM_FAILED) {
        int err = errno;
        fprintf(stderr, "Error: sem_open() for %s failed.  errno:%d\n", name, err);
        if (err == EEXIST)
            fprintf(stderr, "EEXIST : Both O_CREAT and O_EXCL were specified in oflag, "
                            "but a semaphore with this name already exists. \n");
    }
    return sem;
}

pid_t fork_err_proc() {
    pid_t stat;
    int err = errno;
    do {
        stat = fork();
        if (stat == -1) {
            fprintf(stderr, "Error: fork() for %s failed.  errno:%d\n", err);
        }
    } while (stat == -1 && err == EAGAIN);
    return stat;
}

int sem_wait_err_proc(sem_t *sem) {
    int stat = sem_wait(sem);
    if (stat == -1) {
        int err = errno;
        fprintf(stderr, "sem_wait() failed. errno:%d\n", err);
        fprintf(stderr, "Error : incorrect program execution \n");
    }
    return stat;
}

int sem_post_err_proc(sem_t *sem) {
    int stat = sem_post(sem);
    if (stat == -1) {
        int err = errno;
        fprintf(stderr, "sem_post() failed. errno:%d\n", err);
        fprintf(stderr, "Error : incorrect program execution \n");
    }
    return stat;
}

void sem_close_err_proc(sem_t *sem) {
    if (sem_close(sem) == -1) {
        int err = errno;
        fprintf(stderr, "sem_close() failed. errno:%d\n", err);
        if (err == EINVAL)
            fprintf(stderr, "EINVAL : sem is not a valid semaphore.");
    }
}

void sem_unlink_err_proc(const char *name) {
    if (sem_unlink(name) == -1) {
        int err = errno;
        fprintf(stderr, "sem_unlink() failed. errno:%d\n", err);
        if (err == EACCES) {
            fprintf(stderr, "EACCES : the caller does not have permission to unlink this semaphore.");
        }
    }
}
