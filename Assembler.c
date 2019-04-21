#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "SymbolTable.c"
#include <ctype.h>

typedef enum {A_COMMAND, C_COMMAND, L_COMMAND} Command;

int symbolParser(const char* asmFileName,const char* hackFileName);
int hasMoreCommands(FILE *asmFile);
Command nextCommand(char **cmdStr, unsigned int cmdStrSize, FILE *asmFile);
void removeWhitespaceAndComments(FILE *asmFILE);
void readSymbol(char *cmdStr, char **cmdStrBinary);
void readComp(char *cmdStr, char **compStr, int isDest);
void readJump(char *cmdStr, char **jumpStr);
void readDest(char *cmdStr, char **destStr);
void translateDest(char* dest, char **translatedDest);
void translateComp(char* comp, char **translatedComp); 
void translateJump(char* jump, char **translatedJump);



SymbolTable *symTable;
int ROMCounter = 0;
int RAMCounter = 16;
int linenr = 0;


/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
	tmp_value = value;
	value /= base;
	*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
	tmp_char = *ptr;
	*ptr--= *ptr1;
	*ptr1++ = tmp_char;
    }
    return result;
}

int hasMoreCommands(FILE *asmFile) {
    removeWhitespaceAndComments(asmFile);
    char c = getc(asmFile);
    if (c == EOF) {
	return 0;
    } else {
	ungetc(c, asmFile);
	return 1;
    }
}

/*
 * Reads the next command into cmdStr and returns the command type.
 */
Command nextCommand(char **cmdStr, unsigned int cmdStrSize, FILE *asmFile)
{
    int c;
    c = getc(asmFile);
    if (c == '@') {
	// A command @xxx 
	int count = 0;
	while ((c = getc(asmFile)) != EOF && (count < cmdStrSize) && (isdigit(c) || isalpha(c) || c == '_' || c == '.' || c ==  '$' || c == ':')) {
	    (*cmdStr)[count] = c;
	    count++;
	}
	(*cmdStr)[count] = '\0';
	ungetc(c, asmFile);
	return A_COMMAND;
    } else if (c == '(') {
	// L command (xxx) 
	int count = 0;
	while ((c = getc(asmFile)) != EOF && (count < cmdStrSize) && (isdigit(c) || isalpha(c) || c == '_' || c == '.' || c ==  '$' || c == ':')) {
	    if (count == 0 && isdigit(c)) {
		printf("Syntax Error: L command begins with digit. \n");
		exit(EXIT_FAILURE);
	    }
	    if (count == cmdStrSize) {
		printf("Syntax Error: Command too long.\n");
		exit(EXIT_FAILURE);
	    }
	    (*cmdStr)[count] = c;
	    count++;
	}
	if (c != ')') {
	    printf("Syntax Error: L command mismatched parenthesis\n");
	    exit(EXIT_FAILURE);
	}
	(*cmdStr)[count] = '\0';
	return L_COMMAND;
    } else {
	ungetc(c, asmFile);
        // C command
	// dest=comp;jump
	// if dest is empty, '=' is omitted
	// if jump is empty ';' is omitted
	int count = 0;
	while ((c = getc(asmFile)) == 'M' || c == 'D' || c == 'A' || c == '!' || c == '+' || c == '-'
	          || c == '1' || c == '0' || c == '!' || c == '|' || c == '&' || c == '=' || c == ';'
	          || c == ' ' || c == '\t' || c == '\v' || c ==  '\f' || c == 'J' || c == 'G' || c == 'T' || c == 'E' || c ==  'Q'
	          || c == 'L' || c == 'P' || c == 'N') {
	    if (c == ' ' || c == '\t' || c == '\v' || c ==  '\f') {
		continue;
	    }
	    if (count == cmdStrSize) {
		printf("Syntax Error: C Command too long");
		exit(EXIT_FAILURE);
	    }
	    (*cmdStr)[count] = c;
	    count++;
	}
	ungetc(c, asmFile);
	(*cmdStr)[count] = '\0';
	return C_COMMAND;
    }
}

/*
 * Reads the dest part of cmdStr into destStr 
 * dest=comp;jump
 */
