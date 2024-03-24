//helo 
//sin argumento y con un solo argumento
//prompt> ./wish ( interactive mode )
//prompt> ./wish batch.txt ( batch mode,se lee desde batch file)



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n\a"

char *builtin_commands[] = {"exit", "cd", "path"};
char *path_directories[MAX_ARGS]; // Variable global para almacenar los directorios de búsqueda

void wish_loop();
char *wish_read_line();
char **wish_split_line(char *line);
int wish_execute(char **args);
int wish_launch(char **args);
int wish_cd(char **args);
int wish_path(char **args);
void wish_error(const char *msg);

int main(int argc, char **argv) {
    // Inicializar la variable global de path con /bin
    path_directories[0] = "/bin";
    path_directories[1] = NULL;

    // Comprobar si se proporciona un archivo en batch mode
    if (argc > 2) {
        wish_error("An error has occurred");
        exit(1);
    } else if (argc == 2) {
        // Batch mode
        FILE *batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            wish_error("An error has occurred");
            exit(1);
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&line, &len, batch_file)) != -1) {
            // Elimina el salto de línea si existe
            if (line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }

            // Parsea la línea para obtener los argumentos del comando
            char **args = wish_split_line(line);

            // Ejecuta el comando
            wish_execute(args);

            // Libera la memoria asignada
            free(args);
        }

        // Cierra el archivo
        fclose(batch_file);

        // Salida después del modo batch
        exit(0);
    } else {
        // Interactive mode
        wish_loop();
    }

    return 0;
}

void wish_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("wish> ");
        line = wish_read_line();
        args = wish_split_line(line);
        status = wish_execute(args);

        free(line);
        free(args);
    } while (status);
}

char *wish_read_line() {
    char *line = NULL;
    size_t bufsize = 0; // Ajustar automáticamente el tamaño del buffer
    getline(&line, &bufsize, stdin);
    return line;
}

char **wish_split_line(char *line) {
    int bufsize = MAX_ARGS, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        wish_error("An error has occurred");
        exit(1);
    }

    token = strtok(line, DELIMITERS);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += MAX_ARGS;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                wish_error("An error has occurred");
                exit(1);
            }
        }

        token = strtok(NULL, DELIMITERS);
    }
    tokens[position] = NULL;
    return tokens;
}

int wish_execute(char **args) {
    if (args[0] == NULL) {
        return 1; // Línea vacía
    }

    if (strcmp(args[0], "exit") == 0) {
        // Verificar si se pasan argumentos al comando exit
        if (args[1] != NULL) {
            wish_error("An error has occurred");
            return 1;
        }
        return 0; // Salir del shell
    }

    for (int i = 0; i < sizeof(builtin_commands) / sizeof(char*); i++) {
        if (strcmp(args[0], builtin_commands[i]) == 0) {
            switch (i) {
                case 1:
                    return wish_cd(args);
                case 2:
                    return wish_path(args);
            }
        }
    }

    return wish_launch(args);
}

int wish_launch(char **args) {
    pid_t pid;
    int status;
    int out_redirect_index = -1;

    // Buscar el índice de la redirección de salida
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            out_redirect_index = i;
            break;
        }
    }

    pid = fork();
    if (pid == 0) {
        // Proceso hijo
        // Verificar y manejar la redirección de salida si es necesario
        if (out_redirect_index >= 0) {
            // Redirigir la salida estándar a cada archivo especificado
            for (int i = out_redirect_index + 1; args[i] != NULL; i++) {
                FILE *output_file = freopen(args[i], "w", stdout);
                if (output_file == NULL) {
                    // Si no se puede abrir el archivo, imprimir un mensaje de error
                    wish_error("An error has occurred");
                    exit(1);
                }
            }
            // Eliminar los argumentos de redirección de salida y archivos de la lista de argumentos
            args[out_redirect_index] = NULL;
        }

        // Buscar el ejecutable en los directorios de búsqueda
        for (int i = 0; path_directories[i] != NULL; i++) {
            char path[MAX_INPUT_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", path_directories[i], args[0]);
            if (access(path, X_OK) == 0) {
                execv(path, args);
                // Si execv() falla, imprimir un mensaje de error y salir
                wish_error("An error has occurred");
                exit(1);
            }
        }
        // Si el ejecutable no se encuentra en ninguno de los directorios de búsqueda
        wish_error("An error has occurred");
        exit(1);
    } else if (pid < 0) {
        // Error en el fork
        wish_error("An error has occurred");
    } else {
        // Proceso padre
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int wish_cd(char **args) {
    if (args[1] == NULL) {
        // No se proporcionaron argumentos para cd
        wish_error("An error has occurred");
        return 1; // Devuelve un código de error
    } else {
        // Se proporcionó un argumento, intenta cambiar de directorio
        if (chdir(args[1]) != 0) {
            // Error al cambiar de directorio
            wish_error("An error has occurred");
            return 1; // Devuelve un código de error
        }
    }
    return 0; // Éxito
}



int wish_path(char **args) {
    int i = 0;
    while (args[++i] != NULL) {
        path_directories[i - 1] = args[i];
    }
    path_directories[i - 1] = NULL;
    return 1;
}

void wish_error(const char *message) {
    fprintf(stderr, "%s\n", message);
}
