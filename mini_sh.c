#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

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
	struct sigaction sa;

        sa.sa_handler = managment_SIGCHILD;     //Funcion a ejecutar (que va hacer)
        sigemptyset(&sa.sa_mask);               //No bloquea nada, 	vacia, sin signal bloqueada
        sa.sa_flags = SA_RESTART;               //Reinicia la interrupcion de sistema automaticamente

	sigaction(SIGCHLD,&sa,NULL);            //Configuracion al sistema
}

void handler_SIGINT(int signum){
	//Funcion para manejar la señal de CTRL+C
	//En vez de interrumpir, salto de linea
        (void) signum;                          //para evitar warnings
	printf("\n>> ");
        fflush(stdout);
}

//tokenizador mas especifico que strtok para soportar '', ""
int tokenizador(char *linea, char *tokens[]){
        int cont = 0;
        char *p = linea;

        //recorremos toda la linea
        while(*p != '\0'){

                //ignoramos espacios, tabulacion y ,
                while(*p == ' ' || *p == '\t' || *p == ',') p++; 
                
                //volvemos a verificar que aun no llegamos al final de linea
                if(*p == '\0') break;

                //caso: tenemos comillas
                if(*p == '"' || *p == '\''){
                        char comilla = *p++;
                        tokens[cont++] = p;

                        //mientras no lleguemos al fin ni a la comilla que cierra, seguimos avanzando
                        while(*p != '\0' && *p != comilla) 
                                p++;
                        if(*p != '\0'){
                                *p = '\0';
                                p++;
                        }
                }
                //caso sin comillas
                else{
                        tokens[cont++] = p;
                        while(*p != '\0' && *p != ' ' && *p != '\t' && *p != ',')
                                p++;
                        if(*p != '\0'){
                                *p = '\0';
                                p++;
                        }
                }
        }

        tokens[cont] = NULL;                   //marcamos el final de los tokens
        return cont;
}

//para redirecciones de entrada < salida > y error 2>
void handler_redireccion(char *args[]){
        for(int i =0; args[i]!=NULL; i++){
                if(strcmp(args[i], ">") == 0){
                        int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if(fd < 0){
                                perror("open >");
                                exit(1);
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        args[i] = NULL;
                        break;
                }
                else if (strcmp(args[i], "<") == 0){
                        int fd = open(args[i+1], O_RDONLY);
                        if(fd < 0){
                                perror("open <");
                                exit(1);
                        }
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                        args[i] = NULL;
                        break;
                }
                else if (strcmp(args[i], "2>") == 0){
                        int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if(fd <0){
                                perror("open 2>");
                                exit(1);
                        }
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                        args[i] = NULL;
                        break;
                }
        }
}

//comandos compuestosc con pipeline|
//agregamos el is_background para saber si ejecutar en segundo plano
int pipeline(char *args_left[], char *args_right[], int is_background){
        int pipefd[2];
        if(pipe(pipefd) < 0){
                perror("pipe");
                return -1;
        }

        pid_t p1 = fork();
        if(p1 == 0){
                signal(SIGINT, SIG_DFL);
                close(pipefd[0]);                       //cierra lectura
                dup2(pipefd[1], STDOUT_FILENO);         //redirige salida hacia el pipe
                close(pipefd[1]);
                handler_redireccion(args_left);
                execvp(args_left[0], args_left);
                perror("execvp pipe izq");
                exit(1);
        }

        pid_t p2 = fork();
        if(p2 == 0){
                signal(SIGINT, SIG_DFL);
                close(pipefd[1]);                       //cierra escritura
                dup2(pipefd[0], STDIN_FILENO);          //redirige la entrada desde el pipe
                close(pipefd[0]);
                handler_redireccion(args_right);
                execvp(args_right[0], args_right);
                perror("execvp pipe der");
                exit(1);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        if(is_background){
                printf("Background process [PID] == %d\n", p1);
                printf("Background process [PID] == %d\n", p2);
        }
        else{
                waitpid(p1, NULL, 0);
                waitpid(p2, NULL, 0);
        }
        return 0;
}

int main(){
        inicializador_managment();		//Inicializar la funcion para zombies
	signal(SIGINT, handler_SIGINT);		//Inicializar para CTRL+C

        char cmd[80];				//Lo recibido por el usuario
        char *args[20];				//manipularemos este para evitar problemas de memoria por la modificacion de strtok
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

                int cant_tokens = tokenizador(cmd, args);
                if(!cant_tokens) continue;      //linea vacia

                /*
                i= 0;
		
		//separamos las palabras problemas al compilar en un punto
                args[i] = strtok(cmd, delim);		
                
		while(args[i] != NULL){
                        args[++i]  = strtok(NULL, delim);
                }
                */

                is_background = 0;
                if(strcmp(args[cant_tokens - 1],"&") == 0){
                        //& handle for background task
                        is_background = 1;
                        args[cant_tokens - 1] = NULL;	//quitamos & para evitar problemas
                        cant_tokens--;
                }


                //BUSCAMOS UN PIPE
                int pipe_i = -1;
                for(int k=0; args[k] != NULL; k++){
                        if(strcmp(args[k], "|") == 0){
                                pipe_i = k;
                                break;
                        }
                }

                //Si si hay pipe
                if(pipe_i != -1){
                        char *args_left[20];
                        char *args_right[20];

                        //copiamos argumentos
                        for(int i=0; i<pipe_i; i++)
                                args_left[i] = args[i];
                        
                        args_left[pipe_i] = NULL;

                        int j;
                        for(j=0; args[pipe_i+j+1] != NULL; j++)
                                args_right[j] = args[j+pipe_i+1];

                        args_right[j] = NULL;

                        pipeline(args_left, args_right, is_background);
                        continue;
                }
		
                p = fork();		//Hijo

                if(p == 0){
                        signal(SIGINT, SIG_DFL);        //restauramos ctrl + c en el hijo

                        handler_redireccion(args);      //manejamos redirecciones antes de continuar

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
        printf("Fin de la sesion \n");
}
