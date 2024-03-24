#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

void imprimir_tiempo_transcurrido(struct timeval tiempo_inicio, struct timeval tiempo_fin) {
  double tiempo_transcurrido = (tiempo_fin.tv_sec - tiempo_inicio.tv_sec) * 1000.0;  // Segundos a milisegundos
  tiempo_transcurrido += (tiempo_fin.tv_usec - tiempo_inicio.tv_usec) / 1000.0;  // Microsegundos a milisegundos
  printf("Tiempo transcurrido: %.3f ms\n", tiempo_transcurrido);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
    return 1;
  }

  struct timeval tiempo_inicio, tiempo_fin;
  pid_t pid_hijo;

  // Obtener tiempo inicial
  gettimeofday(&tiempo_inicio, NULL);

  // Crear un proceso hijo
  pid_hijo = fork();
  if (pid_hijo == -1) {
    perror("Error al crear el proceso hijo");
    return 1;
  }

  if (pid_hijo == 0) {
    // Ejecutar el comando en el proceso hijo
    execvp(argv[1], argv + 1);
    perror("Error al ejecutar el comando");
    exit(1);
  } else {
    // Esperar a que el proceso hijo termine
    wait(NULL);

    // Obtener tiempo final
    gettimeofday(&tiempo_fin, NULL);

    // Calcular y mostrar el tiempo transcurrido
    imprimir_tiempo_transcurrido(tiempo_inicio, tiempo_fin);
  }

  return 0;
}
