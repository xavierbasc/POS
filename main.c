#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h> // For usleep()
#include <stdbool.h> // For boolean values
#include <menu.h> // For menus

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

bool beep_on_insert = false; // Configuration for beep
bool authenticated = false;

char agent_code[20] = "Default"; // Agent code
time_t agent_login_time; // Time when the agent logged in

// Function to load configuration from config.ini
void load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "beep_on_insert=1", 16) == 0) {
            beep_on_insert = true;
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

// Function to append a transaction to a CSV file
void save_transaction(const char *filename, Product **cart, int count, float total) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Error opening transaction file");
        exit(EXIT_FAILURE);
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char datetime[20];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", t);

    static int ticket_id = 1;
    fprintf(file, "Ticket %d, Agent: %s, Date: %s, Total: %.2f\n", ticket_id++, agent_code, datetime, total);
    for (int i = 0; i < count; i++) {
        fprintf(file, "  %s, %s, %.2f\n", cart[i]->code, cart[i]->name, cart[i]->price);
    }

    fclose(file);
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


// Function to display the product management menu
int manage_menu() {
    WINDOW *menu_win;
    int menu_height = 8, menu_width = 40;
    int start_y = (LINES - menu_height) / 2;
    int start_x = (COLS - menu_width) / 2;

    menu_win = newwin(menu_height, menu_width, start_y, start_x);
    box(menu_win, 0, 0);
    keypad(menu_win, TRUE);

    // Set the background color to blue
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
            if (strlen(choices[i]) == 0) { // Si es una línea en blanco, no mostrar selección
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

        // Refresh the window to reflect the background
        wrefresh(menu_win);

        int ch = wgetch(menu_win);
        switch (ch) {
            case KEY_UP:
                do {
                    highlight = (highlight - 1 + n_choices) % n_choices;
                } while (strlen(choices[highlight]) == 0); // Saltar líneas vacías
                break;
            case KEY_DOWN:
                do {
                    highlight = (highlight + 1) % n_choices;
                } while (strlen(choices[highlight]) == 0); // Saltar líneas vacías
                break;
            case 9:
            case '9':
                delwin(menu_win);
                return 'q';
            case 'q':
                delwin(menu_win);
                return 0;
            case 10: // Enter key
                choice = highlight;
                if (choice == 3) { // Exit option
                    delwin(menu_win);
                    return 'q';
                } else if (choice == n_choices - 1) { // Exit option
                    curs_set(1);
                    delwin(menu_win);
                    return;
                } else {
                    mvwprintw(menu_win, menu_height - 2, 1, "Selected: %s", choices[choice]);
                    wrefresh(menu_win);
                    // Here you can call functions to manage products based on the choice
                    usleep(500000); // Simulate some processing time
                }
                break;
        }
    }
    return 0;
}

int main() {
    Product *product = NULL;

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
                mvprintw(0, x_offset + 25, "> Checkout? (y/N)");
                nodelay(stdscr, FALSE); // Temporarily block
                int ch = getch();
                nodelay(stdscr, TRUE); // Restore non-blocking
                if (ch == 'Y' || ch == 'y') {
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
