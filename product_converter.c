#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PRODUCTS 10000

typedef struct {
    int    ID;
    char   EAN13[14];         // Código EAN13: 13 caracteres + terminador nulo
    char   product[50];
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
} Product;

/**
 * Lee un archivo CSV con columnas:
 *   ID,EAN13,product,price,stock,price01,price02,price03,price04,fabricante,proveedor,departamento,clase,subclase,tipo_IVA
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
    char line[512]; // Aumentar el tamaño para manejar líneas más largas

    // Leer la línea de encabezado y verificar si coincide con el formato esperado
    if (fgets(line, sizeof(line), f)) {
        // Opcional: verificar encabezados
        // Puedes implementar una verificación más estricta si lo deseas
    }

    while (fgets(line, sizeof(line), f) && count < MAX_PRODUCTS) {
        // Saltar líneas vacías o con salto de línea
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        Product temp;
        memset(&temp, 0, sizeof(Product));

        // Formato esperado (15 campos):
        // ID,EAN13,product,price,stock,price01,price02,price03,price04,fabricante,proveedor,departamento,clase,subclase,tipo_IVA
        int fields = sscanf(line, "%d,%13[^,],%49[^,],%f,%d,%f,%f,%f,%f,%49[^,],%49[^,],%49[^,],%49[^,],%49[^,],%19[^,\n]",
                            &temp.ID,
                            temp.EAN13,
                            temp.product,
                            &temp.price,
                            &temp.stock,
                            &temp.price01,
                            &temp.price02,
                            &temp.price03,
                            &temp.price04,
                            temp.fabricante,
                            temp.proveedor,
                            temp.departamento,
                            temp.clase,
                            temp.subclase,
                            temp.tipo_IVA
        );

        if (fields == 15) {
            // Validar que tipo_IVA sea "reducido" o "super reducido"
            if (strcasecmp(temp.tipo_IVA, "reducido") != 0 && strcasecmp(temp.tipo_IVA, "super reducido") != 0) {
                fprintf(stderr, "Tipo de IVA inválido en línea: %s\n", line);
                continue; // Saltar este registro
            }

            products[count++] = temp;
        } else {
            fprintf(stderr, "Línea CSV mal formateada o incompleta: %s\n", line);
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
 * Guarda en CSV la información de un arreglo de productos.
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

    // Escribir encabezado
    fprintf(f, "ID,EAN13,product,price,stock,price01,price02,price03,price04,fabricante,proveedor,departamento,clase,subclase,tipo_IVA\n");

    for (int i = 0; i < count; i++) {
        fprintf(f, "%d,%s,%s,%.2f,%d,%.2f,%.2f,%.2f,%.2f,%s,%s,%s,%s,%s,%s\n",
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
                products[i].tipo_IVA
        );
    }

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
