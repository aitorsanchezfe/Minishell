#include <stdio.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parser.h"

tline *line;

void 
handler_signint(int sig) {
    
    printf("msh> ");

}

void
create_pipes(int **p){ // Función que crea los pipes necesarios para el exec_commands.
    int i;

        for(i=0;i<line->ncommands-1;i++){ // nº de pipes= nº de comandos -1.
            
            p[i]=malloc(sizeof(int)*2);
            pipe(p[i]);
        }
}

void
close_pipes(int **p){ // Función que cierra todos los pipes que han sido creados por el create_pipes.
    int i;

    for(i=0;i<line->ncommands-1;i++){
        
        close(p[i][0]); //cierro el pipe de entrada.
        close(p[i][1]); //cierro el pipe de salida.
        free(p[i]); //libero la memoria creada por malloc del create_pipes.
    }
    free(p); //libero la memoria creada por el 2do malloc del create_pipes.
}

void 
exec_commands(void){ // Función que ejecuta 1/n mandato/s.
    int i,fd;
    pid_t pid;
    int **p;

    if (line->ncommands>1){ // Condición imprescindible para que haya un uso de pipes.
        
        p=malloc(sizeof(int*)*(line->ncommands-1)); // Reserva dinámica de memoria para el espacio que va a ocupar el array de p.
        create_pipes(p);

    }

    for (i=0;i<line->ncommands;i++){
        pid= fork();
        if (pid<0){
            fprintf(stderr,"Error en el fork\n");
            exit(1);

        }
        if (pid==0){
            if (line->commands[i].filename== NULL){
                fprintf(stderr,"mandato: No se encuentra el mandato\n");
                exit(1);

            }
            if (line->ncommands>1){
                if(i==0) // Si es el primer comando del pipe, solo redirecciono la salida estandar.
                    dup2(p[0][1],1);

                else if (i==(line->ncommands-1)) // Si es el último comando del pipe, solo redirecciono la entrada estandar.
                    dup2(p[i-1][0],0);

                else { // Cuando es un comando que va entre medias, tengo que redireccionar tanto entrada como salida estandar.
                    dup2(p[i-1][0],0);
                    dup2(p[i][1],0);

                } 
                close_pipes(p);   

            }

            if((i==0)&&(line->redirect_input!=NULL)){ // Redirección de entrada en el primer mandato siempre.
                fd=open(line->redirect_input,O_RDONLY);
                
                if(fd<0){
                    fprintf(stderr,"Error al abrir el fichero de entrada.\n");
                    exit(1);

                } else {
                    dup2(fd,0);
                    close(fd);

                }
            }

            if((i==(line->ncommands-1))&&(line->redirect_output!=NULL)){ // Redirección de salida en el último mandato siempre.
                fd=creat(line->redirect_output,0644);
                if (fd<0){
                    fprintf(stderr,"Error al crear el fichero de redirección de salida.\n");
                    exit(1);

                } else {
                    dup2(fd,1);
                    close(fd);
                }
            }

            if((i==(line->ncommands-1))&&(line->redirect_error!=NULL)){ // Redirección de error en el último mandato siempre.
                fd=creat(line->redirect_error,0644);
                if (fd<0){
                    fprintf(stderr,"Error al crear el fichero de redirección de error.\n");
                    exit(1);

                } else {
                    dup2(fd,2);
                    close(fd);
                }
            }

            execvp(line->commands[i].filename,line->commands[i].argv);
            exit(1); // Si el proceso hijo llega a ejecutar este mandato, es porque execvp no se ha ejecutado correctamente.

        }    
    }

    if (line->ncommands>1) // El padre tambien tiene que cerrar los pipes creados ya que han sido creados previos al fork.
        close_pipes(p);   
    
    for (i=0;i<line->ncommands;i++){ // Realizamos todos los wait a los hijos.
        wait(NULL);

    }    
}

void
exec_cd(void){ // Función que ejecuta el mandato cd. 
    char *home;
    int dir;
    char dir_actual[PATH_MAX];

    if(line->commands[0].argv[1]==NULL){ // Si no tiene argumentos, cambiamos al directorio HOME.
        home=getenv("HOME");
        dir=chdir(home);

        if (dir<0)
            fprintf(stderr,"Error cd.\n");

        else { 
            getcwd(dir_actual,PATH_MAX);
            printf("Directorio actual: %s\n",dir_actual);  
        }
 
    } else if (line->commands[0].argv[2]==NULL){ // Para comprobar que el mandato contiene cd y un directorio únicamente.
        dir=chdir(line->commands[0].argv[1]);
        if (dir<0)
            fprintf(stderr,"Error: %s no existe o no es un directorio accesible.\n", line->commands[0].argv[1]);

        else { 
            getcwd(dir_actual,PATH_MAX);
            printf("Directorio actual: %s\n",dir_actual);  
        } 
    } else // Si el mandato contiene cd y mas de un argumento más, se está utilizando mal el cd.
        fprintf(stderr,"Error uso de cd: demasiados argumentos.\n");
}

int 
main(void) {
	char buf[1024];

    printf("msh> ");	

	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);

		if (line==NULL) {
			continue;

		} 
        if (strcmp(line->commands[0].argv[0],"cd")==0)
            exec_cd();
            
        else
            exec_commands();

        printf("msh> ");

    }        

}