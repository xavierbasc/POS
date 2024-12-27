#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h> // Para usleep()
#include <stdbool.h> // Para valores booleanos

// Define the structure for products
typedef struct {
    char code[10];
    char name[50];
    float price;
} Product;

// Product catalog
#define MAX_PRODUCTS 10000
Product catalog[MAX_PRODUCTS];
int catalog_count = 0;

bool beep_on_insert = false;
char currency_symbol[10] = "$";
bool hide_currency_symbol = false;
bool currency_after_amount = false;

bool authenticated = false;

char agent_code[20] = "Default"; // Agent code
time_t agent_login_time; // Time when the agent logged in

// Función para cargar configuración desde config.ini
void load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Eliminar espacios iniciales
        char *trimmed_line = line;
        while (isspace(*trimmed_line)) {
            trimmed_line++;
        }

        // Ignorar líneas vacías o comentarios
        if (*trimmed_line == '\0' || *trimmed_line == '#' || *trimmed_line == ';') {
            continue;
        }

        // Parsear configuraciones
        if (strncmp(trimmed_line, "beep_on_insert=1", 16) == 0) {
            beep_on_insert = true;
        } else if (strncmp(trimmed_line, "currency_symbol=", 16) == 0) {
            sscanf(trimmed_line + 16, "%9s", currency_symbol); // Lee hasta 9 caracteres para el símbolo
        } else if (strncmp(trimmed_line, "hide_currency_symbol=1", 22) == 0) {
            hide_currency_symbol = true;
        } else if (strncmp(trimmed_line, "currency_after_amount=1", 23) == 0) {
            currency_after_amount = true;
        }
    }

    fclose(file);
}
// Function to load products from a CSV file
void load_products(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening product file");
        exit(EXIT_FAILURE);
    }

    char line[128];
    while (fgets(line, sizeof(line), file) && catalog_count < MAX_PRODUCTS) {
        sscanf(line, "%9[^,],%49[^,],%f", catalog[catalog_count].code, catalog[catalog_count].name, &catalog[catalog_count].price);
        catalog_count++;
    }

    fclose(file);
}

// Function to validate agent and password from a CSV file
bool validate_agent_and_password(const char *filename, const char *code, const char *password) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening agent file");
        return false;
    }

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

// Function to read the last ID from a file
int read_last_id(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 1; // Default to 1 if the file doesn't exist
    }

    int last_id;
    if (fscanf(file, "%d", &last_id) != 1) {
        last_id = 1; // Default to 1 if the file is empty or invalid
    }

    fclose(file);
    return last_id;
}

// Function to update the last ID in a file
void update_last_id(const char *filename, int last_id) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening last ID file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%d\n", last_id);
    fclose(file);
}

// Function to append a transaction to a CSV file
void save_transaction(const char *filename, Product **cart, int count, float total) {
    static const char *id_filename = "last_id.txt";
    int ticket_id = read_last_id(id_filename);

    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Error opening transaction file");
        exit(EXIT_FAILURE);
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char datetime[20];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", t);

    fprintf(file, "Ticket %d, Agent: %s, Date: %s, Total: %.2f\n", ticket_id, agent_code, datetime, total);
    for (int i = 0; i < count; i++) {
        fprintf(file, "  %s, %s, %.2f\n", cart[i]->code, cart[i]->name, cart[i]->price);
    }

    fclose(file);

    // Update the last ID
    update_last_id(id_filename, ticket_id + 1);
}


// Function to search for a product by code or name
Product *search_product(const char *query) {
    for (int i = 0; i < catalog_count; i++) {
        if (strstr(catalog[i].code, query) || strstr(catalog[i].name, query)) {
            return &catalog[i];
        }
    }
    return NULL;
}

