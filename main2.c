#include <notcurses/notcurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
  // Inicializar Notcurses.
  struct notcurses_options opts = {0};
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if (!nc) {
    fprintf(stderr, "Error al inicializar Notcurses\n");
    return EXIT_FAILURE;
  }

  // Configurar un timeout de 10 segundos.
  struct timespec ts;
  ts.tv_sec = 10;
  ts.tv_nsec = 0;

  // Declarar la variable para capturar la entrada.
  struct ncinput ni;

  // Llamada a notcurses_get con timeout.
  uint32_t key = notcurses_get(nc, &ts, &ni);

  // Se puede procesar 'key' o 'ni' según las necesidades.
  // Por ejemplo, mostrar el código de la tecla:
  ncplane_printf_aligned(notcurses_stdplane(nc), 1, NCALIGN_CENTER,
                           "Tecla presionada: 0x%x", key);
  notcurses_render(nc);

  // Espera una tecla antes de salir.
  notcurses_get(nc, NULL, &ni);

  // Finalizar Notcurses.
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
