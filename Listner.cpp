#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

void handler(int dummy) {
    ;
}

main() {
    key_t key;
    int shmid;
    void *shmaddr;
    char buf[1024];
    sigset_t mask;

    key = ftok("/etc/passwd", 1);
    shmid = shmget(key, 1024, IPC_CREAT|0666);

    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigset(SIGUSR1, handler);
    printf("Listener wait for Talker\n");
    sigsuspend(&mask);

    printf("Listener Start =====\n");
    shmaddr = shmat(shmid, NULL, 0);
    strcpy(buf, shmaddr);
    printf("Listener received : %sn", buf);

    strcpy(shmaddr, "Have a nice day\n");
    sleep(10);
    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);
}
