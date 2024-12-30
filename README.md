# Point Of Sale (POS)

## Prerequisites
sudo apt-get install libncurses5-dev libncursesw5-dev


# Product Converter

This C program converts a list of products between two formats:

- **CSV** (plain text)  
- **Binary** (`.dat`)

## What does the program do?

1. **`csv2bin`:** Converts a CSV file to a binary file, so each CSV row is turned into a `Product` structure in the binary file.

2. **`bin2csv`:** Converts a binary file of products back to CSV, keeping the same fields.

The data structure `Product` is defined as follows:

```c
typedef struct {
    int   ID;
    char  product[50];
    float price;
    int   stock;
    float price01;
    float price02;
    float price03;
    float price04;
} Product;
```

## Expected CSV Format

The CSV file must have **eight** columns in the following order:
```
ID,product,price,stock,price01,price02,price03,price04
```

Example:

```
1001,Martillo,100.00,50,95.00,90.00,85.00,80.00
1002,Destornillador,75.50,20,70.00,68.00,65.00,60.00
...
```

## How to Use the Program

Compile the program (e.g., with `gcc`):

```bash
gcc product_converter.c -o product_converter
```

Then run it in one of the following ways:

1. **Convert CSV to binary**:

   ```bash
   ./product_converter csv2bin <csvFile> <binFile>
   ```

   - `<csvFile>` is the input CSV file.  
   - `<binFile>` is the output binary file to be generated.  

   For example:

   ```bash
   ./product_converter csv2bin products.csv products.dat
   ```

2. **Convert binary to CSV**:

   ```bash
   ./product_converter bin2csv <binFile> <csvFile>
   ```

   - `<binFile>` is the input binary file.  
   - `<csvFile>` is the output CSV file to be generated.  

   For example:

   ```bash
   ./product_converter bin2csv products.dat products_out.csv
   ```

## Main Functions in the Code

- `load_from_csv(...)`:  
  - Opens the CSV and **reads** each line, filling an array of `Product` in memory.

- `save_to_binary(...)`:  
  - Takes the `Product` array in memory and **writes** it to a binary file.

- `load_from_binary(...)`:  
  - Opens the binary file, **reads** each record into `Product` structures.

- `save_to_csv(...)`:  
  - Takes the products in memory and **writes** them to a CSV file.

## Notes

- The program assumes a maximum of `MAX_PRODUCTS` (by default 10,000). If your CSV or binary file contains more, you can adjust `MAX_PRODUCTS`.
- If there are malformed lines in the CSV (for instance, fewer than 8 fields), an error message is printed and those lines are skipped.

Enjoy converting your product data between CSV and binary as needed!