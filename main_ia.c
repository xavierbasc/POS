/*
  POS (Point of Sale) Program using ncurses and forms.
  Features:
    - Gestión de productos: Ver, Añadir (usando formularios), Borrar (con paginación).
    - Gestión de usuarios: (stubs de ejemplo)
    - Visualización de tickets (con paginación).
    - Login de agente.
    - Ventas (POS): Agregar productos al carrito, realizar cobro y registrar transacción.
    
  Compilar con:
      gcc pos.c -o pos -lncurses -lform
*/

#include <ncurses.h>
#include <form.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Constantes y definiciones
// ---------------------------------------------------------------------------
#define PRODUCTS_FILE "products.dat"
#define LAST_ID_FILE "last_id.txt"
#define TRANSACTIONS_FILE "transactions.csv"
#define CONFIG_FILE "config.ini"
#define AGENTS_FILE "agents.csv"

#define MAX_CART 50

// ---------------------------------------------------------------------------
// Estructura del producto
// ---------------------------------------------------------------------------
typedef struct {
    int    ID;
    char   EAN13[14];         // Código EAN13 (13 caracteres + '\0')
    char   product[100];
    float  price;
    int    stock;
    float  price01;
    float  price02;
    float  price03;
    float  price04;
    char   fabricante[50];
    char   proveedor[50];
    char   departamento[50];
    char   clase[50];
    char   subclase[50];
    char   tipo_IVA[20];
    char   descripcion1[100];
    char   descripcion2[100];
    char   descripcion3[100];
    char   descripcion4[100];
} Product;

// ---------------------------------------------------------------------------
// Variables globales de configuración y estado
// ---------------------------------------------------------------------------
bool beep_on_insert = false;
char currency_symbol[10] = "$";
bool hide_currency_symbol = false;
bool currency_after_amount = false;

char agent_code[20] = "Default";
time_t agent_login_time;
bool authenticated = false;
int ticket_id = 0;

Product *shopping_cart[MAX_CART];
int cart_count = 0;
float total = 0.0;

// ---------------------------------------------------------------------------
// Prototipos de funciones
// ---------------------------------------------------------------------------
// Configuración y persistencia
void load_config(const char *filename);
bool search_product_disk(const char *query, Product *result);
bool add_product_disk(const Product *prod);
bool delete_product_disk(int ID);
bool validate_agent_and_password(const char *filename, const char *code, const char *password);
int read_last_id(const char *filename);
void update_last_id(const char *filename, int last_id);
void save_transaction(const char *filename, Product **cart, int count, float total);

// Inicialización y limpieza de ncurses
void init_ncurses(void);
void cleanup_ncurses(void);

// Menús
int main_menu(void);
int manage_products_menu(void);
int manage_users_menu(void);
int view_tickets_menu(void);
int pos_sale_menu(void);
int agent_login_menu(void);

// Gestión de productos
void view_products(void);
void form_add_product(void);
void form_delete_product(void);

// Gestión de usuarios (stubs)
void view_users(void);
void add_user(void);
void delete_user(void);

// Login de agente
void agent_login(void);

// Ventas (POS)
void pos_sale(void);

// Función auxiliar para paginación en listados
void paginate_listing(void (*print_line)(int *current_row, int *lines_printed));

// ---------------------------------------------------------------------------
// Implementación de funciones: Configuración y persistencia
// ---------------------------------------------------------------------------
void load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == '#' || *trimmed == ';')
            continue;
        if (strncmp(trimmed, "beep_on_insert=1", 16) == 0)
            beep_on_insert = true;
        else if (strncmp(trimmed, "currency_symbol=", 16) == 0)
            sscanf(trimmed + 16, "%9s", currency_symbol);
        else if (strncmp(trimmed, "hide_currency_symbol=1", 22) == 0)
            hide_currency_symbol = true;
        else if (strncmp(trimmed, "currency_after_amount=1", 23) == 0)
            currency_after_amount = true;
    }
    fclose(file);
}

