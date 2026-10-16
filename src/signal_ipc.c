#include "signal_ipc.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int run_worker(pid_t supervisor_pid) {
    sigset_t wait_mask;
    sigemptyset(&wait_mask);

    sigaddset(&wait_mask, SIGUSR1);
    sigaddset(&wait_mask, SIGUSR2);

    printf("worker: ready\n");
    fflush(stdout);

    kill(supervisor_pid, SIGUSR1);

    int tasks_done = 0;
    while (tasks_done < NUM_TASKS) {
        int sig = 0;

        if (sigwait(&wait_mask, &sig) != 0) {
            return 1;
        }

        if (sig == SIGUSR1) {
            tasks_done++;
            printf("worker: received task %d\n", tasks_done);
            fflush(stdout);

            kill(supervisor_pid, SIGUSR1);
        }
    }

    int final_sig;
    sigwait(&wait_mask, &final_sig);

    printf("worker: all tasks done, exiting\n");
    fflush(stdout);
    return 0;
}

int run_supervisor(pid_t worker_pid) {
    sigset_t wait_mask;
    sigemptyset(&wait_mask);

    sigaddset(&wait_mask, SIGUSR1);
    sigaddset(&wait_mask, SIGCHLD);

    int sig;
    if (sigwait(&wait_mask, &sig) != 0) {
        return 1;
    }

    for (int i = 1; i <= NUM_TASKS; i++) {
        printf("supervisor: sending task %d\n", i);
        fflush(stdout);

        kill(worker_pid, SIGUSR1);

        sigwait(&wait_mask, &sig);
    }

    printf("supervisor: shutting down worker\n");
    fflush(stdout);

    kill(worker_pid, SIGUSR2);

    sigwait(&wait_mask, &sig);

    int status;
    waitpid(worker_pid, &status, 0);
    (void)status;

    printf("supervisor: worker exited cleanly\n");
    fflush(stdout);
    return 0;
}

int main(void) {
    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    sigaddset(&block_mask, SIGUSR2);
    sigaddset(&block_mask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &block_mask, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        return run_worker(getppid());
    }

    return run_supervisor(child_pid);
}.0