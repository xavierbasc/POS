#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PRODUCTS 10000
#define MAX_FIELDS 19  // Actualizado para incluir los nuevos campos de descripción

typedef struct {
    int    ID;
    char   EAN13[14];         // Código EAN13: 13 caracteres + terminador nulo
    char   product[100];      // Aumentado para acomodar descripciones más largas
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

/**
 * Función auxiliar para eliminar espacios en blanco al inicio y al final de una cadena.
 */
void trim(char *str) {
    // Eliminar espacios al inicio
    char *start = str;
    while(isspace((unsigned char)*start)) start++;
    if(start != str) memmove(str, start, strlen(start) + 1);

    // Eliminar espacios al final
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = '\0';
}

/**
 * Función para parsear una línea de CSV respetando comillas.
 * Retorna la cantidad de campos parseados.
 */
int parse_csv_line(char *line, char **fields, int max_fields) {
    int count = 0;
    char *ptr = line;
    while (*ptr && count < max_fields) {
        // Si el campo comienza con comillas
        if (*ptr == '\"') {
            ptr++; // Saltar la comilla inicial
            fields[count++] = ptr;
            // Buscar la comilla de cierre
            while (*ptr && !(*ptr == '\"' && (*(ptr+1) == ',' || *(ptr+1) == '\0' || *(ptr+1) == '\n'))) {
                if (*ptr == '\"' && *(ptr+1) == '\"') {
                    // Doble comilla encontrada, saltar una
                    memmove(ptr, ptr+1, strlen(ptr));
                }
                ptr++;
            }
            if (*ptr == '\"') {
                *ptr = '\0'; // Terminar el campo
                ptr += 2; // Saltar la comilla de cierre y la coma
            }
        } else {
            // Campo sin comillas
            fields[count++] = ptr;
            while (*ptr && *ptr != ',') ptr++;
            if (*ptr == ',') {
                *ptr = '\0';
                ptr++;
            }
        }

        // Trim espacios
        trim(fields[count-1]);
    }
    return count;
}

/**
 * Lee un archivo CSV con columnas adicionales:
 *   descripcion1, descripcion2, descripcion3, descripcion4
 * y retorna la cantidad de registros leídos en el array 'products'.
 * 
 * @param filename  Nombre del archivo CSV
 * @param products  Arreglo donde se guardan los registros
 * @return          Cantidad de registros leídos
 */
int load_from_csv(const char *filename, Product *products) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("No se pudo abrir el archivo CSV");
        return -1;
    }

    int count = 0;
    char line[2048]; // Aumentado el tamaño para manejar líneas más largas

    // Leer la línea de encabezado y verificar si coincide con el formato esperado
    if (fgets(line, sizeof(line), f)) {
        // Opcional: verificar encabezados
        // Puedes implementar una verificación más estricta si lo deseas
    }

    while (fgets(line, sizeof(line), f) && count < MAX_PRODUCTS) {
        // Saltar líneas vacías o con solo saltos de línea
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        Product temp;
        memset(&temp, 0, sizeof(Product));

        // Array para almacenar punteros a los campos
        char *fields[MAX_FIELDS];
        int num_fields = parse_csv_line(line, fields, MAX_FIELDS);

        if (num_fields < 19) { // Verificar si hay al menos 19 campos
            fprintf(stderr, "Línea CSV mal formateada o incompleta: %s\n", line);
            continue;
        }

        // Asignar campos al struct
        temp.ID = atoi(fields[0]);
        strncpy(temp.EAN13, fields[1], sizeof(temp.EAN13)-1);
        strncpy(temp.product, fields[2], sizeof(temp.product)-1);
        temp.price = atof(fields[3]);
        temp.stock = atoi(fields[4]);
        temp.price01 = atof(fields[5]);
        temp.price02 = atof(fields[6]);
        temp.price03 = atof(fields[7]);
        temp.price04 = atof(fields[8]);
        strncpy(temp.fabricante, fields[9], sizeof(temp.fabricante)-1);
        strncpy(temp.proveedor, fields[10], sizeof(temp.proveedor)-1);
        strncpy(temp.departamento, fields[11], sizeof(temp.departamento)-1);
        strncpy(temp.clase, fields[12], sizeof(temp.clase)-1);
        strncpy(temp.subclase, fields[13], sizeof(temp.subclase)-1);
        strncpy(temp.tipo_IVA, fields[14], sizeof(temp.tipo_IVA)-1);
        strncpy(temp.descripcion1, fields[15], sizeof(temp.descripcion1)-1);
        strncpy(temp.descripcion2, fields[16], sizeof(temp.descripcion2)-1);
        strncpy(temp.descripcion3, fields[17], sizeof(temp.descripcion3)-1);
        strncpy(temp.descripcion4, fields[18], sizeof(temp.descripcion4)-1);

        // Validar que tipo_IVA sea "reducido" o "super reducido"
        if (strcasecmp(temp.tipo_IVA, "reducido") != 0 && strcasecmp(temp.tipo_IVA, "super reducido") != 0) {
            fprintf(stderr, "Tipo de IVA inválido en línea: %s\n", line);
            continue; // Saltar este registro
        }

        products[count++] = temp;
    }

    fclose(f);
    return count;
}

/**
 * Guarda en CSV la información de un arreglo de productos, incluyendo los nuevos campos de descripción.
 *
 * @param filename  Nombre del archivo CSV a generar
 * @param products  Arreglo con los productos
 * @param count     Cantidad de productos
 */