bool search_product_disk(const char *query, Product *result) {
    if (strlen(query) == 0)
        return false;
    FILE *file = fopen(PRODUCTS_FILE, "rb");
    if (!file) return false;
    Product temp;
    while (fread(&temp, sizeof(Product), 1, file) == 1) {
        if (temp.ID == atoi(query)) {
            *result = temp;
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}

bool add_product_disk(const Product *prod) {
    FILE *file = fopen(PRODUCTS_FILE, "ab");
    if (!file) return false;
    size_t written = fwrite(prod, sizeof(Product), 1, file);
    fclose(file);
    return written == 1;
}

bool delete_product_disk(int ID) {
    FILE *file = fopen(PRODUCTS_FILE, "rb");
    if (!file) return false;
    FILE *temp = fopen("temp.dat", "wb");
    if (!temp) {
        fclose(file);
        return false;
    }
    Product prod;
    bool found = false;
    while (fread(&prod, sizeof(Product), 1, file) == 1) {
        if (prod.ID == ID) {
            found = true;
            continue;
        }
        fwrite(&prod, sizeof(Product), 1, temp);
    }
    fclose(file);
    fclose(temp);
    if (found) {
        remove(PRODUCTS_FILE);
        rename("temp.dat", PRODUCTS_FILE);
    } else {
        remove("temp.dat");
    }
    return found;
}

bool validate_agent_and_password(const char *filename, const char *code, const char *password) {
    FILE *file = fopen(filename, "r");
    if (!file) return false;
    char line[128];
    while (fgets(line, sizeof(line), file)) {
        char agent[20], pass[20];
        if (sscanf(line, "%19[^,],%19[^\n]", agent, pass) == 2) {
            if (strcmp(agent, code) == 0 && strcmp(pass, password) == 0) {
                fclose(file);
                return true;
            }
        }
    }
    fclose(file);
    return false;
}

int read_last_id(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 1001;
    int last_id;
    if (fscanf(file, "%d", &last_id) != 1)
        last_id = 1000;
    fclose(file);
    return last_id;
}

void update_last_id(const char *filename, int last_id) {
    FILE *file = fopen(filename, "w");
    if (!file) return;
    fprintf(file, "%d\n", last_id);
    fclose(file);
}

void save_transaction(const char *filename, Product **cart, int count, float total) {
    ticket_id = read_last_id(LAST_ID_FILE);
    FILE *file = fopen(filename, "a");
    if (!file) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char datetime[20];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", t);
    fprintf(file, "Ticket %d, Agent: %s, Date: %s, Total: %.2f\n", ticket_id, agent_code, datetime, total);
    for (int i = 0; i < count; i++) {
        fprintf(file, "  %s, %.2f\n", cart[i]->product, cart[i]->price);
    }
    fclose(file);
    ticket_id++;
    update_last_id(LAST_ID_FILE, ticket_id);
}

// ---------------------------------------------------------------------------
// Inicialización y limpieza de ncurses
// ---------------------------------------------------------------------------
void init_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_YELLOW, COLOR_BLUE);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    curs_set(1);
}

void cleanup_ncurses(void) {
    endwin();
}

// ---------------------------------------------------------------------------
// Menús interactivos (cada uno limpia la pantalla antes de mostrarse)
// ---------------------------------------------------------------------------
int main_menu(void) {
    clear();
    WINDOW *menu_win = newwin(10, 40, (LINES - 10) / 2, (COLS - 40) / 2);
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 2, "POS System Main Menu");
    mvwprintw(menu_win, 3, 2, "1. Manage Products");
    mvwprintw(menu_win, 4, 2, "2. Manage Users");
    mvwprintw(menu_win, 5, 2, "3. View Tickets");
    mvwprintw(menu_win, 6, 2, "4. Agent Login");
    mvwprintw(menu_win, 7, 2, "5. Sales (POS)");
    mvwprintw(menu_win, 8, 2, "6. Exit");
    wrefresh(menu_win);
    int ch = wgetch(menu_win);
    delwin(menu_win);
    clear();
    return ch;
}

int manage_products_menu(void) {
    clear();
    WINDOW *menu_win = newwin(8, 40, (LINES - 8) / 2, (COLS - 40) / 2);
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 2, "Manage Products");
    mvwprintw(menu_win, 3, 2, "1. View Products");
    mvwprintw(menu_win, 4, 2, "2. Add Product");
    mvwprintw(menu_win, 5, 2, "3. Delete Product");
    mvwprintw(menu_win, 6, 2, "4. Back");
    wrefresh(menu_win);
    int ch = wgetch(menu_win);
    delwin(menu_win);
    clear();
    return ch;
}

int manage_users_menu(void) {
    clear();
    WINDOW *menu_win = newwin(8, 40, (LINES - 8) / 2, (COLS - 40) / 2);
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 2, "Manage Users");
    mvwprintw(menu_win, 3, 2, "1. View Users");
    mvwprintw(menu_win, 4, 2, "2. Add User");
    mvwprintw(menu_win, 5, 2, "3. Delete User");
    mvwprintw(menu_win, 6, 2, "4. Back");
    wrefresh(menu_win);
    int ch = wgetch(menu_win);
    delwin(menu_win);
    clear();
    return ch;
}