void get_password(int x, int y, char *buffer, size_t max_len) {
    size_t i = 0;
    int ch;

    while ((ch = getch()) != '\n' && i < max_len - 1) { // Leer hasta ENTER o llenar el buffer
        if (ch == KEY_BACKSPACE || ch == 127) { // Manejar BACKSPACE
            if (i > 0) {
                i--;
                mvaddch(x , y + i, ' '); // Borrar el último asterisco
                move(x , y + i); // Mover el cursor atrás
            }
        } else if (isprint(ch)) { // Solo caracteres imprimibles
            buffer[i++] = ch;
            addch('*'); // Mostrar un asterisco
        }
    }
    buffer[i] = '\0'; // Terminar la cadena
}

// Función para mostrar el detalle de un ticket
void view_ticket_details(const char *filename, int ticket_id) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        mvprintw(0, 0, "Error al abrir el archivo de tickets");
        nodelay(stdscr, FALSE); // Temporarily block
        getch();
        nodelay(stdscr, TRUE); // Restore non-blocking
        return;
    }

    char line[256];
    bool found = false;
    clear();
    mvprintw(0, 0, "Detalles del Ticket %d:", ticket_id);
    int y = 2;

    while (fgets(line, sizeof(line), file)) {
        int current_ticket_id;
        if (sscanf(line, "Ticket %d,", &current_ticket_id) == 1) {
            if (current_ticket_id == ticket_id) {
                found = true;
                mvprintw(y++, 0, "%s", line);
            } else if (found) {
                break; // Salir cuando se encuentra un nuevo ticket
            }
        } else if (found) {
            mvprintw(y++, 0, "%s", line);
        }
    }

    fclose(file);

    if (!found) {
        mvprintw(y, 0, "Ticket no encontrado.");
    }

    mvprintw(y + 2, 0, "Presione cualquier tecla para volver...");
    nodelay(stdscr, FALSE); // Temporarily block
    getch();
    nodelay(stdscr, TRUE); // Restore non-blocking
}

