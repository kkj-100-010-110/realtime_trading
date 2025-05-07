#ifndef _TRANSACTION_H
#define _TRANSACTION_H

#include "common.h"

#define MAX_FILE_SIZE 10 * 1024 * 1024
#define MAX_TRANSACTION_FILES 5

typedef struct {
	long trade_timestamp;
	char code[16];
	char side[4];
	char ord_type[7];
	char maker_taker[6];
    char state[7];
	double price;
    double avg_price;
	double volume;
    double executed_volume;
    double executed_funds;
    double trade_fee;
    double total;
    char uuid[37];
    char trade_uuid[37];
} transaction_t;

void init_txn();
void destroy_txn();
transaction_t *create_txn(long trade_timestamp, const char *code, const char *side,
						  const char *ord_type, bool is_maker, const char *state,
						  double price, double avg_price, double volume,
						  double executed_volume, double executed_funds,
						  double trade_fee, const char *uuid, const char *trade_uuid);
void save_transaction(transaction_t *txn);
void save_txn_task(void *arg);

#endif//_TRANSACTION_H