int view_tickets_menu(void) {
    clear();
    WINDOW *menu_win = newwin(6, 40, (LINES - 6) / 2, (COLS - 40) / 2);
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 2, "View Tickets");
    mvwprintw(menu_win, 3, 2, "Press any key to view tickets...");
    wrefresh(menu_win);
    wgetch(menu_win);
    delwin(menu_win);
    clear();
    return 0;
}

int pos_sale_menu(void) {
    clear();
    WINDOW *menu_win = newwin(6, 40, (LINES - 6) / 2, (COLS - 40) / 2);
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 2, "Sales (POS)");
    mvwprintw(menu_win, 3, 2, "Press any key to start sale...");
    wrefresh(menu_win);
    wgetch(menu_win);
    delwin(menu_win);
    clear();
    return 0;
}

int agent_login_menu(void) {
    clear();
    WINDOW *menu_win = newwin(6, 50, (LINES - 6) / 2, (COLS - 50) / 2);
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 2, "Agent Login");
    mvwprintw(menu_win, 3, 2, "Press any key to login...");
    wrefresh(menu_win);
    wgetch(menu_win);
    delwin(menu_win);
    clear();
    return 0;
}

// ---------------------------------------------------------------------------
// Funciones de gestión de productos
// ---------------------------------------------------------------------------
/* Listado de productos con paginación.
   Se calcula el número máximo de líneas disponibles (menos una línea para la instrucción)
   y se muestran "páginas" que se avanzan al pulsar una tecla. */
void view_products(void) {
    clear();
    FILE *file = fopen(PRODUCTS_FILE, "rb");
    if (!file) {
        mvprintw(0, 0, "No products found. Press any key to return.");
        getch();
        clear();
        return;
    }
    int page = 1;
    int lines_per_page = LINES - 3; // Reservamos líneas para cabecera y mensaje
    int current_line = 0;
    Product prod;
    while (fread(&prod, sizeof(Product), 1, file) == 1) {
        if (current_line % lines_per_page == 0) {
            clear();
            mvprintw(0, 0, "Product List - Page %d (Press any key for next page, 'q' to quit)", page);
            mvprintw(1, 0, "ID\tProduct\t\tPrice\tStock");
        }
        mvprintw(2 + (current_line % lines_per_page), 0, "%d\t%-15s\t%.2f\t%d", prod.ID, prod.product, prod.price, prod.stock);
        current_line++;
        if (current_line % lines_per_page == 0) {
            int ch = getch();
            if (ch == 'q' || ch == 'Q') {
                fclose(file);
                clear();
                return;
            }
            page++;
        }
    }
    fclose(file);
    mvprintw(LINES - 1, 0, "End of list. Press any key to return.");
    getch();
    clear();
}

/* Formulario para añadir un producto mediante ncurses forms */
void form_add_product(void) {
    FIELD *field[8];
    FORM  *my_form;
    int ch;
    field[0] = new_field(1, 30, 4, 18, 0, 0); // Product Name
    field[1] = new_field(1, 10, 6, 18, 0, 0);  // Price
    field[2] = new_field(1, 10, 8, 18, 0, 0);  // Stock
    field[3] = new_field(1, 10, 10, 18, 0, 0); // Price01
    field[4] = new_field(1, 10, 12, 18, 0, 0); // Price02
    field[5] = new_field(1, 10, 14, 18, 0, 0); // Price03
    field[6] = new_field(1, 10, 16, 18, 0, 0); // Price04
    field[7] = NULL;
    for (int i = 0; i < 7; i++) {
        set_field_back(field[i], A_UNDERLINE);
        field_opts_off(field[i], O_AUTOSKIP);
    }
    my_form = new_form(field);
    post_form(my_form);
    refresh();
    mvprintw(4, 2, "Product Name:");
    mvprintw(6, 2, "Price:");
    mvprintw(8, 2, "Stock:");
    mvprintw(10, 2, "Price01:");
    mvprintw(12, 2, "Price02:");
    mvprintw(14, 2, "Price03:");
    mvprintw(16, 2, "Price04:");
    mvprintw(18, 2, "F1 to submit, ESC to cancel");
    refresh();
    while ((ch = getch()) != KEY_F(1) && ch != 27) {
        switch (ch) {
            case KEY_DOWN:
                form_driver(my_form, REQ_NEXT_FIELD);
                form_driver(my_form, REQ_END_LINE);
                break;
            case KEY_UP:
                form_driver(my_form, REQ_PREV_FIELD);
                form_driver(my_form, REQ_END_LINE);
                break;
            case KEY_BACKSPACE:
            case 127:
                form_driver(my_form, REQ_DEL_PREV);
                break;
            default:
                form_driver(my_form, ch);
                break;
        }
    }
    if (ch == 27) {
        unpost_form(my_form);
        free_form(my_form);
        for (int i = 0; i < 7; i++) {
            free_field(field[i]);
        }
        clear();
        return;
    }
    form_driver(my_form, REQ_NEXT_FIELD);
    unpost_form(my_form);
    refresh();
    Product new_prod;
    int last_id = read_last_id(LAST_ID_FILE);
    new_prod.ID = last_id + 1;
    strncpy(new_prod.product, field_buffer(field[0], 0), sizeof(new_prod.product) - 1);
    new_prod.product[sizeof(new_prod.product) - 1] = '\0';
    new_prod.price = atof(field_buffer(field[1], 0));
    new_prod.stock = atoi(field_buffer(field[2], 0));
    new_prod.price01 = atof(field_buffer(field[3], 0));
    new_prod.price02 = atof(field_buffer(field[4], 0));
    new_prod.price03 = atof(field_buffer(field[5], 0));
    new_prod.price04 = atof(field_buffer(field[6], 0));
    // Los demás campos se dejan vacíos.
    if (add_product_disk(&new_prod)) {
        update_last_id(LAST_ID_FILE, new_prod.ID);
        mvprintw(20, 2, "Product added with ID %d.", new_prod.ID);
        if (beep_on_insert)
            beep();
    } else {
        mvprintw(20, 2, "Failed to add product.");
    }
    mvprintw(22, 2, "Press any key to continue...");
    getch();
    free_form(my_form);
    for (int i = 0; i < 7; i++) {
        free_field(field[i]);
    }
    clear();
}

