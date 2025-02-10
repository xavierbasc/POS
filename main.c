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
#include "draw.h"
#include <math.h>

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
#define FIELD_LABEL_WIDTH 14

const char *labels[NUM_FIELDS] = {
    "CODE", "ID", "Producto", "Stock", "Fabricante", "Proveedor",
    "Departamento", "Clase", "Subclase", "Descripción", "Precio", "IVA"
};

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


TextField *f_code;
TextField *f_producto;
TextField *f_stock;
TextField *f_fabricante;
TextField *f_proveedor;
TextField *f_departamento;
TextField *f_clase;
TextField *f_subclase;
TextField *f_descripcion;

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

/**
 * my_getch: Captura un carácter sin mostrarlo en pantalla.
 * 
 * - En Windows, utiliza _getch() de <conio.h>. Si se detecta una tecla extendida
 *   (cuando _getch() retorna 0 o 224), se lee el siguiente carácter y se combinan ambos.
 * 
 * - En Linux/macOS, se invoca el comando "stty raw -echo" para poner la terminal en modo raw
 *   sin eco, se lee un carácter con getchar(), y luego se restaura la configuración normal
 *   con "stty sane". Si se detecta que el carácter es ESC (27), se asume que es el inicio de
 *   una secuencia de escape (por ejemplo, para teclas de función) y se leen dos caracteres más,
 *   combinándolos en un entero.
 */
