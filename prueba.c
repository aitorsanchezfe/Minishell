#include <stdio.h>
#include <signal.h>

void 
manejador(int sig) {
    
    printf("Recibida señal %d!\n",sig);
}

int 
main() {

    signal(SIGINT, manejador);
    printf("Esperando señal.\n");
    pause();
    printf("Saliendo.\n");
    return 0;
}