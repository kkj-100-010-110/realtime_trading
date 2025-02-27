#include "symbol_manager.h"

void load_symbols(char symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN], int *symbol_cnt)
{
	FILE *file = fopen("./config/symbols.txt", "r");
	if (!file) {
		pr_err("fopen() failed.");
		return;
	}

	*symbol_cnt = 0;
	while (*symbol_cnt < MAX_SYMBOLS
			&& fscanf(file, "%s", symbols[*symbol_cnt])) {
		*symbol_cnt += 1;
	}
	fclose(file);
}

char *create_subscription()
{
	char symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN];
	int symbol_cnt;

	load_symbols(symbols, &symbol_cnt);
	if (symbol_cnt == 0) {
		pr_err("No symbols");
		return NULL;
	}

	/* Json format */
	char *json_msg = (char *)malloc(sizeof(char) * 512);
	if (!json_msg) {
		pr_err("malloc() failed.");
		return NULL;
	}

	strcpy(json_msg, "[{\"ticket\":\"test\"},{\"type\":\"ticker\",\"codes\":[");

	for (int i = 0; i < symbol_cnt; i += 1) {
		strcat(json_msg, "\"");
		strcat(json_msg, symbols[i]);
		strcat(json_msg, "\"");
		if (i < symbol_cnt - 1)
			strcat(json_msg, ",");
	}
	strcat(json_msg, "]}]");

	return json_msg;
}