// Función para listar y seleccionar un ticket con paginación para miles de tickets (orden descendente)
void list_tickets(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        mvprintw(0, 0, "Error al abrir el archivo de tickets");
        getch();
        return;
    }

    char line[256];
    int ticket_ids[1000];
    char ticket_dates[1000][20];
    char ticket_agents[1000][20];
    float ticket_totals[1000];
    int ticket_count = 0;

    while (fgets(line, sizeof(line), file)) {
        int ticket_id;
        char date[20], agent[20];
        float total;
        if (sscanf(line, "Ticket %d, Agent: %19[^,], Date: %19[^,], Total: %f", &ticket_id, agent, date, &total) == 4) {
            ticket_ids[ticket_count] = ticket_id;
            strncpy(ticket_dates[ticket_count], date, sizeof(ticket_dates[ticket_count]) - 1);
            strncpy(ticket_agents[ticket_count], agent, sizeof(ticket_agents[ticket_count]) - 1);
            ticket_totals[ticket_count] = total;
            ticket_count++;
        }
    }

    fclose(file);

    if (ticket_count == 0) {
        mvprintw(0, 0, "No hay tickets disponibles.");
        mvprintw(1, 0, "Presione cualquier tecla para volver...");
        getch();
        return;
    }

    int current_selection = 0;
    int page_size = LINES - 4; // Tamaño de la página menos espacio para encabezado
    int page = 0;
    int total_pages = (ticket_count + page_size - 1) / page_size;
    int ch;

    while (1) {
        erase(); // Clear screen without flickering
        mvprintw(0, 0, "Seleccione un ticket (q para salir, izquierda/derecha para cambiar página):");
        mvprintw(1, 0, "%-10s %-20s %-20s %-10s", "Ticket ID", "Fecha", "Agente", "Total");
        int start_index = ticket_count - 1 - (page * page_size);
        int end_index = start_index - page_size;
        if (end_index < -1) {
            end_index = -1;
        }

        for (int i = start_index, row = 2; i > end_index; i--, row++) {
            if (i == ticket_count - 1 - current_selection) {
                attron(A_REVERSE);
                if (hide_currency_symbol) {
                    mvprintw(row, 0, "%-10d %-20s %-20s %6.2f", ticket_ids[i], ticket_dates[i], ticket_agents[i], ticket_totals[i]);
                } else if (currency_after_amount) {
                    mvprintw(row, 0, "%-10d %-20s %-20s %6.2f %s", ticket_ids[i], ticket_dates[i], ticket_agents[i], ticket_totals[i], currency_symbol);
                } else {
                    mvprintw(row, 0, "%-10d %-20s %-20s %s%6.2f", ticket_ids[i], ticket_dates[i], ticket_agents[i], currency_symbol, ticket_totals[i]);
                }
                attroff(A_REVERSE);
            } else {
                if (hide_currency_symbol) {
                    mvprintw(row, 0, "%-10d %-20s %-20s %6.2f", ticket_ids[i], ticket_dates[i], ticket_agents[i], ticket_totals[i]);
                } else if (currency_after_amount) {
                    mvprintw(row, 0, "%-10d %-20s %-20s %6.2f %s", ticket_ids[i], ticket_dates[i], ticket_agents[i], ticket_totals[i], currency_symbol);
                } else {
                    mvprintw(row, 0, "%-10d %-20s %-20s %s%6.2f", ticket_ids[i], ticket_dates[i], ticket_agents[i], currency_symbol, ticket_totals[i]);
                }
            }
        }

        mvprintw(LINES - 1, 0, "Página %d/%d", page + 1, total_pages);

        ch = getch();
        if (ch == 'q') {
            break;
        } else if (ch == KEY_UP) {
            if (current_selection > 0) {
                current_selection--;
                if (current_selection < page * page_size) {
                    page--;
                }
            }
        } else if (ch == KEY_DOWN) {
            if (current_selection < ticket_count - 1) {
                current_selection++;
                if (current_selection >= (page + 1) * page_size) {
                    page++;
                }
            }
        } else if (ch == KEY_LEFT) {
            if (page > 0) {
                page--;
                current_selection = page * page_size;
            }
        } else if (ch == KEY_RIGHT) {
            if (page < total_pages - 1) {
                page++;
                current_selection = page * page_size;
            }
        } else if (ch == 10) { // Enter key
            view_ticket_details(filename, ticket_ids[ticket_count - 1 - current_selection]);
        }
    }
}

// Función para guardar productos en CSV
void save_products_to_csv(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        mvprintw(0, 0, "Error al guardar productos en el archivo");
        getch();
        return;
    }
    for (int i = 0; i < 100; i++) {
        fprintf(file, "Producto%d,%.2f\n", i + 1, 10.0 + i);
    }
    fclose(file);
}

// Función para guardar usuarios en CSV
void save_users_to_csv(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        mvprintw(0, 0, "Error al guardar usuarios en el archivo");
        getch();
        return;
    }
    for (int i = 0; i < 100; i++) {
        fprintf(file, "Usuario%d\n", i + 1);
    }
    fclose(file);
}

// Función para gestionar productos
void manage_products() {
    clear();
    mvprintw(0, 0, "Gestión de Productos");
    mvprintw(2, 0, "1. Ver productos");
    mvprintw(3, 0, "2. Añadir producto");
    mvprintw(4, 0, "3. Eliminar producto");
    mvprintw(5, 0, "q. Salir");

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case '1':
                clear();
                mvprintw(0, 0, "Lista de Productos:");
                for (int i = 0; i < 100; i++) {
                    mvprintw(i + 2, 0, "Producto %d", i + 1);
                }
                mvprintw(20, 0, "Presione cualquier tecla para continuar...");
                nodelay(stdscr, FALSE); // Temporarily block
                getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                break;
            case '2':
                clear();
                mvprintw(0, 0, "Añadir Producto:");
                mvprintw(2, 0, "Nombre:");
                char name[50];
                echo();
                getstr(name);
                noecho();
                mvprintw(4, 0, "Producto añadido: %s", name);
                save_products_to_csv("productos.csv");
                nodelay(stdscr, FALSE); // Temporarily block
                getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                break;
            case '3':
                clear();
                mvprintw(0, 0, "Eliminar Producto:");
                mvprintw(2, 0, "ID:");
                char id[10];
                echo();
                getstr(id);
                noecho();
                mvprintw(4, 0, "Producto eliminado: %s", id);
                save_products_to_csv("productos.csv");
                nodelay(stdscr, FALSE); // Temporarily block
                getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                break;
        }
        erase(); // Clear screen without flickering
        mvprintw(0, 0, "Gestión de Productos");
        mvprintw(2, 0, "1. Ver productos");
        mvprintw(3, 0, "2. Añadir producto");
        mvprintw(4, 0, "3. Eliminar producto");
        mvprintw(5, 0, "q. Salir");
    }
}

