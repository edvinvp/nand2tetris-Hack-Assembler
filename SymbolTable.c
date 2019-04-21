#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "LinkedListSymbol.c"

/*
 * Simple hash table for the asm to hack assembler.
 * Supports add, contains and get operations. No deletion.
 */

typedef struct SymbolTable {
    SymbolList **table;
    int capacity;
    int count; 
} SymbolTable;


unsigned long hash(unsigned char *str);
int containsSymbol(SymbolTable *table, char *symbol);
int addSymbol(SymbolTable *table, char *symbol, int address);
int getAddress(SymbolTable *table, char *symbol);
void printSymbolTable(SymbolTable *table);
void testSymbolTable(void);

// String hash function
// djb2 from http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
	hash = ((hash << 5) + hash) + c;

    return hash;
}

int addSymbol(SymbolTable *table, char *symbol, int address)
{
    if (symbol == NULL)
	return -1;

    unsigned long symbolHash = hash(symbol) % table->capacity;
    addSymbolToList(&(table->table[symbolHash]), symbol, address);
    table->count++;
}

int getAddress(SymbolTable *table, char *symbol)
{
    if (symbol == NULL)
	return -1;

    unsigned long symbolHash = hash(symbol) % table->capacity;
    SymbolList *sym = getSymbolFromList(table->table[symbolHash], symbol);
    return sym->address;
}

int containsSymbol(SymbolTable *table, char *symbol)
{
    unsigned long symbolHash = hash(symbol) % table->capacity;
    SymbolList *sym = getSymbolFromList(table->table[symbolHash], symbol);
    if (sym == NULL)
	return 0;
    else
	return 1;
}

void printSymbolTable(SymbolTable *table)
{
    for (int i = 0; i < table->capacity; i++) {
	printf("[%d] = ", i);
	printSymbolList(table->table[i]);
    }
}

void freeSymbolTable(SymbolTable **table)
{
    for (int i = 0; i < (*table)->capacity; i++) {
	freeSymbolList(&((*table)->table[i]));
    }
    free(*table);
}
/*
void testSymbolTable(void)
{
    SymbolTable *table = malloc(sizeof(SymbolTable));
    table->capacity = 10;
    table->count = 0;
    table->table = malloc(sizeof(table->table) * table->capacity);
    char sym1[5] = "sym1";
    char sym2[5] = "sym2";
    char sym3[5] = "sym3";
    char sym4[5] = "sym4";
    char sym5[5] = "sym5";
    char sym6[5] = "sym6";
    char sym7[5] = "sym7";
    char sym8[5] = "sym8";
    char sym9[5] = "sym9";
    char sym10[5] = "sym10";
    char sym11[5] = "sym11";
    addSymbol(table, sym1, 1);
    addSymbol(table, sym2, 2);
    addSymbol(table, sym3, 3);
    addSymbol(table, sym4, 4);
    addSymbol(table, sym5, 5);
    addSymbol(table, sym6, 6);
    addSymbol(table, sym7, 7);
    addSymbol(table, sym8, 8);
    addSymbol(table, sym9, 9);
    addSymbol(table, sym10, 10);
    addSymbol(table, sym11, 11);
    printSymbolTable(table);
    printf("get address (sym1, 1). Got %d\n", getAddress(table, sym1));
    printf("get address (sym2, 2). Got %d\n", getAddress(table, sym2));
    printf("get address (sym3, 3). Got %d\n", getAddress(table, sym3));
    printf("get address (sym4, 4). Got %d\n", getAddress(table, sym4));
    printf("get address (sym5, 5). Got %d\n", getAddress(table, sym5));
    printf("get address (sytable, m6, 6). Got %d\n", getAddress(table, sym6));
    printf("get address (sym7, 7). Got %d\n", getAddress(table, sym7));
    printf("get addtable, ress (sym8, 8). Got %d\n", getAddress(table, sym8));
    printf("get address (sym9, 9). Got %d\n", getAddress(table, sym9));
    printf("get address (sym10, 10). Got %d\n", getAddress(table, sym10));
    printf("get address (sym11, 11). Got %d\n", getAddress(table, sym11));
    freeSymbolTable(&table);
}

int main(void)
{
    testSymbolTable();
    return 0;
}
*/
