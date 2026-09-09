# Minishell
In this practice for the Operating System Fundamentals class, we will implement a minishell
Para compilar: 
  gcc -o mini_sh mini_sh.c

Para ejecutar:
  ./mini_sh

DECISIONES DE DISEÑO
  - Procesos zombie.
  - Ctrl + C.
  - PARSING DE ENTRADA. Implementamos un parser propio para tokens en lugar de strtok, puesto que con los delimitadores como espacios y comas este dividía en varios argumentos una sola cadena de texto encerrada por "" o '', lo cual debía ser un solo argumento.
  - REDIRECCIONES. Con ayuda de la biblioteca fcntrl (file control) y una función handler_redireccion() si detectamos < se abre en modo lectura, con > o 2> abre con O_CREAT | O_TRUNC y redirigimos el flujo con dup2() para reconectar la salida estándar o entrada estándar hacia el archivo especificado. Finalmente se reemplaza el símbolo con NULL para limpiar la lista de argumentos antes de invocar execvp().
  - COMPOSICIÓN CON PIPE. Se canalizan 2 procesos o comandos con '|' invocando a dos hijos con fork() y usando dup2() para conectar la salida del primer hijo al lado de escritura y la entrada del segundo hijo al lado de lectura, para que de esta forma el segundo hijo procese los datos que va generando el primero.

LIMITACIONES
Entre las limitaciones tenemos primero que la implementación del pipeline soporta hasta 2 comandos; uno a la izquierda y otro a la derecha. No soporta enlaces entre 3 o más comandos.
Así también 'exit' es el único comando integrado, puesto que no implementamos cd para la navegación entre directorios.
Entre otras limitaciones relevantes tenemos que no se diseñó una validación sintáctica ni mensajes de advertencia, por lo que comandos mal estructurados podrían generar comportamientos indefinidos.