void readDest(char *cmdStr, char **destStr)
{
    // dest=comp;jump
    int isDest = 0;
    int i;
    // dest is maximum 3 chars so atleast the fourth char must be '='
    for (i = 0; i < 4; i++) {
	if (cmdStr[i] == '=') {
	    isDest = 1;
	    break;
	}
    }
    if (isDest) {
	for (int j = 0; j < i; j++)  {
	    (*destStr)[j] = cmdStr[j];
	}
	(*destStr)[i] = '\0';
    } else {
	(*destStr)[0] = '\0';
    }
}

/*
 * Reads the comp part of dest=comp;jump
 * either the c command is dest=comp or comp;jump
 */
void readComp(char *cmdStr, char **compStr, int isDest)
{
    // dest must've been read before comp
    if (isDest) {
	// dest=comp
	int i = 0;
	while (cmdStr[i] != '=') i++;
	i++;
	int c = 0;
	// 
	while (cmdStr[i] != '\0' && c < 3) {
	    (*compStr)[c] = cmdStr[i];
	    c++;
	    i++;
	}
	(*compStr)[c] = '\0';
    } else {
	// comp;jump
	int c = 0;
	while (cmdStr[c] != ';') {
	    if (c == 3) {
		printf("Syntax Error: Invalid compStr: %s\n", *compStr);
		exit(EXIT_FAILURE);
	    }
	    (*compStr)[c] = cmdStr[c];
	    c++;
	}
	(*compStr)[c] = '\0';
    }
}

/*
 * Reads the jump part of dest=comp;jump command string.
 */
void readJump(char *cmdStr, char **jumpStr)
{
    int i = 0;
    while (cmdStr[i] != ';' && cmdStr[i] != '\0') ++i;

    // Not a jump
    if (cmdStr[i] == '\0') {
	(*jumpStr)[0] = '\0';
	return;
    }
    
    // advance i to first JUMP char
    ++i;
    int c = 0;
    while (cmdStr[i] != '\0') {
	if (c == 3) {
	    printf("Syntax Error: Invalid jumpStr\n");
	    exit(EXIT_FAILURE);
	}
	(*jumpStr)[c] = cmdStr[i];
	c++;
	i++;
    }
    (*jumpStr)[c] = '\0';
}

void readSymbol(char* cmdStr, char** cmdStrBinary)
{
    // 16 bits address
    int adr = (int) strtol(cmdStr, NULL, 10);
    // support 16 bit
    // convert the base 10 adr to a binary string
    // cmdStrBinary[15] = adr >> 15 (Tror jag...)
    int shift = 15;
    char *test = *cmdStrBinary;
    for (int i = 0; i < 16; i++) {
	int c = 1 & (adr >> shift);
	if (c == 0)
	    (*cmdStrBinary)[i] = '0';
	else
	    (*cmdStrBinary)[i] = '1';
	shift--;
    }
    (*cmdStrBinary)[0] = '0'; // A instruction
    (*cmdStrBinary)[16] = '\0';
}


/*
 * Parses an asm file into binary HACK instructions.
 * Important to first call readLabels before. 
 */
