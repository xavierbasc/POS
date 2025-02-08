/*
 * How to run:
 *   gcc -Wall -Werror -g -pedantic -o pos main.c -static -lform -lncurses
 *   macos: find $(brew --prefix) -name "libncurses.a"
 *   /usr/local/Cellar/ncurses/6.5/lib/libncurses.a
 * 
 * gcc -Wall -Werror -g -pedantic -o pos main.c -L/usr/local/Cellar/ncurses/6.5/lib -Wl,-Bstatic -lncurses -lform -Wl,-Bdynamic
 * 
 * gcc -Wall -Werror -g -pedantic -o pos main.c /usr/local/Cellar/ncurses/6.5/lib/libncurses.a /usr/local/Cellar/ncurses/6.5/lib/libform.a -L/usr/lib -lc
 */
#include <ncurses.h>
#include <form.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define N_FIELDS 25
#define X_FIELDS 17
#define Y_FIELDS 2
#define X_TICKET COLS/2 + 2
#define Y_TICKET 2

// Application mode status
#define M_POS 0
#define M_PRODUCT_EDIT 1
#define M_PRODUCT_LIST 2

// Variables globales

static FORM *f_product;
static FIELD *fields[N_FIELDS];
char username[50];

int mode = M_POS;

int print(WINDOW *win, int y, int x, const char *fmt, ...);
void topbar(void);
void fields_config(int mode);
static void driver(int ch);
FIELD * set_field(int height, int width, int toprow, int leftcol, int offscreen, int nbuffers);
void rectangle(int y1, int x1, int y2, int x2);
void vertical(int y1, int x1, int y2, int x2);
void create_form(FIELD *fields[], FORM *form);
void ticket(void);
void draw_screen(void);


void rectangle(int y1, int x1, int y2, int x2)
{
    mvhline(y1, x1, 0, x2-x1);
    mvhline(y2, x1, 0, x2-x1);
    mvvline(y1, x1, 0, y2-y1);
    mvvline(y1, x2, 0, y2-y1);
    mvaddch(y1, x1, ACS_ULCORNER);
    mvaddch(y2, x1, ACS_LLCORNER);
    mvaddch(y1, x2, ACS_URCORNER);
    mvaddch(y2, x2, ACS_LRCORNER);
}

void vertical(int y1, int x1, int y2, int x2)
{
    for (int y = 2; y < LINES - 2; y++) {
        mvaddch(y, 0, ACS_VLINE);
        mvaddch(y, COLS / 2, ACS_VLINE);
        mvaddch(y, COLS - 1, ACS_VLINE);
    }
}

int print(WINDOW *win, int y, int x, const char *fmt, ...) {
    int cur_y, cur_x;
    getyx(win, cur_y, cur_x);
    wmove(win, y, x);
    va_list args;
    va_start(args, fmt);
    int ret = vw_printw(win, fmt, args);
    va_end(args);
    wmove(win, cur_y, cur_x);
    return ret;
}

void topbar(void)
{
    //clrtoeol();
    char mode_str[50];
    switch (mode) {
        case M_POS:
            strcpy(mode_str, "POS");
            break;
        case M_PRODUCT_EDIT:
            strcpy(mode_str, "PRODUCT (EDIT)");
            break;
        case M_PRODUCT_LIST:
            strcpy(mode_str, "PRODUCT (LIST)");
            break;
    }
    attron(COLOR_PAIR(4));
    print(stdscr, 0, 2, "MODE: %-20s", mode_str);
    print(stdscr, 0, 27, "USER: %-10s", username);
}

void fields_config(int mode)
{
    int color_fields = 2;
    int color_code = 6;
    Field_Options options = O_VISIBLE | O_PUBLIC;

    switch (mode) {
        case M_POS:
            break;
        case M_PRODUCT_LIST:
        case M_PRODUCT_EDIT:
            color_fields = 5;
            options = O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE | O_NULLOK;
            break;
    };

    // CODE (EAN, etc):
    set_field_fore(fields[0], COLOR_PAIR(color_code));
    set_field_back(fields[0], COLOR_PAIR(color_code));
    for (int i = 1; fields[i] != NULL; i++)
    {
        set_field_opts(fields[i], options);
        set_field_fore(fields[i], COLOR_PAIR(color_fields));
        set_field_back(fields[i], COLOR_PAIR(color_fields));
    }
    refresh();
    // CODE FOCUS
    set_current_field(f_product, fields[0]);
    //form_driver(f_product, REQ_END_LINE);
    //pos_form_cursor(f_product);
    refresh();
}

