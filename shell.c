#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

void managment_SIGCHILD(int s){
        //SIGCHILD = signal sent to a parent process when a child process stops, continues, or terminates
        (void) s;
        int status;
        while(waitpid(-1,&status,WNOHANG) > 0);//si algun hijo ya termino recogerlo
        //WNOHANG = informa si no ha salido un hijo
}
void inicializador_managment(void){
        //sigaction man 2i function froma manual, already define
        struct sigaction sa;
        sa.sa_handler = managment_SIGCHILD;     //que va hacer
        sigemptyset(&sa.sa_mask);               //vacia, sin signal bloqueada
        sa.sa_flags = SA_RESTART;               //Reinicia la interrupcion de sistema
        sigaction(SIGCHLD,&sa,NULL);            //
}
int main(){
        inicializador_managment();
        char cmd[80];
        char *args[10];//manipularemos este para evitar problemas de memoria por la modificacion de strtok
        int p;

        printf("shell con exec, pid = %d\n",getpid());
        //exec, llamada a sistema con variantes

        char delim[] = " ,\t";//delimitadores para el shell
        int is_background, i;

        while(1){
                printf(">> ");

                fgets(cmd,80,stdin);
                cmd[strcspn(cmd,"\r\n")] = '\0';

                if(strcmp(cmd,"exit") == 0){
                        break;
                }

                i= 0;
                args[i] = strtok(cmd, delim);//separamos las palabras problemas al compilar en un punto
                while(args[i] != NULL){
                        args[++i]  = strtok(NULL, delim);
                }

                if(i == 0) continue;

                is_background = 0;
                if(strcmp(args[i - 1],"&") == 0){
                        //& handle for background task
                        is_background = 1;
                        args[i - 1] = NULL;
                }

                p = fork();

                if(p == 0){
                        execvp(args[0], args);//este proceso no crea otro hijo como en system
                        //remplaza la imagen del proceso por otro proceso
                }else{
                        if(is_background == 1){
                                printf("Background process [PID] == %d\n",p);
                        }else{
                                waitpid(p, NULL, 0);
                        }
                }
                //agregamos un wait porque el padre no estaba esperando al hijo
                //wait(NULL);
        }
        printf("FIn de la sesion \n");
}
