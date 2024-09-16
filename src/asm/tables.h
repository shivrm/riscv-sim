#define TABLES_H

typedef struct RegTableEntry {
	char *key;
	int value;
} RegTableEntry;

typedef struct RInsTableEntry {
	char *key;
	int opcode, funct3, funct7;	 
} RInsTableEntry;

typedef struct IInsTableEntry {
	char *key;
	int opcode, funct3;	 
} IInsTableEntry;

typedef struct SInsTableEntry {
    char *key;
    int opcode, funct3;
} SInsTableEntry;

typedef struct BInsTableEntry {
    char *key;
    int opcode, funct3;
} BInsTableEntry;

typedef struct UInsTableEntry {
    char *key;
    int opcode;
} UInsTableEntry;

typedef struct JInsTableEntry {
    char *key;
    int opcode;
} JInsTableEntry;