/* Función de manejo de entrada: procesa las teclas presionadas y
 * actualiza el formulario y la ventana en consecuencia.
 */
static void driver(int ch)
{
    switch (ch) {
        case KEY_F(2):
            switch (mode) {
                case M_POS:
                    mode = M_PRODUCT_EDIT;
                    break;
                case M_PRODUCT_EDIT:
                    mode = M_PRODUCT_LIST;
                    break;                
                case M_PRODUCT_LIST:
                    mode = M_POS;
                    break;
            }
            draw_screen();
            break;

        case 9:
        case KEY_DOWN:
            form_driver(f_product, REQ_NEXT_FIELD);
            form_driver(f_product, REQ_END_LINE);
            break;

        case KEY_BTAB:
        case KEY_UP:
            form_driver(f_product, REQ_PREV_FIELD);
            form_driver(f_product, REQ_END_LINE);
            break;

        case KEY_LEFT:
            form_driver(f_product, REQ_PREV_CHAR);
            break;

        case KEY_RIGHT:
            form_driver(f_product, REQ_NEXT_CHAR);
            break;

        case KEY_BACKSPACE:
        case 127:
            form_driver(f_product, REQ_DEL_PREV);
            break;

        case KEY_DC:
            form_driver(f_product, REQ_DEL_CHAR);
            break;

        default:
            form_driver(f_product, ch);
            break;
    }
	refresh();
}

/* Función para crear y configurar un campo */
FIELD * set_field(int height, int width, int toprow, int leftcol, int offscreen, int nbuffers) 
{
    FIELD *field = new_field(height, width, toprow, leftcol, offscreen, nbuffers);
    set_field_just(field, JUSTIFY_LEFT);
    field_opts_off(field, O_AUTOSKIP);     // Evitar el avance automático
    //set_field_buffer(field, 0, "-");
    return field;
}

void create_form(FIELD *fields[], FORM *form) 
{
    fields[ 0] = set_field(1, 13,  0+Y_FIELDS, X_FIELDS, 0, 1);  // CODE
    fields[ 1] = set_field(1, 13,  2+Y_FIELDS, X_FIELDS, 0, 1);  // ID
    fields[ 2] = set_field(1, 40,  3+Y_FIELDS, X_FIELDS, 0, 1);  // Producto
    fields[ 3] = set_field(1, 10,  4+Y_FIELDS, X_FIELDS, 0, 1);  // Stock
    fields[ 4] = set_field(1, 40,  6+Y_FIELDS, X_FIELDS, 0, 1);  // Fabricante
    fields[ 5] = set_field(1, 40,  7+Y_FIELDS, X_FIELDS, 0, 1);  // Proveedor
    fields[ 6] = set_field(1, 40,  9+Y_FIELDS, X_FIELDS, 0, 1);  // Departamento
    fields[ 7] = set_field(1, 40, 10+Y_FIELDS, X_FIELDS, 0, 1);  // Clase
    fields[ 8] = set_field(1, 40, 11+Y_FIELDS, X_FIELDS, 0, 1); // Subclase
    fields[ 9] = set_field(2, 40, 13+Y_FIELDS, X_FIELDS, 0, 1); // Descripción 1
    fields[10] = set_field(10,40, 16+Y_FIELDS, X_FIELDS, 0, 1); // Descripción 2
    fields[11] = set_field(1, 20, 27+Y_FIELDS, X_FIELDS, 0, 1); // Precio 1
    fields[12] = set_field(1, 20, 28+Y_FIELDS, X_FIELDS, 0, 1); // Precio 2
    fields[13] = set_field(1, 20, 29+Y_FIELDS, X_FIELDS, 0, 1); // Precio 3
    fields[14] = set_field(1, 20, 30+Y_FIELDS, X_FIELDS, 0, 1); // Precio 4
    fields[15] = set_field(1, 20, 32+Y_FIELDS, X_FIELDS, 0, 1); // IVA
    fields[16] = NULL;

    f_product = new_form(fields);
    post_form(f_product);
    //refresh();
}

void ticket(void)
{
    attron(COLOR_PAIR(2));
    mvprintw(Y_TICKET, X_TICKET, "Total: %.2f", 145.5);
    attron(COLOR_PAIR(4));
    mvprintw(Y_TICKET, X_TICKET + 20, "[ticket %d]", 987123);
    refresh();
}


