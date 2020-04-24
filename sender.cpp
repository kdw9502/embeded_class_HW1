//
// Created by kdw on 2020-04-22.
//

#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

main(int argc, char **argv) {
    key_t key;
    int shmid;
    void *shmaddr;
    char buf[1024];

    key = ftok("/etc/passwd", 1);
    shmid = shmget(key, 1024, 0);

    shmaddr = shmat(shmid, NULL, 0);
    strcpy(shmaddr, "Hello, I'm sender\n");

    kill(atoi(argv[1]), SIGUSR1);

    sleep(1);
    strcpy(buf, shmaddr);
    printf("Listener said : %s\n", buf);
    sleep(3);
    system("ipcs");
    shmdt(shmaddr);
}