// Función para gestionar usuarios
void manage_users() {
    clear();
    mvprintw(0, 0, "Gestión de Usuarios");
    mvprintw(2, 0, "1. Ver usuarios");
    mvprintw(3, 0, "2. Añadir usuario");
    mvprintw(4, 0, "3. Eliminar usuario");
    mvprintw(5, 0, "q. Salir");

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case '1':
                clear();
                mvprintw(0, 0, "Lista de Usuarios:");
                for (int i = 0; i < 100; i++) {
                    mvprintw(i + 2, 0, "Usuario %d", i + 1);
                }
                mvprintw(20, 0, "Presione cualquier tecla para continuar...");
                nodelay(stdscr, FALSE); // Temporarily block
                getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                break;
            case '2':
                clear();
                mvprintw(0, 0, "Añadir Usuario:");
                mvprintw(2, 0, "Nombre:");
                char name[50];
                echo();
                getstr(name);
                noecho();
                mvprintw(4, 0, "Usuario añadido: %s", name);
                save_users_to_csv("usuarios.csv");
                nodelay(stdscr, FALSE); // Temporarily block
                getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                break;
            case '3':
                clear();
                mvprintw(0, 0, "Eliminar Usuario:");
                mvprintw(2, 0, "ID:");
                char id[10];
                echo();
                getstr(id);
                noecho();
                mvprintw(4, 0, "Usuario eliminado: %s", id);
                save_users_to_csv("usuarios.csv");
                nodelay(stdscr, FALSE); // Temporarily block
                getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                break;
        }
        erase(); // Clear screen without flickering
        mvprintw(0, 0, "Gestión de Usuarios");
        mvprintw(2, 0, "1. Ver usuarios");
        mvprintw(3, 0, "2. Añadir usuario");
        mvprintw(4, 0, "3. Eliminar usuario");
        mvprintw(5, 0, "q. Salir");
    }
}

// Integración en el menú de gestión
int manage_menu() {
    WINDOW *menu_win;
    int menu_height = 8, menu_width = 40;
    int start_y = (LINES - menu_height) / 2;
    int start_x = (COLS - menu_width) / 2;

    menu_win = newwin(menu_height, menu_width, start_y, start_x);
    box(menu_win, 0, 0);
    keypad(menu_win, TRUE);

    wbkgd(menu_win, COLOR_PAIR(3));
    curs_set(0);

    const char *choices[] = {
        "1. Manage products",
        "2. Manage users",
        "3. View tickets",
        "9. Exit application",
        "",
        "q. Exit menu"
    };
    int n_choices = sizeof(choices) / sizeof(choices[0]);
    int choice = 0;
    int highlight = 0;

    while (1) {
        for (int i = 0; i < n_choices; i++) {
            if (strlen(choices[i]) == 0) {
                mvwprintw(menu_win, i + 1, 1, " ");
                continue;
            }

            if (i == highlight) {
                wattron(menu_win, A_REVERSE);
                mvwprintw(menu_win, i + 1, 1, "%s", choices[i]);
                wattroff(menu_win, A_REVERSE);
            } else {
                mvwprintw(menu_win, i + 1, 1, "%s", choices[i]);
            }
        }

        wrefresh(menu_win);

        int ch = wgetch(menu_win);
        switch (ch) {
            case KEY_UP:
                do {
                    highlight = (highlight - 1 + n_choices) % n_choices;
                } while (strlen(choices[highlight]) == 0);
                break;
            case KEY_DOWN:
                do {
                    highlight = (highlight + 1) % n_choices;
                } while (strlen(choices[highlight]) == 0);
                break;
            case 10: // Enter key
                if (highlight == 0) {
                    delwin(menu_win);
                    manage_products();
                    return 0;
                } if (highlight == 1) {
                    delwin(menu_win);
                    manage_users();
                    return 0;
                } else if (highlight == 2) {
                    delwin(menu_win);
                    list_tickets("transactions.csv");
                    return 0;
                } else if (highlight == n_choices - 1 || highlight == 3) { // Exit
                    delwin(menu_win);
                    return 'q';
                } else {
                    mvwprintw(menu_win, menu_height - 2, 1, "Opción no implementada.");
                    wrefresh(menu_win);
                    usleep(500000);
                }
                break;
            case 'q':
                delwin(menu_win);
                return 0;
        }
    }
}


