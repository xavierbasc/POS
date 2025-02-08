/*
 * Programa de ejemplo para UI de texto con manejo de colores, detección del tamaño de terminal,
 * campos de texto subrayados y edición con soporte para flechas de dirección.
 *
 * Compilación:
 *   Linux/macOS:
 *      gcc -Os -s -flto -o text_ui text_ui.c
 *
 *   Windows (MinGW):
 *      gcc -Os -s -flto -o text_ui.exe text_ui.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <termios.h>
#endif

/* =========================== */
/* Definición para tamaño de terminal */
/* =========================== */
typedef struct {
    int rows;
    int cols;
} TerminalSize;

#define NUM_FIELDS 12
#define FIELD_LABEL_WIDTH 10

const char *labels[NUM_FIELDS] = {
    "CODE", "ID", "Producto", "Stock", "Fabricante", "Proveedor",
    "Departamento", "Clase", "Subclase", "Descripción", "Precio", "IVA"
};


/* Obtiene el tamaño de la terminal */
TerminalSize get_terminal_size(void) {
    TerminalSize ts = {0, 0};
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        ts.cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        ts.rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
        ts.cols = w.ws_col;
        ts.rows = w.ws_row;
    }
#endif
    return ts;
}

/* =========================== */
/* Modo Raw para Unix */
/* =========================== */
#ifndef _WIN32
struct termios orig_termios;

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void enable_raw_mode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // deshabilitar entrada canónica y eco
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}
#endif

/* =========================== */
/* Función getch() para capturar un carácter */
/* =========================== */
#ifdef _WIN32
// En Windows usamos _getch() de <conio.h>
int getch_wrapper(void) {
    return _getch();
}
#else
// En Unix implementamos getch usando termios
int getch_wrapper(void) {
    int ch;
    ch = getchar();
    return ch;
}
#endif

/* =========================== */
/* Funciones para manejo ANSI  */
/* =========================== */

