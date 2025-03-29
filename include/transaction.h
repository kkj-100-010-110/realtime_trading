#ifndef _TRANSACTION_H
#define _TRANSACTION_H

#include "common.h"

#define MAX_FILE_SIZE 10 * 1024 * 1024
#define MAX_TRANSACTION_FILES 5

typedef struct {
	char date[11];
	char time[9];
	char code[16];
	char side[4];
	double price;
	double volume;
} Transaction;

void rotate_txn_file();
void save_transaction(const char *date, const char *time, const char *market,
					  const char *side, double price, double volume);
void save_txn_task(void *arg);
void terminate_txn();

#endif//_TRANSACTION_H