int main() {
    Product *product = NULL;

    load_config("config.ini");

    // Load products from CSV
    load_products("products.csv");

    // Initialize ncurses
    initscr();
    cbreak();
    noecho(); // Disable echo for smoother UI
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); // Non-blocking input
    start_color();

    // Define colors
    init_color(COLOR_CYAN, 100, 100, 100); // Gris claro (escala de 0 a 1000)
    init_color(COLOR_BLUE, 0, 0, 500); // Gris claro (escala de 0 a 1000)
    init_color(COLOR_YELLOW, 1000, 1000, 0); // Gris claro (escala de 0 a 1000)

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_CYAN);
    init_pair(3, COLOR_WHITE, COLOR_BLUE);
    init_pair(4, COLOR_YELLOW, COLOR_BLUE);

    char query1[50] = "";
    char query2[50] = "";
    Product *shopping_cart[50];
    int cart_count = 0;
    float total = 0.0;
    int scroll_offset = 0; // Offset for scrolling

    int input_mode = 1; // 1 for code, 2 for name
    agent_login_time = time(NULL); // Record the login time
    
    int ch = 0;

    while (1) {
        erase(); // Clear screen without flickering
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        // Draw vertical separator
        for (int y = 2; y < max_y-2; y++) {
            mvaddch(y, max_x / 2, ACS_VLINE);
        }

        // Display current time in the top-right corner
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_str[10];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", t);
        mvprintw(0, max_x - strlen(time_str) - 1, "%s", time_str);

        // Calculate logged-in duration
        int duration = (int)difftime(now, agent_login_time);
        int hours = duration / 3600;
        int minutes = (duration % 3600) / 60;
        int seconds = duration % 60;

        // Left panel for input
        if (!authenticated)
            mvprintw(0, 1, "Agent: - no agent -");
        else
            mvprintw(0, 1, "Agent: %s (%02d:%02d:%02d)", agent_code, hours, minutes, seconds);

        mvprintw(2, 1, "CODE: ");
        //mvprintw(4, 10, "%2s", query2);
        //mvchgat(2, 1, 1, A_UNDERLINE, 0, NULL);
        
        mvprintw(4, 1, "Product: %s", (query1[0]=='\0' || product==NULL)?"":product->name);
        mvprintw(5, 1, "Price: %.2f", (query1[0]=='\0' || product==NULL)?0.0:product->price);

        attron(COLOR_PAIR(2));
        mvprintw(2, 7, "%19s", query1);
        attroff(COLOR_PAIR(2));

        // Right panel for shopping cart
        int x_offset = max_x / 2 + 2;
        attron(COLOR_PAIR(1));
        mvprintw(0, x_offset, "  Total ticket: %.2f", total);
        attroff(COLOR_PAIR(1));

        for (int i = scroll_offset; i < cart_count && i - scroll_offset < max_y - 3; i++) {
            mvprintw((i - scroll_offset) + 2, x_offset, "%c %03d. %-15s - %.2f", (i+1)==cart_count?'>':' ' , i + 1, shopping_cart[i]->name, shopping_cart[i]->price);
        }

        // Footer

#ifdef DEBUG
        static int ch_old;
        if (ch != -1) {
            ch_old = ch;
        }
        mvprintw(max_y - 2, 0, "KEY: %d", ch_old);
#endif

        attron(COLOR_PAIR(3));

        // Draw horizontal
        for (int x = 0; x < max_x / 2; x++) {
            mvaddch(max_y - 1, x, ' ');
        }
        for (int x = max_x / 2 + 1; x < max_x ; x++) {
            mvaddch(max_y - 1, x, ' ');
        }

        mvprintw(max_y - 1, 0, " Agent  Management");
        mvchgat(max_y - 1, 1, 1, A_COLOR, 4, NULL); // BACKGROUND COLOR
        mvchgat(max_y - 1, 8, 1, A_COLOR, 4, NULL); // BACKGROUND COLOR

        mvprintw(max_y - 1, max_x / 2 + 1, " Pay  Delete ");
        mvchgat(max_y - 1, max_x / 2 + 2, 1, A_COLOR, 4, NULL); // BACKGROUND COLOR
        mvchgat(max_y - 1, max_x / 2 + 7, 1, A_COLOR, 4, NULL); // BACKGROUND COLOR


        // Scroll instructions if needed
        if (cart_count > max_y - 3) {
            mvprintw(max_y - 1, max_x-20, "Scroll: [Up/Down]");
        }

        attroff(COLOR_PAIR(3));
        move(2, 25); // Place the cursor at the end of the input field

        // Refresh screen every second
        timeout(1000);

        // Non-blocking input
        nodelay(stdscr, TRUE);
        ch = getch();
        
        // Handle KEY_RESIZE explicitly
        if (ch == KEY_RESIZE) {
            getmaxyx(stdscr, max_y, max_x); // Recalculate window size
            clear(); // Clear the screen completely to redraw
        }

        if (ch != ERR && ch != KEY_RESIZE) { // Only process valid inputs
            if (ch == ch == 'M' || ch == 'm') {
                ch = manage_menu();
                clear(); // Clear the main window after returning from the menu
            }

            int l = strlen(query1);
            char text[50] = "";
            strcpy(text, query1);
            text[l] = (char)ch;
            text[l + 1] = '\0';
            product = search_product(text);

            if (ch == 10) { // ENTER key
                product = search_product(query1);
                query1[0] = '\0'; // Clear the input field

                if (product) {
                    shopping_cart[cart_count++] = product;
                    total += product->price;
                    if (beep_on_insert) {
                        beep();
                    }

                } else {
                    mvprintw(max_y - 2, 0, "Product not found. Press key ...");
                    nodelay(stdscr, FALSE); // Temporarily block
                    getch();
                    nodelay(stdscr, TRUE); // Restore non-blocking
                }
            } else if (ch == 'N' || ch == 'n') { // Switch to name search
                mvprintw(max_y - 3, 0, "Enter product name: ");
                echo();
                char product_name[20];                
                nodelay(stdscr, FALSE); // Temporarily block
                getstr(product_name);
                nodelay(stdscr, TRUE); // Restore non-blocking
            } else if (ch == 'A' || ch == 'a') { // Change agent code
                mvprintw(max_y - 3, 0, "Enter agent code (ENTER='no agent'): ");
                echo();
                char new_agent_code[20];
                char psw[20];
                nodelay(stdscr, FALSE); // Temporarily block
                getstr(new_agent_code);
                mvprintw(max_y - 3, 0, "Enter agent password:                  ");
                noecho();
                move(max_y - 3, 22);
                get_password(max_y - 3, 22, psw, sizeof(psw));
                nodelay(stdscr, TRUE); // Restore non-blocking
                if (validate_agent_and_password("agents.csv", new_agent_code, psw)) {
                    strncpy(agent_code, new_agent_code, sizeof(agent_code) - 1);
                    agent_code[sizeof(agent_code) - 1] = '\0';
                    agent_login_time = time(NULL); // Record the login time
                    authenticated = true;
                } else {
                    mvprintw(max_y - 2, 0, "Invalid agent code.");
                    nodelay(stdscr, FALSE); // Temporarily block
                    getch();
                    nodelay(stdscr, TRUE); // Restore non-blocking
                }
            } else if (ch == 'P' || ch == 'p') { // Checkout
                mvprintw(max_y - 2, x_offset , "> Checkout? (Y/n)");
                nodelay(stdscr, FALSE); // Temporarily block
                int ch = getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                if (ch != 'N' && ch != 'n') {
                    save_transaction("transactions.csv", shopping_cart, cart_count, total);
                    cart_count = 0;
                    total = 0.0;
                }
            } else if (ch == 'D' || ch == 'd') { // Delete item
                mvprintw(max_y - 3, max_x / 2 + 2, "POS to delete (last by default): ");
                echo();
                char pos_input[10];
                nodelay(stdscr, FALSE); // Temporarily block
                getstr(pos_input);
                nodelay(stdscr, TRUE); // Restore non-blocking
                noecho();
                int pos;
                if (strlen(pos_input)==0) { pos = cart_count - 1; }
                else pos = atoi(pos_input) - 1;
                if (pos >= 0 && pos < cart_count) {
                    mvprintw(max_y - 2, max_x / 2 + 2, "Delete %s (%s)? (Y/n): ", shopping_cart[pos]->name, shopping_cart[pos]->code);
                    nodelay(stdscr, FALSE); // Temporarily block
                    int confirm = getch();
                    nodelay(stdscr, TRUE); // Restore non-blocking
                    if (confirm != 'n' && confirm != 'N') {
                        total -= shopping_cart[pos]->price;
                        for (int i = pos; i < cart_count - 1; i++) {
                            shopping_cart[i] = shopping_cart[i + 1];
                        }
                        cart_count--;
                    } else {
                        mvprintw(max_y - 2, max_x / 2 + 2, "Item not removed. Press key ...");
                        curs_set(0);
                        nodelay(stdscr, FALSE);
                        getch();
                        nodelay(stdscr, TRUE);
                        curs_set(1);
                    }
                } else {
                    mvprintw(max_y - 2, max_x / 2 + 2, "Invalid position. Press key ...");
                    curs_set(0);
                    nodelay(stdscr, FALSE);
                    getch();
                    nodelay(stdscr, TRUE);
                    curs_set(1);
                }

            } else if (ch == KEY_BACKSPACE || ch == 127) { // BACKSPACE key
                if (input_mode == 1 && strlen(query1) > 0) {
                    query1[strlen(query1) - 1] = '\0';
                } else if (input_mode == 2 && strlen(query2) > 0) {
                    query2[strlen(query2) - 1] = '\0';
                }
            } else if (ch == KEY_UP) { // Scroll up
                if (scroll_offset > 0) {
                    scroll_offset--;
                }
            } else if (ch == KEY_DOWN) { // Scroll down
                if (scroll_offset < cart_count - (max_y - 3)) {
                    scroll_offset++;
                }
            } else if (ch == 'q' || ch == 'Q') { // Quit
                break;
            } else if (ch != '\n') {
                if (input_mode == 1) {
                    int len = strlen(query1);
                    query1[len] = (char)ch;
                    query1[len + 1] = '\0';
                } else {
                    int len = strlen(query2);
                    query2[len] = (char)ch;
                    query2[len + 1] = '\0';
                }
            }
        } 
        // Delay to refresh every 100ms for smooth updates
        usleep(100000);
    }

    // End ncurses mode
    endwin();
    return 0;
}