/* Función para crear las ventanas, el formulario y dibujar las etiquetas */
void draw_screen(void)
{
    clear();
    topbar();
    attron(COLOR_PAIR(4));
    rectangle(1, 0, LINES-2, COLS-1);
    vertical(1, COLS/2, LINES-2, COLS/2);
    mvaddch(1, COLS/2, ACS_TTEE);
    mvaddch(LINES - 2, COLS/2, ACS_BTEE);

    attron(COLOR_PAIR(2));
    mvwprintw(stdscr,  0+Y_FIELDS, 2, "CODE:");
    attron(COLOR_PAIR(4));
    mvwprintw(stdscr,  2+Y_FIELDS, 2, "[ID]:");
    mvwprintw(stdscr,  3+Y_FIELDS, 2, "Producto:");
    mvwprintw(stdscr,  4+Y_FIELDS, 2, "Stock:");
    mvwprintw(stdscr,  5+Y_FIELDS, X_FIELDS, "--------");
    mvwprintw(stdscr,  6+Y_FIELDS, 2, "Fabricante:");
    mvwprintw(stdscr,  7+Y_FIELDS, 2, "Proveedor:");
    mvwprintw(stdscr,  8+Y_FIELDS, X_FIELDS, "--------");
    mvwprintw(stdscr,  9+Y_FIELDS, 2, "Departamento:");
    mvwprintw(stdscr, 10+Y_FIELDS, 2, "Clase:");
    mvwprintw(stdscr, 11+Y_FIELDS, 2, "Subclase:");
    mvwprintw(stdscr, 12+Y_FIELDS, X_FIELDS, "--------");
    mvwprintw(stdscr, 13+Y_FIELDS, 2, "Descripción 1:");
    mvwprintw(stdscr, 15+Y_FIELDS, X_FIELDS, "--------");
    mvwprintw(stdscr, 16+Y_FIELDS, 2, "Descripción 2:");
    mvwprintw(stdscr, 26+Y_FIELDS, X_FIELDS, "--------");
    mvwprintw(stdscr, 27+Y_FIELDS, 2, "Precio 1:");
    mvwprintw(stdscr, 28+Y_FIELDS, 2, "Precio 2:");
    mvwprintw(stdscr, 29+Y_FIELDS, 2, "Precio 3:");
    mvwprintw(stdscr, 30+Y_FIELDS, 2, "Precio 4:");
    mvwprintw(stdscr, 31+Y_FIELDS, X_FIELDS, "--------");
    mvwprintw(stdscr, 32+Y_FIELDS, 2, "IVA:");

    attron(COLOR_PAIR(2));
    mvwprintw(stdscr, LINES-1, 1, "F1: Quit F2: Mode");
    ticket();
    fields_config(mode);
    refresh();
}


int main(void)
{
    int ch;
    strcpy(username, "guest");
    /* Inicialización de ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();

    /* Definir algunos colores (paleta retro) */
    init_color(COLOR_GREEN, 152 * 1000 / 256, 229 * 1000 / 256, 165 * 1000 / 256);
    init_color(COLOR_RED,   107 * 1000 / 256, 148 * 1000 / 256, 108 * 1000 / 256);
    init_color(COLOR_YELLOW, 56 * 1000 / 256, 152 * 1000 / 256,  68 * 1000 / 256);
    init_color(COLOR_BLUE,    0 * 1000 / 256,  66 * 1000 / 256,  15 * 1000 / 256);
    init_color(COLOR_MAGENTA,12 * 1000 / 256,  32 * 1000 / 256,  13 * 1000 / 256);

    init_pair(1, COLOR_GREEN,  COLOR_BLACK);
    init_pair(2, COLOR_RED,    COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE,   COLOR_BLACK);
    init_pair(5, COLOR_RED,    COLOR_MAGENTA);
    init_pair(6, COLOR_YELLOW, COLOR_BLUE);

    create_form(fields, f_product);
    draw_screen();
    /* Bucle principal: se procesa la entrada */
    while ((ch = getch()) != KEY_F(1))
        driver(ch);

    /* Liberar recursos */
    unpost_form(f_product);
    free_form(f_product);
    for (int i = 0; fields[i] != NULL; i++)
        free_field(fields[i]);
    endwin();
    return 0;
}
