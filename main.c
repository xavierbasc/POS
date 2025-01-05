#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h> // Para usleep()
#include <stdbool.h> // Para valores booleanos
#include <stdarg.h>

// Define the structure for products
typedef struct {
    int    ID;
    char   EAN13[14];         // Código EAN13: 13 caracteres + terminador nulo
    char   product[100];
    float  price;
    int    stock;
    float  price01;
    float  price02;
    float  price03;
    float  price04;
    char   fabricante[50];    // Fabricante
    char   proveedor[50];     // Proveedor
    char   departamento[50];  // Departamento
    char   clase[50];         // Clase
    char   subclase[50];      // Subclase
    char   tipo_IVA[20];      // Tipo de IVA: "reducido" o "super reducido"
    char   descripcion1[100]; // Nuevos campos de descripción
    char   descripcion2[100];
    char   descripcion3[100];
    char   descripcion4[100];
} Product;


<<<<<<< HEAD
=======
// Configuración y otras variables globales
>>>>>>> 2d01d40c9a73526f1dd2c0ef93ef9738011250dc
bool beep_on_insert = false;
char currency_symbol[10] = "$";
bool hide_currency_symbol = false;
bool currency_after_amount = false;
<<<<<<< HEAD

=======
>>>>>>> 2d01d40c9a73526f1dd2c0ef93ef9738011250dc
bool authenticated = false;
int ticket_id;

char agent_code[20] = "Default"; // Agent code
time_t agent_login_time; // Time when the agent logged in

<<<<<<< HEAD
=======
Product product;
char query1[50] = "\0";
Product *shopping_cart[50];
int cart_count = 0;
float total = 0.0;
int scroll_offset = 0; // Offset for scrolling
int max_y, max_x;

>>>>>>> 2d01d40c9a73526f1dd2c0ef93ef9738011250dc
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
<<<<<<< HEAD
// Function to load products from a CSV file
void load_products(const char *filename) {
    FILE *file = fopen(filename, "r");
=======

// Función para buscar un producto en el archivo binario
bool search_product_disk(const char *query, Product *result) {
    if (strlen(query)>0) {
        FILE *file = fopen("products.dat", "rb");
        if (!file) {
            perror("Error opening binary product file");
            return false;
        }

        Product temp;
        while (fread(&temp, sizeof(Product), 1, file) == 1) {
            //if (strcmp(temp.ID, query) == 0) {
            if (temp.ID == atoi(query)) {
                *result = temp;
                fclose(file);
                return true;
            }
        }

        fclose(file);
    }
    return false;
}

// Función para añadir un producto al archivo binario
bool add_product_disk(const Product *prod) {
    FILE *file = fopen("products.dat", "ab");
>>>>>>> 2d01d40c9a73526f1dd2c0ef93ef9738011250dc
    if (!file) {
        perror("Error opening binary product file for appending");
        return false;
    }

    if (fwrite(prod, sizeof(Product), 1, file) != 1) {
        perror("Error writing product to binary file");
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

// Función para eliminar un producto del archivo binario
bool delete_product_disk(int ID) {
    FILE *file = fopen("products.dat", "rb");
    if (!file) {
        perror("Error opening binary product file for reading");
        return false;
    }

    FILE *temp = fopen("temp.dat", "wb");
    if (!temp) {
        perror("Error opening temporary file for writing");
        fclose(file);
        return false;
    }

    Product prod;
    bool found = false;
    while (fread(&prod, sizeof(Product), 1, file) == 1) {
        if (prod.ID == ID) {
            found = true;
            continue; // Saltar el producto a eliminar
        }
        fwrite(&prod, sizeof(Product), 1, temp);
    }

    fclose(file);
    fclose(temp);

    if (found) {
        // Reemplazar el archivo original con el temporal
        remove("products.dat");
        rename("temp.dat", "products.dat");
    } else {
        // Si no se encontró, eliminar el archivo temporal
        remove("temp.dat");
    }

    return found;
}

// Función para validar agente y contraseña desde un CSV
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

// Función para leer el último ID desde un archivo
int read_last_id(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 1001; // Iniciar desde 1001 si el archivo no existe
    }

    int last_id;
    if (fscanf(file, "%d", &last_id) != 1) {
        last_id = 1000; // Iniciar desde 1001 si el archivo está vacío o es inválido
    }

    fclose(file);
    return last_id;
}

// Función para actualizar el último ID en un archivo
void update_last_id(const char *filename, int last_id) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening last ID file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%d\n", last_id);
    fclose(file);
}

// Función para guardar una transacción en un archivo CSV
void save_transaction(const char *filename, Product **cart, int count, float total) {
    static const char *id_filename = "last_id.txt";
    ticket_id = read_last_id(id_filename);

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
        fprintf(file, "  %s, %s, %.2f\n", cart[i]->product, cart[i]->product, cart[i]->price);
    }

    fclose(file);

    // Update the last ID
    ticket_id = ticket_id + 1;
    update_last_id(id_filename, ticket_id);
}