int symbolParser(const char* asmFileName, const char* hackFileName)
{
    FILE *asmFile = fopen(asmFileName, "r");
    FILE *hackFile = fopen(hackFileName, "w");
    // Since the first pass readLabels parsed L_COMMANDs we can now ignore them.
    if (asmFile) {
	while (hasMoreCommands(asmFile)) {
	    char *cmdStr = malloc(sizeof(char)*128);
	    Command cmd = nextCommand(&cmdStr, 128, asmFile);
	    if (cmd == A_COMMAND) {
		// either the symbol is @decimal, or @xxx, xxx could be a (label), existing variable or a new variable.
		// check if @decimal, variabels and labels are not allowed to start with a digit
		if (isdigit(*cmdStr)) {
		    char *sym = malloc(sizeof(char)*17);
		    readSymbol(cmdStr, &sym);	        
		    fprintf(hackFile, "%s\n", sym);	   
		    free(sym);
		} else if (containsSymbol(symTable, cmdStr)) {
		    // existing variable or label
		    int address = getAddress(symTable, cmdStr);
		    char *sym10 = malloc(sizeof(char)*17);
		    sym10 = itoa(address, sym10, 10);
		    char *sym2 = malloc(sizeof(char)*17);
		    readSymbol(sym10, &sym2);
		    fprintf(hackFile, "%s\n", sym2);
		    free(sym2);
		    free(sym10);		    
		} else {
		    // new variable
		    // RAM counter is in base 10
		    char *sym10 = malloc(sizeof(char)*17);
		    sym10 = itoa(RAMCounter, sym10, 10);
		    char *sym2 = malloc(sizeof(char)*17);
		    readSymbol(sym10, &sym2);
		    fprintf(hackFile, "%s\n", sym2);
		    // Add to symbolTable
		    addSymbol(symTable, cmdStr, RAMCounter);
		    free(sym2);
		    free(sym10);
		    RAMCounter++;
		}
	    } else if (cmd == C_COMMAND) {
		// C Command, dest=comp;jump
		char *destStr = malloc(sizeof(char)*4);
		char *compStr = malloc(sizeof(char)*4);
		char *jumpStr = malloc(sizeof(char)*4);
		char *destInstr = malloc(sizeof(char)*4);
		char *jumpInstr = malloc(sizeof(char)*4);
		char *compInstr = malloc(sizeof(char)*8);
		readDest(cmdStr, &destStr);
		int isDest = 0;
		if (*destStr != '\0') {
		    isDest = 1;
		}
		readComp(cmdStr, &compStr, isDest);
		readJump(cmdStr, &jumpStr);
		translateDest(destStr, &destInstr);
		translateComp(compStr, &compInstr);
		translateJump(jumpStr, &jumpInstr);
		// Build c instruction
		char instruction[17];
		instruction[0] = '1';
		// [1..2] = 1, not used
		instruction[1] = '1';
		instruction[2] = '1';
		for (int i = 3; i < 10; i++) instruction[i] = compInstr[i-3];		
		for (int i = 10; i < 13; i++) instruction[i] = destInstr[i-10];
		for (int i = 13; i < 16; i++) instruction[i] = jumpInstr[i-13];
		instruction[16] = '\0';
		fprintf(hackFile, "%s\n", instruction);
		// cleanup
		free(compStr);
		free(jumpStr);
		free(destStr);
		free(compInstr);
		free(jumpInstr);
		free(destInstr);
	    }
	    free(cmdStr);
	}
	fclose(asmFile);
	fclose(hackFile);
    }
}

/*
 * Recursively removes whitespace and comments untill next non-whitespace / comment character.
 */
void removeWhitespaceAndComments(FILE *asmFile)
{
    char c;
    // Remove all whitespace and newline
    while (isspace(c = getc(asmFile))) {
	if (c == '\n')
	    linenr++;
    }
    // check if comment
    if (c == '/') {
	if((c = getc(asmFile)) == '/') {
	    // if comment read untill newline
	    while((c = getc(asmFile)) != '\n') {
		if (c == EOF) {
		    ungetc(c, asmFile);
		    break;
		}
	    }
	    linenr++;
	    // Keep scanning line following comment for whitespace || comments
	    removeWhitespaceAndComments(asmFile);
	} else {
	    ungetc('/', asmFile);
	    ungetc(c, asmFile);
	} 
    } else {
	ungetc(c, asmFile);
    }
}

/*
 * Scans asm source file for (xxx) L_COMMMANDS and adds them to the symbol table.
 * The label gets the free ROM slot (ROMCounter keeps track).
 */
void parseLabels(const char *asmFileName)
{
    int c;
    FILE *asmFile = fopen(asmFileName, "r");
    
    if (asmFile) {
	while (hasMoreCommands(asmFile)) {
	    char *cmdStr = malloc(sizeof(char)*128);
	    Command cmd = nextCommand(&cmdStr, 128, asmFile);
	    if (cmd == A_COMMAND) {
		ROMCounter++;
	    } else if (cmd == L_COMMAND) {
		if (!containsSymbol(symTable, cmdStr)) {
		    addSymbol(symTable, cmdStr, ROMCounter);
		} else {
		    printf("Syntax Error: Label \"%s\" already declared\n", cmdStr);
		    exit(EXIT_FAILURE);
		}
	    } else if (cmd == C_COMMAND) {
		ROMCounter++;
	    } else {
		exit(EXIT_FAILURE);
	    }
	    free(cmdStr);
	}
	fclose(asmFile);
    }
}

