#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

void managment_SIGCHILD(int s){
	//Funcion para recoger a los hijos zombies terminados de fondo
	//evitando que se queden ahi
	
        //SIGCHILD = signal sent to a parent process when a child process stops, continues, or terminates
        //WNOHANG = informa si no ha salido un hijo, no bloquea si no hay hijo terminado
	
	(void) s;					//Se usa para evitar avisos, no se usa
        int status;					//Almacena la informacion de salida del proceso hijo
				
        while(waitpid(-1,&status,WNOHANG) > 0);		//Bucle para recoger los hijos terminados
}


//sigaction man 2i function froma manual, already define
//Mas explicito que signal
void inicializador_managment(void){
	//Funcion complementaria para recoger procesos zombies
	struct sigaction sa;i

        sa.sa_handler = managment_SIGCHILD;     //Funcion a ejecutar (que va hacer)
        sigemptyset(&sa.sa_mask);               //No bloquea nada, 	vacia, sin signal bloqueada
        sa.sa_flags = SA_RESTART;               //Reinicia la interrupcion de sistema automaticamente

	sigaction(SIGCHLD,&sa,NULL);            //Configuracion al sistema
}

void handler_SIGINT(int signum){
	//Funcion para manejar la señal de CTRL+C
	//En vez de interrumpir, salto de linea
	printf("\n");
}

int main(){
        inicializador_managment();		//Inicializar la funcion para zombies
	signal(SIGINT, handler_SIGINT);		//Inicializar para CTRL+Ci

        char cmd[80];				//Lo recibido por el usuario
        char *args[10];				//manipularemos este para evitar problemas de memoria por la modificacion de strtok
        int p;					//pid


        char delim[] = " ,\t";			//delimitadores para el shell [con strtok]
        int is_background, i;			//bandera background	| indice de tokens

        while(1){
                printf(">> ");

                fgets(cmd,80,stdin);
                cmd[strcspn(cmd,"\r\n")] = '\0';	//Eliminar saltos de linea

                if(strcmp(cmd,"exit") == 0){
			//salir del shell
                        break;
                }

                i= 0;
		
		//separamos las palabras problemas al compilar en un punto
                args[i] = strtok(cmd, delim);		
                
		while(args[i] != NULL){
                        args[++i]  = strtok(NULL, delim);
                }

                if(i == 0) continue;		//Linea vacia sigue

                is_background = 0;
                if(strcmp(args[i - 1],"&") == 0){
                        //& handle for background task
                        is_background = 1;
                        args[i - 1] = NULL;	//quitamos & para evitar problemas
                }
		
                p = fork();		//Hijo

                if(p == 0){
                        execvp(args[0], args);		//este proceso no crea otro hijo como en system
				                        //remplaza la imagen del proceso por otro proceso
			//error
			perror("execvp: ");
			exit(1);

                }if(p > 0){	
                        if(is_background == 1){
				//Usamos el handler para procesos de background
                                printf("Background process [PID] == %d\n",p);
                        }else{
                    		if(waitpid(p, NULL, 0) < 0 && errno != ECHILD){		//el error es especifcamente del hijo 
					perror("waitpid");
				}
                        }
                } else{
			perror("fork");
		}
                //agregamos un wait porque el padre no estaba esperando al hijo
                //wait(NULL);
        }
        printf("FIn de la sesion \n");
}