void save_to_csv(const char *filename, Product *products, int count) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("No se pudo abrir el archivo CSV para escribir");
        return;
    }

    // Escribir encabezado con los nuevos campos de descripción
    fprintf(f, "ID,EAN13,product,price,stock,price01,price02,price03,price04,fabricante,proveedor,departamento,clase,subclase,tipo_IVA,descripcion1,descripcion2,descripcion3,descripcion4\n");

    for (int i = 0; i < count; i++) {
        // Función para encerrar campos en comillas y escapar comillas internas
        #define ESCAPE_AND_QUOTE(field) "\"" , \
            ({ \
                char *temp_field = malloc(strlen(products[i].field)*2 + 1); \
                if (!temp_field) { \
                    fprintf(stderr, "Error de memoria\n"); \
                    exit(1); \
                } \
                char *src = products[i].field; \
                char *dst = temp_field; \
                while (*src) { \
                    if (*src == '\"') { \
                        *dst++ = '\"'; \
                    } \
                    *dst++ = *src++; \
                } \
                *dst = '\0'; \
                temp_field; \
            }) , "\"" 

        // Preparar cada campo entrecomillado si es necesario
        fprintf(f, "%d,%s,%s,%.2f,%d,%.2f,%.2f,%.2f,%.2f,%s,%s,%s,%s,%s,%s,\"%s\",\"%s\",\"%s\",\"%s\"\n",
                products[i].ID,
                products[i].EAN13,
                // Encerrar en comillas y manejar comas dentro
                products[i].product,
                products[i].price,
                products[i].stock,
                products[i].price01,
                products[i].price02,
                products[i].price03,
                products[i].price04,
                products[i].fabricante,
                products[i].proveedor,
                products[i].departamento,
                products[i].clase,
                products[i].subclase,
                products[i].tipo_IVA,
                products[i].descripcion1,
                products[i].descripcion2,
                products[i].descripcion3,
                products[i].descripcion4
        );

        /*
        // Alternativa con manejo de comillas:
        // Implementar una función para encerrar en comillas y escapar comillas internas
        // Para simplificar, aquí se asume que los campos de descripción no contienen comillas
        fprintf(f, "%d,\"%s\",\"%s\",%.2f,%d,%.2f,%.2f,%.2f,%.2f,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                products[i].ID,
                products[i].EAN13,
                products[i].product,
                products[i].price,
                products[i].stock,
                products[i].price01,
                products[i].price02,
                products[i].price03,
                products[i].price04,
                products[i].fabricante,
                products[i].proveedor,
                products[i].departamento,
                products[i].clase,
                products[i].subclase,
                products[i].tipo_IVA,
                products[i].descripcion1,
                products[i].descripcion2,
                products[i].descripcion3,
                products[i].descripcion4
        );
        */
    }

    fclose(f);
}

/**
 * Lee un archivo binario de productos y los almacena en 'products'.
 *
 * @param filename  Nombre del archivo binario
 * @param products  Arreglo donde se guardan los productos leídos
 * @return          Cantidad de productos leídos
 */
int load_from_binary(const char *filename, Product *products) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("No se pudo abrir el archivo binario para lectura");
        return -1;
    }

    // Leemos hasta MAX_PRODUCTS
    int count = 0;
    while (!feof(f) && count < MAX_PRODUCTS) {
        size_t readCount = fread(&products[count], sizeof(Product), 1, f);
        if (readCount == 1) {
            count++;
        }
    }

    fclose(f);
    return count;
}

/**
 * Guarda un arreglo de 'count' productos en un archivo binario.
 *
 * @param filename  Nombre del archivo binario
 * @param products  Arreglo de productos
 * @param count     Cantidad de productos a guardar
 */
void save_to_binary(const char *filename, Product *products, int count) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("No se pudo abrir el archivo binario para escribir");
        return;
    }

    // Escribimos uno a uno los productos
    fwrite(products, sizeof(Product), count, f);
    fclose(f);
}

/**
 * Programa principal que según el modo (csv2bin o bin2csv),
 * convierte archivos CSV a binario o binario a CSV.
 *
 * Uso:
 *   ./product_converter csv2bin <archivoCSV> <archivoBin>
 *   ./product_converter bin2csv <archivoBin> <archivoCSV>
 */
int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s [csv2bin|bin2csv] <archivoEntrada> <archivoSalida>\n", argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const char *inputFile = argv[2];
    const char *outputFile = argv[3];

    Product products[MAX_PRODUCTS];
    int count;

    if (strcmp(mode, "csv2bin") == 0) {
        // Leer CSV, guardar bin
        count = load_from_csv(inputFile, products);
        if (count >= 0) {
            save_to_binary(outputFile, products, count);
            printf("Convertido CSV a binario: %d registros.\n", count);
        }
    } else if (strcmp(mode, "bin2csv") == 0) {
        // Leer bin, guardar CSV
        count = load_from_binary(inputFile, products);
        if (count >= 0) {
            save_to_csv(outputFile, products, count);
            printf("Convertido binario a CSV: %d registros.\n", count);
        }
    } else {
        fprintf(stderr, "Modo no reconocido. Use 'csv2bin' o 'bin2csv'.\n");
        return 1;
    }

    return 0;
}
