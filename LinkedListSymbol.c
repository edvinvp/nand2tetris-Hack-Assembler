#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Simple single-linked list for the asm to hack assembler.
 * Supports add and get operations. No deletion.
 */

typedef struct SymbolList {
    char *symbol;
    int address;
    struct SymbolList *nextSymbol;
} SymbolList;

void addSymbolToList(SymbolList** head, char *symbol, int address);
SymbolList *getSymbolFromList(SymbolList* head, char *symbol);
void printSymbolList(SymbolList* head);
void freeSymbolList(SymbolList** head);

void printSymbolList(SymbolList* head)
{
    SymbolList *tmp = head;
    while (tmp != NULL) {
	printf("(%s,%d),", tmp->symbol, tmp->address);
	tmp = tmp->nextSymbol;
    }
    printf("\n");
}


void freeSymbolList(SymbolList** head)
{
    SymbolList* tmp;
    while (*head != NULL) {
	tmp = *head;
        *head = (*head)->nextSymbol;
	free(tmp->symbol);
	free(tmp);
    }
}


void addSymbolToList(SymbolList **head, char *symbol, int address)
{
    // allocate memory for new node
    SymbolList *newSymbol = malloc(sizeof(SymbolList));
    newSymbol->symbol = malloc((strlen(symbol) + 1) * sizeof(char));
    strcpy(newSymbol->symbol, symbol);
    newSymbol->address = address;
    newSymbol->nextSymbol = NULL;

    if (*head == NULL) {
	*head = newSymbol;
    } else {
	SymbolList *tmp = *head;
	while (tmp->nextSymbol != NULL)
	    tmp = tmp->nextSymbol;

	tmp->nextSymbol = newSymbol;
    }
}

SymbolList * getSymbolFromList(SymbolList *head, char *symbol)
{
    SymbolList *tmp = head;
    while (tmp != NULL) {
	if (strcmp(tmp->symbol, symbol) == 0)
	    break;
	tmp = tmp->nextSymbol;
    }
    return tmp;
}