int getch(void) {
#ifdef _WIN32
    int ch = _getch();
    if (ch == 0 || ch == 224) {  // Tecla extendida
        int ch2 = _getch();
        // Combina ambos bytes en un entero (por ejemplo, para distinguir F1, F2, etc.)
        return (ch << 8) | ch2;
    }
    return ch;
#else
    int ch;
    // Cambiar la terminal a modo raw sin eco (requiere que el sistema disponga de stty)
    system("stty raw -echo");
    ch = getchar();
    if (ch == 27) {  // Si se detecta ESC, se asume que es el comienzo de una secuencia de escape
        int ch2 = getchar();
        int ch3 = getchar();
        ch = (ch << 16) | (ch2 << 8) | ch3;
    }
    // Restaurar el modo normal de la terminal
    system("stty sane");
    return ch;
#endif
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

void set_foreground_256_hex(unsigned int hex_color) {
    // Extrae los componentes del color
    int r = (hex_color >> 16) & 0xFF;  // Componente rojo
    int g = (hex_color >> 8)  & 0xFF;   // Componente verde
    int b = hex_color         & 0xFF;   // Componente azul

    // Convierte cada componente al rango 0-5
    int r_index = (int)round(r / 51.0);
    int g_index = (int)round(g / 51.0);
    int b_index = (int)round(b / 51.0);

    // Calcula el índice en la paleta ANSI de 256 colores
    int color_index = 16 + (r_index * 36) + (g_index * 6) + b_index;

    // Establece el color usando la secuencia ANSI para 256 colores
    printf("\033[38;5;%dm", color_index);
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
void draw_text_field(TextField *tf, int underline) {
    if (!tf) return;
    gotoxy(tf->row, tf->col);
    if (underline == 1) printf("\033[24m");// desactiva subrayado

    int len = strlen(tf->label);

    for (int i = 0; i < FIELD_LABEL_WIDTH-1; i++) {
        if (i < len)
            putchar(tf->label[i]);
        else
            putchar('.');  // Completa con puntos
    }
    putchar(':');


    gotoxy(tf->row, tf->col + FIELD_LABEL_WIDTH);
    if (underline == 1) printf("\033[4m"); // activa subrayado
    printf("%-*s", tf->width, tf->buffer);
    if (underline == 1) printf("\033[24m");// desactiva subrayado
}

/* Edita el campo de texto permitiendo mover el cursor con las flechas.
 * Se captura carácter a carácter hasta que se presione Enter.
 */
void edit_text_field(TextField *tf) {
    gotoxy(tf->row, tf->col + tf->cursor + FIELD_LABEL_WIDTH);

    if (!tf) return;
    
    // En sistemas Unix, habilitar modo raw para capturar cada tecla sin esperar Enter
#ifndef _WIN32
    enable_raw_mode();
#endif

    // Cambiar color para indicar edición (ejemplo: texto blanco sobre fondo azul oscuro)
    //set_colors_rgb(255, 255, 255, 0, 0, 128);

    set_foreground_256_hex(green);

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
        draw_text_field(tf, 0);
        // Reposiciona el cursor en la posición actual dentro del campo
        gotoxy(tf->row, tf->col + tf->cursor + FIELD_LABEL_WIDTH);
        fflush(stdout);
    }
    
    // Finaliza la edición: restablece atributos
#ifndef _WIN32
    disable_raw_mode();
#endif
    reset_colors();
    draw_text_field(tf, 0);
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
void update_ui(void) {
    TerminalSize ts = get_terminal_size();
    
    clear_screen();
    hide_cursor();
    
    /* Dibuja un recuadro que abarque toda la terminal */
    set_foreground_256_hex(dark_green);
    draw_box(1, 2, ts.cols, ts.rows-1);
    
    /* Título en la parte superior */
    gotoxy(1, 3);
    set_foreground_256_hex(dark_green);
    printf("Tamaño: %d x %d", ts.cols, ts.rows);
    //reset_colors();
    
    /* Instrucciones justo debajo del título */
    //gotoxy(4, 3);
    //printf("Comandos: 'e' - Editar campo, 'q' - Salir");
    
    /* Ubica el campo de texto en el centro horizontal y a 1/3 vertical */
    //int field_row = ts.rows / 3;
    //int field_col = (ts.cols - tf->width) / 2;
    //tf->row = field_row;
    //tf->col = field_col;

    set_foreground_256_hex(dark_green);
    draw_text_field(f_code, 1);
    draw_text_field(f_producto, 1);
    draw_text_field(f_stock, 1);
    draw_text_field(f_fabricante, 1);
    draw_text_field(f_proveedor, 1);
    draw_text_field(f_departamento, 1);
    draw_text_field(f_clase, 1);
    draw_text_field(f_subclase, 1);
    draw_text_field(f_descripcion, 1);

    /* Posiciona el cursor en la zona de comandos (parte inferior) */
    gotoxy(ts.rows - 2, 3);
    fflush(stdout);
    show_cursor();
}

int process_input(TextField *tf) {
    char command[100];
    int i = 0;
    TerminalSize ts = get_terminal_size();

    // Muestra el prompt en la ubicación deseada (por ejemplo, última fila, columna 3)
    gotoxy(ts.rows, 3);
    printf("Ingrese comando: ");
    fflush(stdout);

    int ch;
    while (1) {
        ch = getch();
        // Si se presiona ENTER (puede ser '\r' o '\n')
        if (ch == '\r' || ch == '\n')
            break;

        // Manejo de retroceso (backspace, ASCII 8 o 127)
        if (ch == 8 || ch == 127) {
            if (i > 0)
                i--;
            continue;
        }

        // Si se detecta una secuencia de escape (tecla de función u otra especial)
        // se asume que el valor combinado es mayor que 255.
        if (ch > 255) {
            /*  
             * Aquí puedes procesar la tecla de función.
             * Por ejemplo, supongamos que defines:
             *   #define F1_KEY ((27 << 16) | (91 << 8) | 49)  // Ejemplo de código para F1
             *
             * Puedes agregar casos para otras teclas de función.
             */
            // Ejemplo de manejo (solo se imprime en debug, sin mostrar al usuario):
            // if (ch == F1_KEY) { ... }
            continue;  // O bien, manejar la tecla según convenga.
        }

        // Almacenamos el carácter en el buffer
        if (i < (int)sizeof(command) - 1)
            command[i++] = (char)ch;
    }
    command[i] = '\0';

    // Limpia la línea del prompt y la entrada (secuencia ANSI "\033[2K" borra la línea completa).
    gotoxy(ts.rows, 3);
    printf("\033[2K");
    fflush(stdout);

    // Procesa el comando ingresado.
    if (strcmp(command, "q") == 0 || strcmp(command, "Q") == 0)
        return 1;  // Comando para salir
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
    //    "CODE", "ID", "Producto", "Stock", "Fabricante", "Proveedor", "Departamento", "Clase", "Subclase", "Descripción", "Precio", "IVA"
    

    f_code         = create_text_field( 4, 3, 30, 100, "CODE");
    f_producto     = create_text_field( 6, 3, 30, 100, "Producto");
    f_stock        = create_text_field( 8, 3, 30, 100, "Stock");
    f_fabricante   = create_text_field(10, 3, 30, 100, "Fabricante");
    f_proveedor    = create_text_field(12, 3, 30, 100, "Proveedor");
    f_departamento = create_text_field(14, 3, 30, 100, "Departamento");
    f_clase        = create_text_field(16, 3, 30, 100, "Clase");
    f_subclase     = create_text_field(18, 3, 30, 100, "Subclase");
    f_descripcion  = create_text_field(20, 3, 30, 100, "Descripcion");

    while (!exit_requested) {
        update_ui();
        exit_requested = process_input(f_code);
    }
    
    clear_screen();
    show_cursor();
    free_text_field(f_code);
    free_text_field(f_producto);
    free_text_field(f_stock);
    free_text_field(f_fabricante);
    free_text_field(f_proveedor);
    free_text_field(f_departamento);
    free_text_field(f_clase);
    free_text_field(f_subclase);
    free_text_field(f_descripcion);
    
    return 0;
}