/* Formulario para borrar un producto (ingresando su ID) */
void form_delete_product(void) {
    clear();
    char id_str[10];
    echo();
    mvprintw(4, 2, "Enter Product ID to delete: ");
    getstr(id_str);
    noecho();
    int id = atoi(id_str);
    if (delete_product_disk(id))
        mvprintw(6, 2, "Product with ID %d deleted.", id);
    else
        mvprintw(6, 2, "Product with ID %d not found.", id);
    mvprintw(8, 2, "Press any key to continue...");
    getch();
    clear();
}

// ---------------------------------------------------------------------------
// Funciones de gestión de usuarios (stubs)
// ---------------------------------------------------------------------------
void view_users(void) {
    clear();
    mvprintw(0, 0, "User List (stub):");
    for (int i = 0; i < 5; i++) {
        mvprintw(i + 1, 0, "User %d: User%d", i + 1, i + 1);
    }
    mvprintw(LINES - 1, 0, "Press any key to return.");
    getch();
    clear();
}

void add_user(void) {
    clear();
    char username[20];
    mvprintw(0, 0, "Enter new username: ");
    echo();
    getstr(username);
    noecho();
    mvprintw(2, 0, "User '%s' added (stub).", username);
    mvprintw(4, 0, "Press any key to return.");
    getch();
    clear();
}

void delete_user(void) {
    clear();
    char id_str[10];
    mvprintw(0, 0, "Enter user ID to delete: ");
    echo();
    getstr(id_str);
    noecho();
    mvprintw(2, 0, "User with ID %s deleted (stub).", id_str);
    mvprintw(4, 0, "Press any key to return.");
    getch();
    clear();
}

// ---------------------------------------------------------------------------
// Función de login de agente
// ---------------------------------------------------------------------------
void agent_login(void) {
    clear();
    char code[20], password[20];
    mvprintw(2, 2, "Enter Agent Code: ");
    echo();
    getstr(code);
    noecho();
    mvprintw(4, 2, "Enter Password: ");
    int i = 0, ch;
    while ((ch = getch()) != '\n' && ch != KEY_ENTER && i < 19) {
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (i > 0) { i--; mvprintw(4, 18 + i, " "); move(4, 18 + i); }
        } else if (isprint(ch)) {
            password[i++] = ch;
            mvaddch(4, 18 + i - 1, '*');
        }
    }
    password[i] = '\0';
    if (validate_agent_and_password(AGENTS_FILE, code, password)) {
        strncpy(agent_code, code, sizeof(agent_code) - 1);
        authenticated = true;
        agent_login_time = time(NULL);
        mvprintw(6, 2, "Login successful.");
    } else {
        mvprintw(6, 2, "Login failed.");
    }
    mvprintw(8, 2, "Press any key to continue...");
    getch();
    clear();
}