/////////////////
/* CODE module */
/////////////////


// Translates the dest mnemonic into 3 bits
void translateDest(char* dest, char **translatedDest)
{
    // strcmp(s1,s2) = 0 then s1 == s2
    if (!strcmp(dest, ""))
	strcpy(*translatedDest, "000");
    else if (!strcmp(dest, "M"))
	strcpy(*translatedDest, "001");
    else if (!strcmp(dest, "D"))
	strcpy(*translatedDest, "010");
    else if (!strcmp(dest, "MD"))
	strcpy(*translatedDest, "011");
    else if (!strcmp(dest, "A"))
	strcpy(*translatedDest, "100");
    else if (!strcmp(dest, "AM"))
	strcpy(*translatedDest, "101");
    else if (!strcmp(dest, "AD"))
	strcpy(*translatedDest, "110");
    else if (!strcmp(dest, "AMD"))
	strcpy(*translatedDest, "111");
    else {
	printf("Syntax Error - incorrect dest: %s\n", dest);
	exit(EXIT_FAILURE);
    }
}

// Translates the comp mnemonic into 7 bits
void translateComp(char* comp, char **translatedComp)
{
    // a = 0 -> first bit 0
    // could use a static array for matching + loop, but easier to spot mistakes with this..
    if (!strcmp(comp, "0"))
	strcpy(*translatedComp, "0101010");
    else if (!strcmp(comp, "1"))
	strcpy(*translatedComp, "0111111");
    else if (!strcmp(comp, "-1"))
	strcpy(*translatedComp, "0111010");
    else if (!strcmp(comp, "D"))
	strcpy(*translatedComp, "0001100");
    else if (!strcmp(comp, "A"))
	strcpy(*translatedComp, "0110000");
    else if (!strcmp(comp, "!D"))
	strcpy(*translatedComp, "0001101");
    else if (!strcmp(comp, "!A"))
	strcpy(*translatedComp, "0110001");
    else if (!strcmp(comp, "-D"))
	strcpy(*translatedComp, "0001111");
    else if (!strcmp(comp, "-A"))
	strcpy(*translatedComp, "0110011");
    else if (!strcmp(comp, "D+1"))
	strcpy(*translatedComp, "0011111");
    else if (!strcmp(comp, "A+1"))
	strcpy(*translatedComp, "0110111");
    else if (!strcmp(comp, "D-1"))
	strcpy(*translatedComp, "0001110");
    else if (!strcmp(comp, "A-1"))
	strcpy(*translatedComp, "0110010");
    else if (!strcmp(comp, "D+A"))
	strcpy(*translatedComp, "0000010");
    else if (!strcmp(comp, "D-A"))
	strcpy(*translatedComp, "0010011");
    else if (!strcmp(comp, "A-D"))
	strcpy(*translatedComp, "0000111");
    else if (!strcmp(comp, "D&A"))
	strcpy(*translatedComp, "0000000");
    else if (!strcmp(comp, "D|A"))
	strcpy(*translatedComp, "0010101");
    // a = 1 -> first bit 1
    else if (!strcmp(comp, "M"))
	strcpy(*translatedComp, "1110000");
    else if (!strcmp(comp, "!M"))
	strcpy(*translatedComp, "1110001");
    else if (!strcmp(comp, "-M"))
	strcpy(*translatedComp, "1110011");
    else if (!strcmp(comp, "M+1"))
	strcpy(*translatedComp, "1110111");
    else if (!strcmp(comp, "M-1"))
	strcpy(*translatedComp, "1110010");
    else if (!strcmp(comp, "D+M"))
	strcpy(*translatedComp, "1000010");
    else if (!strcmp(comp, "D-M"))
	strcpy(*translatedComp, "1010011");
    else if (!strcmp(comp, "M-D"))
	strcpy(*translatedComp, "1000111");
    else if (!strcmp(comp, "D&M"))
	strcpy(*translatedComp, "1000000");
    else if (!strcmp(comp, "D|M"))
	strcpy(*translatedComp, "1010101");
    else {
	printf("Syntax Error - incorrect comp: %s\n", comp);
	exit(EXIT_FAILURE);
    }
}

