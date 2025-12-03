#include <bits/types/sigset_t.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>
#include "util/rsleep.h"

volatile sig_atomic_t pv = 5;

void prendre_degat(int) {
    pv = pv - 1;
}

void attaque(pid_t adversaire) {
    struct sigaction act = {};
    act.sa_handler = prendre_degat;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR1, &act, nullptr);

    if (kill(adversaire, SIGUSR1) < 0) {
        perror("kill");
        if (errno == ESRCH) exit(0);
    }
    pr::randsleep();
}

void defense() {
    struct sigaction act = {};
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR1, &act, nullptr);
    pr::randsleep();
}

void parer(int) {
    const char msg[] = "Coup parre\n";
    write(1, msg, sizeof(msg) - 1);
}

void defenseLuke() {
    struct sigaction act = {};
    act.sa_handler = parer;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR1, &act, nullptr);

    sigset_t full, old;
    sigfillset(&full);
    sigprocmask(SIG_BLOCK, &full, &old);

    pr::randsleep();

    sigset_t empty;
    sigemptyset(&empty);
    sigsuspend(&empty);

    sigprocmask(SIG_SETMASK, &old, nullptr);
}

void afficher_pv(const char* nom) {
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%s PV: %d\n", nom, pv);
    write(1, buf, len);
}

void combatVador(pid_t adversaire) {
    while (pv > 0) {
        defense();
        attaque(adversaire);
        afficher_pv("Vador");
        int status;
        pid_t res = waitpid(adversaire, &status, WNOHANG);
        if (res == adversaire) exit(0); // l’adversaire est mort
    }
    std::cout << "Défaite pour Vador\n";
    exit(1);
}

void combatLuke(pid_t adversaire) {
    while (pv > 0) {
        defenseLuke();
        attaque(adversaire);
        afficher_pv("Luke");
        std::cout << "PV restants: " << pv << std::endl;
    }
    std::cout << "Défaite pour Luke\n";
    exit(1);
}

int main() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid > 0) {
        combatVador(pid);
    } else {
        combatLuke(getppid());
    }
    return 0;
}