// ---------------------------------------------------------------------------
// Función de ventas (POS)
// ---------------------------------------------------------------------------
void pos_sale(void) {
    char id_str[10], qty_str[10];
    Product prod;
    while (1) {
        clear();
        mvprintw(0, 0, "Enter Product ID (0 to finish): ");
        echo();
        getstr(id_str);
        noecho();
        int id = atoi(id_str);
        if (id == 0)
            break;
        char query[20];
        sprintf(query, "%d", id);
        if (!search_product_disk(query, &prod)) {
            mvprintw(2, 0, "Product not found. Press any key to continue...");
            getch();
            continue;
        }
        mvprintw(2, 0, "Enter Quantity: ");
        echo();
        getstr(qty_str);
        noecho();
        int qty = atoi(qty_str);
        if (qty <= 0) {
            mvprintw(3, 0, "Invalid quantity. Press any key...");
            getch();
            continue;
        }
        for (int i = 0; i < qty && cart_count < MAX_CART; i++) {
            Product *p = malloc(sizeof(Product));
            if (p) {
                *p = prod;
                shopping_cart[cart_count++] = p;
                total += prod.price;
            }
        }
        mvprintw(4, 0, "Added %d of product '%s'.", qty, prod.product);
        mvprintw(5, 0, "Press any key to continue...");
        getch();
    }
    // Resumen de venta y pago
    clear();
    mvprintw(0, 0, "Sale Summary:");
    int row = 2;
    for (int i = 0; i < cart_count; i++) {
        mvprintw(row++, 0, "%d. %s - %.2f", i + 1, shopping_cart[i]->product, shopping_cart[i]->price);
        if (row >= LINES - 3) {
            mvprintw(LINES - 2, 0, "Press any key for next page...");
            getch();
            clear();
            row = 2;
        }
    }
    mvprintw(row++, 0, "Total: %.2f", total);
    char paid_str[20];
    mvprintw(row++, 0, "Enter amount paid: ");
    echo();
    getstr(paid_str);
    noecho();
    float paid = atof(paid_str);
    float change = paid - total;
    mvprintw(row++, 0, "Change: %.2f", change);
    mvprintw(row++, 0, "Press any key to complete sale...");
    getch();
    save_transaction(TRANSACTIONS_FILE, shopping_cart, cart_count, total);
    for (int i = 0; i < cart_count; i++) {
        free(shopping_cart[i]);
    }
    cart_count = 0;
    total = 0.0;
    clear();
}

// ---------------------------------------------------------------------------
// Función principal
// ---------------------------------------------------------------------------
int main(void) {
    load_config(CONFIG_FILE);
    init_ncurses();
    int choice;
    bool running = true;
    while (running) {
        choice = main_menu();
        switch (choice) {
            case '1': { // Manage Products
                int p_choice;
                bool prod_running = true;
                while (prod_running) {
                    p_choice = manage_products_menu();
                    switch (p_choice) {
                        case '1': view_products(); break;
                        case '2': form_add_product(); break;
                        case '3': form_delete_product(); break;
                        case '4': prod_running = false; break;
                        default: break;
                    }
                }
                break;
            }
            case '2': { // Manage Users (stub)
                int u_choice;
                bool user_running = true;
                while (user_running) {
                    u_choice = manage_users_menu();
                    switch (u_choice) {
                        case '1': view_users(); break;
                        case '2': add_user(); break;
                        case '3': delete_user(); break;
                        case '4': user_running = false; break;
                        default: break;
                    }
                }
                break;
            }
            case '3': { // View Tickets
                view_tickets_menu();
                clear();
                FILE *file = fopen(TRANSACTIONS_FILE, "r");
                if (file) {
                    char line[256];
                    int page = 1;
                    int lines_per_page = LINES - 3;
                    int count = 0;
                    while (fgets(line, sizeof(line), file)) {
                        if (count % lines_per_page == 0) {
                            clear();
                            mvprintw(0, 0, "Tickets - Page %d (Press any key for next page, 'q' to quit)", page);
                        }
                        mvprintw(1 + (count % lines_per_page), 0, "%s", line);
                        count++;
                        if (count % lines_per_page == 0) {
                            int ch = getch();
                            if (ch == 'q' || ch == 'Q')
                                break;
                            page++;
                        }
                    }
                    fclose(file);
                } else {
                    mvprintw(0, 0, "No tickets found.");
                    getch();
                }
                mvprintw(LINES - 1, 0, "Press any key to return.");
                getch();
                clear();
                break;
            }
            case '4': { // Agent Login
                agent_login_menu();
                agent_login();
                break;
            }
            case '5': { // Sales (POS)
                pos_sale_menu();
                pos_sale();
                break;
            }
            case '6': { // Exit
                running = false;
                break;
            }
            default: break;
        }
    }
    cleanup_ncurses();
    return 0;
}