// Función para obtener una contraseña de manera segura
void get_password(int x, int y, char *buffer, size_t max_len) {
    size_t i = 0;
    int ch;

    while ((ch = getch()) != '\n' && ch != KEY_ENTER && i < max_len - 1) { // Leer hasta ENTER o llenar el buffer
        if (ch == KEY_BACKSPACE || ch == 127) { // Manejar BACKSPACE
            if (i > 0) {
                i--;
                mvaddch(x, y + i, ' '); // Borrar el último asterisco
                move(x, y + i); // Mover el cursor atrás
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
<<<<<<< HEAD
            strncpy(ticket_agents[ticket_count], agent, sizeof(ticket_agents[ticket_count]) - 1);
            ticket_totals[ticket_count] = total;
            ticket_count++;
=======
            ticket_dates[ticket_count][sizeof(ticket_dates[ticket_count]) - 1] = '\0';
            strncpy(ticket_agents[ticket_count], agent, sizeof(ticket_agents[ticket_count]) - 1);
            ticket_agents[ticket_count][sizeof(ticket_agents[ticket_count]) - 1] = '\0';
            ticket_totals[ticket_count] = total;
            ticket_count++;
            if (ticket_count >= 1000) break; // Limitar a 1000 tickets
>>>>>>> 2d01d40c9a73526f1dd2c0ef93ef9738011250dc
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

<<<<<<< HEAD
        for (int i = start_index, row = 2; i > end_index; i--, row++) {
=======
        for (int i = start_index, row = 2; i > end_index && i >= 0; i--, row++) {
>>>>>>> 2d01d40c9a73526f1dd2c0ef93ef9738011250dc
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

// Función para mostrar una ventana de confirmación
bool show_window(const char *msg) {
    curs_set(0);

    // Crear una pequeña ventana de 5 filas x 40 columnas, centrada
    int height = 5, width = 40;
    int starty = (LINES - height) / 2;
    int startx = (COLS - width) / 2;

    WINDOW *prompt_win = newwin(height, width, starty, startx);
    wbkgd(prompt_win, COLOR_PAIR(4));

    box(prompt_win, 0, 0);  // Dibujar borde
    keypad(prompt_win, TRUE);

    int center_col = (width - (int)strlen(msg)) / 2;
    mvwprintw(prompt_win, 2, center_col, "%s", msg);

    // Mostramos ventana
    wrefresh(prompt_win);

    // Esperar tecla
    nodelay(stdscr, FALSE); // Temporarily block
    int ch = wgetch(prompt_win);
    nodelay(stdscr, TRUE); // Restore non-blocking

    // Analizar la tecla
    //   - Si es 'n' o 'N', consideramos que la respuesta es NO.
    //   - Si es ENTER o 's'/'S' o cualquier otra, por defecto es SÍ.
    bool respuesta_si = true;
    if (ch == 'n' || ch == 'N') {
        respuesta_si = false;
    }
    // ENTER es '\n' (ASCII 10) o KEY_ENTER, dependiendo del terminal.
    // En muchas configuraciones getch() devuelve '\n' al pulsar ENTER.

    // Borrar la ventana
    werase(prompt_win);
    wrefresh(prompt_win);
    delwin(prompt_win);
    curs_set(1);
    return(respuesta_si);
}

// Función para gestionar productos directamente en el archivo binario
void manage_products() {
    clear();
    mvprintw(0, 0, "Product Management");
    mvprintw(2, 0, "1. View products");
    mvprintw(3, 0, "2. Add product");
    mvprintw(4, 0, "3. Delete product");
    mvprintw(5, 0, "q. Exit");

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case '1': { // View products
                clear();
                mvprintw(0, 0, "Product List:");
                mvprintw(1, 0, "ID\tProduct\t\tPrice\tStock");
                mvprintw(2, 0, "----------------------------------------------");

                FILE *file = fopen("products.dat", "rb");
                if (!file) {
                    mvprintw(4, 0, "No products found.");
                    mvprintw(5, 0, "Press any key to continue...");
                    nodelay(stdscr, FALSE);
                    getch();
                    nodelay(stdscr, TRUE);
                    break;
                }

                Product prod;
                int row = 3;
                while (fread(&prod, sizeof(Product), 1, file) == 1) {
                    mvprintw(row++, 0, "%d\t%-15s\t%.2f\t%d", prod.ID, prod.product, prod.price, prod.stock);
                    if (row >= LINES - 2) break; // Evitar desbordamiento de pantalla
                }

                fclose(file);

                mvprintw(row + 1, 0, "Press any key to continue...");
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                break;
            }
            case '2': { // Add product
                clear();
                mvprintw(0, 0, "Add New Product:");

                char name[50];
                float price;
                int stock;
                float price01, price02, price03, price04;

                mvprintw(2, 0, "Product Name: ");
                echo();
                getstr(name);
                noecho();

                mvprintw(3, 0, "Price: ");
                echo();
                scanw("%f", &price);
                noecho();

                mvprintw(4, 0, "Stock: ");
                echo();
                scanw("%d", &stock);
                noecho();

                mvprintw(5, 0, "Price01: ");
                echo();
                scanw("%f", &price01);
                noecho();

                mvprintw(6, 0, "Price02: ");
                echo();
                scanw("%f", &price02);
                noecho();

                mvprintw(7, 0, "Price03: ");
                echo();
                scanw("%f", &price03);
                noecho();

                mvprintw(8, 0, "Price04: ");
                echo();
                scanw("%f", &price04);
                noecho();

                // Obtener el último ID y asignar uno nuevo
                int last_id = read_last_id("last_id.txt");
                Product new_prod;
                new_prod.ID = last_id + 1;
                strncpy(new_prod.product, name, sizeof(new_prod.product) - 1);
                new_prod.product[sizeof(new_prod.product) - 1] = '\0';
                new_prod.price = price;
                new_prod.stock = stock;
                new_prod.price01 = price01;
                new_prod.price02 = price02;
                new_prod.price03 = price03;
                new_prod.price04 = price04;

                if (add_product_disk(&new_prod)) {
                    update_last_id("last_id.txt", new_prod.ID);
                    mvprintw(10, 0, "Product added successfully with ID %d.", new_prod.ID);
                } else {
                    mvprintw(10, 0, "Failed to add product.");
                }

                mvprintw(12, 0, "Press any key to continue...");
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                break;
            }
            case '3': { // Delete product
                clear();
                mvprintw(0, 0, "Delete Product:");

                int del_id;
                mvprintw(2, 0, "Enter Product ID to delete: ");
                echo();
                scanw("%d", &del_id);
                noecho();

                if (delete_product_disk(del_id)) {
                    mvprintw(4, 0, "Product with ID %d deleted successfully.", del_id);
                } else {
                    mvprintw(4, 0, "Product with ID %d not found.", del_id);
                }

                mvprintw(6, 0, "Press any key to continue...");
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                break;
            }
        }
        erase(); // Clear screen without flickering
        mvprintw(0, 0, "Product Management");
        mvprintw(2, 0, "1. View products");
        mvprintw(3, 0, "2. Add product");
        mvprintw(4, 0, "3. Delete product");
        mvprintw(5, 0, "q. Exit");
    }
}

// Función para gestionar usuarios (sin cambios, ya que no está relacionado con los productos)
void manage_users() {
    clear();
    mvprintw(0, 0, "User Management");
    mvprintw(2, 0, "1. View users");
    mvprintw(3, 0, "2. Add user");
    mvprintw(4, 0, "3. Delete user");
    mvprintw(5, 0, "q. Exit");

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case '1':
                clear();
                mvprintw(0, 0, "User List:");
                mvprintw(1, 0, "User ID\tUsername");
                mvprintw(2, 0, "--------------------------");
                for (int i = 0; i < 100; i++) {
                    mvprintw(i + 3, 0, "%d\tUser%d", i + 1, i + 1);
                }
                mvprintw(104, 0, "Press any key to continue...");
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                break;
            case '2':
                clear();
                mvprintw(0, 0, "Add New User:");
                char username[20];
                echo();
                getstr(username);
                noecho();
                mvprintw(2, 0, "User added: %s", username);
                mvprintw(4, 0, "Press any key to continue...");
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                break;
            case '3':
                clear();
                mvprintw(0, 0, "Delete User:");
                int user_id;
                echo();
                mvprintw(2, 0, "Enter User ID to delete: ");
                scanw("%d", &user_id);
                noecho();
                mvprintw(4, 0, "User with ID %d deleted.", user_id);
                mvprintw(6, 0, "Press any key to continue...");
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                break;
        }
        erase(); // Clear screen without flickering
        mvprintw(0, 0, "User Management");
        mvprintw(2, 0, "1. View users");
        mvprintw(3, 0, "2. Add user");
        mvprintw(4, 0, "3. Delete user");
        mvprintw(5, 0, "q. Exit");
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
                } 
                if (highlight == 1) {
                    delwin(menu_win);
                    manage_users();
                    return 0;
                } else if (highlight == 2) {
                    delwin(menu_win);
                    list_tickets("transactions.csv");
                    return 0;
                } else if (highlight == 3 || highlight == n_choices - 1) { // Exit
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

/* What we do when we're all set to exit */
void finish(void) {
    curs_set(1);
    clear();
    refresh();
    resetty();
    endwin();
    exit(0);
}

void print_text(int y, int x, const char *name, const char *value, ...) {
    va_list args;
    int len = strlen(name);
    va_start(args, value);
    attron(COLOR_PAIR(5));
    mvprintw(y, x, "%s:", name);
    //mvaddch(y, x + len, ':');
    attroff(COLOR_PAIR(5));
    move(y, x + len + 2);
    vwprintw(stdscr, value, args);
    //mvprintw(y, x + len + 2, value, args);
    va_end(args);
}

<<<<<<< HEAD
    load_config("config.ini");

    // Load products from CSV
    load_products("products.csv");
=======
void draw_time() {
    // Display current time in the top-right corner
    curs_set(0);
    int x=0, y=0;
    getyx(stdscr, y, x);
>>>>>>> 2d01d40c9a73526f1dd2c0ef93ef9738011250dc

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

    if (!authenticated)
        mvprintw(0, 1, "Agent: - no agent -");
    else
        mvprintw(0, 1, "Agent: %s (%02d:%02d:%02d)", agent_code, hours, minutes, seconds);
    move(x,y);
    curs_set(1);
}

void update_mainscreen() {
        curs_set(0);
        mvprintw(2, 1, "CODE:");
        attron(COLOR_PAIR(2));
        mvprintw(2, 7, "%13s", query1);
        attroff(COLOR_PAIR(2));
        mvprintw(2, 21, "EAN13");

        bool search = search_product_disk(query1, &product);
        
        print_text( 4, 1, "ID","%-13d", !search?0:product.ID);
        //print_text( 5, 1, "Producto","%-38.38s", "----------------------------------------------------------------------------------------");
        print_text( 5, 1, "Producto","%-38.38s", !search?"-":product.product);
        print_text( 6, 1, "Stock","%-13d", !search?0:product.stock);
        print_text( 7, 1, "Fabricante","%-36.36s", !search?"":product.fabricante);
        print_text( 8, 1, "Proveedor","%-37.37s", !search?"":product.proveedor);
        print_text( 9, 1, "Departamento","%-34.34s", !search?"":product.departamento);
        print_text(10, 1, "Clase","%-38.38s", !search?"":product.clase);
        print_text(11, 1, "Subclase","%-38.38s", !search?"":product.subclase);
        print_text(12, 1, "Descripción 1","%-33.33s", !search?"":product.descripcion1);
        print_text(14, 1, "Descripción 2","%-33.33s", !search?"":product.descripcion2);
        print_text(16, 1, "Descripción 3","%-33.33s", !search?"":product.descripcion3);
        print_text(18, 1, "Descripción 4","%-33.33s", !search?"":product.descripcion4);

        print_text(20, 1, "Precio 1","%-7.2f", !search?0.0:product.price);
        print_text(21, 1, "Precio 2","%-7.2f", !search?0.0:product.price01);
        print_text(22, 1, "Precio 3","%-7.2f", !search?0.0:product.price02);
        print_text(23, 1, "Precio 4","%-7.2f", !search?0.0:product.price03);
        print_text(25, 1, "IVA","%-38.38s", !search?"":product.tipo_IVA);

        // Right panel for shopping cart
        int x_offset = max_x / 2 + 2;
        attron(COLOR_PAIR(1));
        mvprintw(0, x_offset + 2, "Total: %.2f", total);
        mvprintw(0, max_x - 26, "[ticket %d]", ticket_id);
        attroff(COLOR_PAIR(1));

        for (int i = scroll_offset; i < cart_count && i - scroll_offset < max_y - 3; i++) {
            mvprintw((i - scroll_offset) + 2, x_offset, "%c %03d %-25.25s - %7.2f", (i+1)==cart_count?'>':' ' , i + 1, shopping_cart[i]->product, shopping_cart[i]->price);
        }
        
        curs_set(1);
}


void draw_mainscreen() {
        erase(); // Clear screen without flickering
        curs_set(0);
        getmaxyx(stdscr, max_y, max_x);

        // Draw vertical separator
        for (int y = 2; y < max_y-2; y++) {
            mvaddch(y, 0, ACS_VLINE);
            mvaddch(y, max_x / 2, ACS_VLINE);
            mvaddch(y, max_x - 1, ACS_VLINE);
        }

        // Líneas horizontales (superior e inferior)
        for (int col = 1; col < max_x; col++) {
            mvaddch(1, col, ACS_HLINE);
            mvaddch(max_y - 2, col, ACS_HLINE);
        }

        // Esquinas
        mvaddch(1, 0, ACS_ULCORNER);                         // esquina sup izq
        mvaddch(1, max_x - 1, ACS_URCORNER);                 // esquina sup der
        mvaddch(max_y - 2, 0, ACS_LLCORNER);                // esquina inf izq
        mvaddch(max_y - 2, max_x - 1, ACS_LRCORNER);        // esquina inf der

        mvaddch(1, max_x/2, ACS_TTEE);
        mvaddch(max_y - 2, max_x/2, ACS_BTEE);

#ifdef DEBUG
        static int ch_old;
        if (ch != -1) {
            ch_old = ch;
        }
        mvprintw(max_y - 2, 0, "KEY: %d", ch_old);
#endif

        attron(COLOR_PAIR(3));

        // Draw horizontal
        for (int x = 0; x < max_x; x++) {
            mvaddch(max_y - 1, x, ' ');
        }

        // Menu inferior
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

        move(2, 19); // Place the cursor at the end of the input field
        curs_set(1);
        /*
        static int n = 0;
        mvprintw(max_y - 2, 2, "draw_mainscreen: %d ", n);
        n=n+1;
        */
}


int main() {
    load_config("config.ini");
    static const char *id_filename = "last_id.txt";
    ticket_id = read_last_id(id_filename);

    // Initialize ncurses
    initscr();
    cbreak();
    noecho(); // Disable echo for smoother UI
    curs_set(FALSE); // Ocultar el cursor
    keypad(stdscr, TRUE); // Habilitar teclas especiales
    nodelay(stdscr, TRUE); // Entrada non-blocking
    
    start_color();

    // Define colors
    init_color(COLOR_CYAN, 50, 50, 50); // (0 - 1000)
    init_color(COLOR_BLUE, 0, 0, 500);
    init_color(COLOR_YELLOW, 1000, 1000, 0);
    init_color(COLOR_RED, 811, 284, 0);

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_CYAN);
    init_pair(3, COLOR_WHITE, COLOR_BLUE);
    init_pair(4, COLOR_YELLOW, COLOR_BLUE);
    init_pair(5, COLOR_RED, COLOR_BLACK);

    agent_login_time = time(NULL); // Record the login time
    
    int ch = 0;
    int draw = true;
    int run = true;

    //nodelay(stdscr, TRUE);
    draw_mainscreen();

    while (run) {
        ch = getch();
        if (ch != ERR) 
        switch (ch) {
            case KEY_RESIZE:
                getmaxyx(stdscr, max_y, max_x);
                draw=true;
                break;

            case 'M':
            case 'm':
                ch = manage_menu();
                draw=true;
                break;

            case 'S':
            case 's':
                system("scrot -u");
                break;

            case KEY_BACKSPACE:
            case 127:
                query1[strlen(query1) - 1] = '\0';
                draw = true;
                break;

            case KEY_UP:
                if (scroll_offset > 0) {
                    scroll_offset--;
                    draw = true;
                }
                break;

            case KEY_DOWN:
                if (scroll_offset < cart_count - (max_y - 3)) {
                    scroll_offset++;
                    draw = true;
                }
                break;

            case 'Q':
            case 'q':
                run = false;
                break;

            case 'D':
            case 'd':
                nodelay(stdscr, FALSE); // Temporarily block
                mvprintw(max_y - 3, max_x / 2 + 2, "POS to delete (last by default): ");
                echo();
                char pos_input[10];
                getstr(pos_input);
                noecho();
                int pos;
                if (strlen(pos_input) == 0) { 
                    pos = cart_count - 1; 
                }
                else pos = atoi(pos_input) - 1;
                if (pos >= 0 && pos < cart_count) {
                    mvprintw(max_y - 3, max_x / 2 + 2, "Delete %-15.15s (%d)? (Y/n): ", shopping_cart[pos]->product, pos + 1);
                    int confirm = getch();
                    if (confirm != 'n' && confirm != 'N') {
                        total -= shopping_cart[pos]->price;
                        free(shopping_cart[pos]);
                        for (int i = pos; i < cart_count - 1; i++) {
                            shopping_cart[i] = shopping_cart[i + 1];
                        }
                        cart_count--;
                    } else {
                        mvprintw(max_y - 2, max_x / 2 + 2, "Item not removed. Press key ...");
                        curs_set(0);
                        getch();
                        curs_set(1);
                    }
                } else {
                    mvprintw(max_y - 2, max_x / 2 + 2, "Invalid position. Press key ...");
                    curs_set(0);
                    getch();
                    curs_set(1);
                }
                nodelay(stdscr, TRUE); // Restore non-blocking
                draw = true;
                break;

            case 'P':
            case 'p':
                if (show_window("Checkout? (Y/n)")) {
                    save_transaction("transactions.csv", shopping_cart, cart_count, total);
                    for (int i = 0; i < cart_count; i++) {
                        free(shopping_cart[i]);
                    }
                    cart_count = 0;
                    total = 0.0;
                };
                draw = true;
                draw_mainscreen();
                break;

            case 'A':
            case 'a':
                nodelay(stdscr, FALSE); // Temporarily block
                mvprintw(max_y - 3, 0, "Enter agent code (ENTER='no agent'): ");
                echo();
                char new_agent_code[20];
                char psw[20];
                getstr(new_agent_code);
                mvprintw(max_y - 3, 0, "Enter agent password:                  ");
                noecho();
                get_password(max_y - 3, 22, psw, sizeof(psw));
                if (validate_agent_and_password("agents.csv", new_agent_code, psw)) {
                    strncpy(agent_code, new_agent_code, sizeof(agent_code) - 1);
                    agent_code[sizeof(agent_code) - 1] = '\0';
                    agent_login_time = time(NULL); // Record the login time
                    authenticated = true;
                } else {
                    mvprintw(max_y - 2, 0, "Invalid agent code.");
                    getch();
                }
                nodelay(stdscr, TRUE); // Restore non-blocking
                break;

            case 'N':
            case 'n':
                mvprintw(max_y - 3, 0, "Enter product name: ");
                echo();
                char product_name[50];                
                nodelay(stdscr, FALSE); // Temporarily block
                getstr(product_name);
                noecho();
                // Realizar búsqueda por nombre
                if (search_product_disk(product_name, &product)) {
                    // Añadir al carrito
                    Product *prod_ptr = malloc(sizeof(Product));
                    if (prod_ptr == NULL) {
                        mvprintw(max_y - 2, 0, "Memory allocation error.");
                        nodelay(stdscr, FALSE); // Temporarily block
                        getch();
                        nodelay(stdscr, TRUE); // Restore non-blocking
                    } else {
                        *prod_ptr = product;
                        shopping_cart[cart_count++] = prod_ptr;
                        total += product.price;
                        if (beep_on_insert) {
                            beep();
                        }
                    }
                }
                break;

            case 10:
            case KEY_ENTER:
                // Añadir al carrito
                Product *prod_ptr = malloc(sizeof(Product));
                *prod_ptr = product;
                shopping_cart[cart_count++] = prod_ptr;
                total += product.price;
                query1[0] = '\0'; // Clear the input field
                if (beep_on_insert) beep();
                draw = true;
            
            default:
                int l = strlen(query1);
                char text[50] = "";
                strcpy(text, query1);
                if (l < 13 && isprint(ch)) {
                    query1[l] = (char)ch;
                    query1[l + 1] = '\0';
                    draw = true;
                };
                bool found = search_product_disk(query1, &product);
                break;

        }

        // Delay to refresh every 100ms for smooth updates
        if (draw) {
            update_mainscreen();
            draw = false;
        }

        draw_time();
        usleep(100000);
    }

    finish();
    return 0;
}