// Translates the jump mnemonic into ... bits
void translateJump(char* jump, char **translatedJump)
{
    // strcmp(s1,s2) = 0 then s1 == s2
    if (!strcmp(jump, ""))
	strcpy(*translatedJump, "000");
    else if (!strcmp(jump, "JGT"))
	strcpy(*translatedJump, "001");
    else if (!strcmp(jump, "JEQ"))
	strcpy(*translatedJump, "010");
    else if (!strcmp(jump, "JGE"))
	strcpy(*translatedJump, "011");
    else if (!strcmp(jump, "JLT"))
	strcpy(*translatedJump, "100");
    else if (!strcmp(jump, "JNE"))
	strcpy(*translatedJump, "101");
    else if (!strcmp(jump, "JLE"))
	strcpy(*translatedJump, "110");
    else if (!strcmp(jump, "JMP"))
	strcpy(*translatedJump, "111");
    else {
	printf("Syntax Error - incorrect jump: %s\n", jump);
	exit(EXIT_FAILURE);
    }
}

void setReservedRamMemory(void)
{
    addSymbol(symTable, "SP", 0);
    addSymbol(symTable, "LCL", 1);
    addSymbol(symTable, "ARG", 2);
    addSymbol(symTable, "THIS", 3);
    addSymbol(symTable, "THAT", 4);
    addSymbol(symTable, "R0", 0);
    addSymbol(symTable, "R1", 1);
    addSymbol(symTable, "R2", 2);
    addSymbol(symTable, "R3", 3);
    addSymbol(symTable, "R4", 4);
    addSymbol(symTable, "R5", 5);
    addSymbol(symTable, "R6", 6);
    addSymbol(symTable, "R7", 7);
    addSymbol(symTable, "R8", 8);
    addSymbol(symTable, "R9", 9);
    addSymbol(symTable, "R10", 10);
    addSymbol(symTable, "R11", 11);
    addSymbol(symTable, "R12", 12);
    addSymbol(symTable, "R13", 13);
    addSymbol(symTable, "R14", 14);
    addSymbol(symTable, "R15", 15);
    addSymbol(symTable, "SCREEN", 16384);
    addSymbol(symTable, "KBD", 24576);
}

int main(int argc, const char* argv[])
{
    if (argc != 2) {
	printf("Incorrect number of arguments\n");
	return EXIT_FAILURE;
    }
    symTable = malloc(sizeof(SymbolTable));
    symTable->capacity = 1000;
    symTable->count = 0;
    symTable->table = malloc(sizeof(symTable->table) * symTable->capacity);
    
    // Construct .hack filename
    char asmfilename[128];
    const char *filenamepointer = argv[1];
    int cf = 0;
    while (*filenamepointer != '.') {
	asmfilename[cf] = *filenamepointer;
	++filenamepointer;
	++cf;
	if (cf == 128) {
	    printf("Name %s too long", argv[1]);
	    return EXIT_FAILURE;
	}
    }
    asmfilename[cf] = '\0';
    char* hackextension = ".hack";
    char hackfilename[128 + 6];
    strncpy(hackfilename, asmfilename, sizeof(hackfilename));
    strncat(hackfilename, hackextension, (sizeof(hackfilename) + strlen(hackextension) + 1));

    // First pass store ROM (read only memory) address of all (xxx) labels.
    setReservedRamMemory();
    parseLabels(argv[1]);
    //  printSymbolTable(symTable);
    // No symbols parser
    linenr=0;
    symbolParser(argv[1], hackfilename);
    // printSymbolTable(symTable);
    freeSymbolTable(&symTable);
}