/* Habilita el procesamiento de secuencias ANSI en Windows */
void enable_ansi_escape_codes(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;
    
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return;
    
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

/* Limpia la pantalla y posiciona el cursor en la esquina superior izquierda */
void clear_screen(void) {
    printf("\033[2J\033[H");
}

/* Posiciona el cursor en la fila 'row' y columna 'col' (1-indexado) */
void gotoxy(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

/* =========================== */
/* Funciones para manejo de colores */
/* =========================== */

/* --- Colores Básicos (0-7) --- */
void set_foreground_color(int color) {
    printf("\033[3%dm", color);
}

void set_background_color(int color) {
    printf("\033[4%dm", color);
}

void set_colors(int fg, int bg) {
    set_foreground_color(fg);
    set_background_color(bg);
}

/* --- Paleta de 256 Colores --- */
void set_foreground_color256(int color) {
    printf("\033[38;5;%dm", color);
}

void set_background_color256(int color) {
    printf("\033[48;5;%dm", color);
}

void set_colors256(int fg, int bg) {
    set_foreground_color256(fg);
    set_background_color256(bg);
}

/* --- Colores Verdaderos (RGB) --- */
void set_foreground_rgb(int r, int g, int b) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
}

void set_background_rgb(int r, int g, int b) {
    printf("\033[48;2;%d;%d;%dm", r, g, b);
}

void set_colors_rgb(int r_fg, int g_fg, int b_fg, int r_bg, int g_bg, int b_bg) {
    set_foreground_rgb(r_fg, g_fg, b_fg);
    set_background_rgb(r_bg, g_bg, b_bg);
}

void reset_colors(void) {
    printf("\033[0m");
}

/* =========================== */
/* Funciones para manejo del cursor */
/* =========================== */
void hide_cursor(void) {
    printf("\033[?25l");
}

void show_cursor(void) {
    printf("\033[?25h");
}

/* =========================== */
/* Funciones para dibujo de UI */
/* =========================== */
void draw_box(int x1, int y1, int x2, int y2) {
    int i, j;
    gotoxy(y1, x1);
    printf("┌");
    for (i = x1 + 1; i < x2; i++) {
        printf("─");
    }
    printf("┐");
    for (j = y1 + 1; j < y2; j++) {
        gotoxy(j, x1);
        printf("│");
        gotoxy(j, x2);
        printf("│");
    }
    gotoxy(y2, x1);
    printf("└");
    for (i = x1 + 1; i < x2; i++) {
        printf("─");
    }
    printf("┘");
}

/* =========================== */
/* Funciones para campos de texto */
/* =========================== */

/* Estructura para representar un campo de texto.
 * Se agregó 'cursor' para almacenar la posición de edición dentro del buffer.
 */
typedef struct {
    int row;      // Fila donde se ubica el campo (1-indexado)
    int col;      // Columna donde se ubica el campo (1-indexado)
    int width;    // Ancho visible del campo (en caracteres)
    int maxlen;   // Longitud máxima permitida para la entrada
    char *buffer; // Buffer que almacena el contenido del campo
    int cursor;   // Posición actual del cursor en el buffer (0-indexado)
    const char *label; // Etiqueta del campo (a la izquierda)
} TextField;

TextField *create_text_field(int row, int col, int width, int maxlen, const char *label) {
    TextField *tf = malloc(sizeof(TextField));
    if (!tf) return NULL;
    tf->row = row;
    tf->col = col;
    tf->width = width;
    tf->maxlen = maxlen;
    tf->buffer = calloc(maxlen + 1, sizeof(char));
    tf->cursor = 0;
    tf->label = label;
    return tf;
}

/* Dibuja el campo de texto con subrayado.
 * Se imprime el contenido rellenado hasta 'width' caracteres.
 */
void draw_text_field(TextField *tf) {
    if (!tf) return;
    gotoxy(tf->row, tf->col);
    printf("%-10s", tf->label);

    gotoxy(tf->row, tf->col + FIELD_LABEL_WIDTH);
    printf("\033[4m"); // activa subrayado
    printf("%-*s", tf->width, tf->buffer);
    printf("\033[0m"); // desactiva atributos
    fflush(stdout);
}

/* Edita el campo de texto permitiendo mover el cursor con las flechas.
 * Se captura carácter a carácter hasta que se presione Enter.
 */
void edit_text_field(TextField *tf) {
    if (!tf) return;
    
    // En sistemas Unix, habilitar modo raw para capturar cada tecla sin esperar Enter
#ifndef _WIN32
    enable_raw_mode();
#endif

    // Cambiar color para indicar edición (ejemplo: texto blanco sobre fondo azul oscuro)
    set_colors_rgb(255, 255, 255, 0, 0, 128);
    // Activar subrayado para el área de edición
    printf("\033[4m");
    fflush(stdout);
    
    int ch;
    while (1) {
        ch = getch_wrapper();
        if (ch == '\r' || ch == '\n') {
            // Finaliza la edición con Enter
            break;
        }
        else if (ch == 127 || ch == 8) { 
            // Backspace: eliminar carácter anterior si existe
            if (tf->cursor > 0) {
                int len = strlen(tf->buffer);
                for (int i = tf->cursor - 1; i < len; i++) {
                    tf->buffer[i] = tf->buffer[i + 1];
                }
                tf->cursor--;
            }
        }
        else if (ch == 27) {
            // Secuencia de escape: posiblemente una tecla de flecha
            int ch2 = getch_wrapper();
            if (ch2 == '[') {
                int ch3 = getch_wrapper();
                if (ch3 == 'C') {
                    // Flecha derecha
                    if (tf->cursor < (int)strlen(tf->buffer))
                        tf->cursor++;
                }
                else if (ch3 == 'D') {
                    // Flecha izquierda
                    if (tf->cursor > 0)
                        tf->cursor--;
                }
                // Se pueden agregar casos para flechas arriba/abajo si se desea
            }
        }
        else if (ch >= 32 && ch <= 126) {
            // Carácter imprimible: inserción en la posición actual
            int len = strlen(tf->buffer);
            if (len < tf->maxlen) {
                // Desplazar a la derecha desde la posición actual
                for (int i = len; i >= tf->cursor; i--) {
                    tf->buffer[i+1] = tf->buffer[i];
                }
                tf->buffer[tf->cursor] = (char)ch;
                tf->cursor++;
            }
        }
        
        // Actualiza el campo en pantalla
        draw_text_field(tf);
        // Reposiciona el cursor en la posición actual dentro del campo
        gotoxy(tf->row, tf->col + tf->cursor + FIELD_LABEL_WIDTH);
        fflush(stdout);
    }
    
    // Finaliza la edición: restablece atributos
#ifndef _WIN32
    disable_raw_mode();
#endif
    reset_colors();
    draw_text_field(tf);
}

/* Libera la memoria asignada al campo de texto */
void free_text_field(TextField *tf) {
    if (tf) {
        free(tf->buffer);
        free(tf);
    }
}

/* =========================== */
/* Funciones de la UI          */
/* =========================== */

/* Actualiza la interfaz principal adaptándola al tamaño de la terminal */
void update_ui(TextField *tf) {
    TerminalSize ts = get_terminal_size();
    
    clear_screen();
    hide_cursor();
    
    /* Dibuja un recuadro que abarque toda la terminal */
    draw_box(1, 1, ts.cols, ts.rows);
    
    /* Título en la parte superior */
    gotoxy(2, 3);
    set_foreground_color(2);  // Verde (color básico)
    printf("Mi Aplicación de UI de Texto (Tamaño: %d x %d)", ts.cols, ts.rows);
    reset_colors();
    
    /* Instrucciones justo debajo del título */
    gotoxy(4, 3);
    printf("Comandos: 'e' - Editar campo, 'q' - Salir");
    
    /* Ubica el campo de texto en el centro horizontal y a 1/3 vertical */
    int field_row = ts.rows / 3;
    int field_col = (ts.cols - tf->width) / 2;
    tf->row = field_row;
    tf->col = field_col;
    draw_text_field(tf);
    
    /* Posiciona el cursor en la zona de comandos (parte inferior) */
    gotoxy(ts.rows - 2, 3);
    fflush(stdout);
    show_cursor();
}

/* Procesa la entrada de comandos: 'e' para editar y 'q' para salir */
int process_input(TextField *tf) {
    char command[100];
    TerminalSize ts = get_terminal_size();
    
    gotoxy(ts.rows - 2, 3);
    printf("Ingrese comando: ");
    fflush(stdout);
    
    if (fgets(command, sizeof(command), stdin) == NULL)
        return 1;  // fin o error
    
    command[strcspn(command, "\n")] = '\0';
    
    if (strcmp(command, "q") == 0 || strcmp(command, "Q") == 0)
        return 1;
    else if (strcmp(command, "e") == 0 || strcmp(command, "E") == 0)
        edit_text_field(tf);
    
    return 0;
}

/* =========================== */
/* Función principal           */
/* =========================== */
int main(void) {
    int exit_requested = 0;
    
    enable_ansi_escape_codes();
    
    /* Crear un campo de texto con ancho de 30 y máximo 100 caracteres.
     * La posición se ajusta en update_ui().
     */
    TextField *tf = create_text_field(0, 0, 30, 100, "Prueba");
    
    while (!exit_requested) {
        update_ui(tf);
        exit_requested = process_input(tf);
    }
    
    clear_screen();
    show_cursor();
    free_text_field(tf);
    
    return 0;
}
