#include "transaction.h"

static FILE *txn_file = NULL;
static pthread_mutex_t txn_mutex;

void init_txn()
{
	if (pthread_mutex_init(&txn_mutex, NULL) != 0) {
        pr_err("pthread_mutex_init() failed.");
        exit(EXIT_FAILURE);
    }
	txn_file = fopen("./transactions/txn_1.csv", "a");
	if (!txn_file) {
		pr_err("fopen() failed.");
		exit(EXIT_FAILURE);
	}
	struct stat st;
    if (stat("./transactions/txn_1.csv", &st) == 0 && st.st_size == 0) {
        fprintf(txn_file,
				"trade_timestamp,datetime,code,side,ord_type,maker_taker,state,"
				"price,avg_price,volume,executed_volume,executed_funds,trade_fee,"
				"total,uuid,trade_uuid\n");
        fflush(txn_file);
    }
}

void destroy_txn()
{
	pthread_mutex_lock(&txn_mutex);
	if (txn_file) { 
		fclose(txn_file);
		txn_file = NULL;
	}
	pthread_mutex_unlock(&txn_mutex);
	pthread_mutex_destroy(&txn_mutex);
}

transaction_t *create_txn(long trade_timestamp, const char *code, const char *side,
						  const char *ord_type, bool is_maker, const char *state,
						  double price, double avg_price, double volume,
						  double executed_volume, double executed_funds,
						  double trade_fee, const char *uuid, const char *trade_uuid)
{
	transaction_t *txn;
	MALLOC(txn, sizeof(transaction_t));
	txn->trade_timestamp = trade_timestamp;
	strcpy(txn->code, code);
	strcpy(txn->side, side);
	strcpy(txn->ord_type, ord_type);
	strcpy(txn->maker_taker, is_maker ? "maker" : "taker"); // true: maker order, false: taker order
	strcpy(txn->state, state);
	txn->price = price;
	txn->avg_price = avg_price;
	txn->volume = volume;
	txn->executed_volume = executed_volume;
	txn->executed_funds = executed_funds;
	txn->trade_fee = trade_fee;
	if (strcmp(txn->side, "ASK") == 0) txn->total = executed_funds - trade_fee;
	else txn->total = executed_funds + trade_fee;
	strcpy(txn->uuid, uuid);
	strcpy(txn->trade_uuid, trade_uuid);

	return txn;
}

void rotate_txn_file()
{
    struct stat st;
    if (stat("./transactions/txn_1.csv", &st) == 0 && st.st_size > MAX_FILE_SIZE) {
        fclose(txn_file);

        char oldest_file[64];
        snprintf(oldest_file, sizeof(oldest_file), "./transactions/txn_%d.csv", MAX_TRANSACTION_FILES);
        remove(oldest_file);

        for (int i = MAX_TRANSACTION_FILES - 1; i >= 1; i--) {
            char old_name[64], new_name[64];
            snprintf(old_name, sizeof(old_name), "./transactions/txn_%d.csv", i);
            snprintf(new_name, sizeof(new_name), "./transactions/txn_%d.csv", i + 1);
            rename(old_name, new_name);
        }

        txn_file = fopen("./transactions/txn_1.csv", "w");
		if (!txn_file) {
			MY_LOG_ERR("fopen() failed.");
			pr_err("fopen() failed.");
			pthread_mutex_unlock(&txn_mutex);
			exit(EXIT_FAILURE);
		}
	}
}

void save_transaction(transaction_t *txn)
{
	pthread_mutex_lock(&txn_mutex);

    rotate_txn_file();

	struct tm *tm_info = localtime(&txn->trade_timestamp);
	char datetime[20];
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(txn_file,
			"%ld,%s,%s,%s,%s,%s,%s,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%s,%s\n",
			txn->trade_timestamp, datetime, txn->code, txn->side, txn->ord_type,
			txn->maker_taker, txn->state, txn->price, txn->avg_price,
			txn->volume, txn->executed_volume, txn->executed_funds,
			txn->trade_fee, txn->total, txn->uuid, txn->trade_uuid);

    fflush(txn_file); // send the buffer to kernel
    fsync(fileno(txn_file)); // force to store kernel buffer on the disk

	pthread_mutex_unlock(&txn_mutex);
}

void save_txn_task(void *arg)
{
	pthread_mutex_lock(&txn_mutex);

	transaction_t *txn = (transaction_t *)arg;

    rotate_txn_file();

	struct tm *tm_info = localtime(&txn->trade_timestamp);
	char datetime[20];
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(txn_file,
			"%ld,%s,%s,%s,%s,%s,%s,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%s,%s\n",
			txn->trade_timestamp, datetime, txn->code, txn->side, txn->ord_type,
			txn->maker_taker, txn->state, txn->price, txn->avg_price,
			txn->volume, txn->executed_volume, txn->executed_funds,
			txn->trade_fee, txn->total, txn->uuid, txn->trade_uuid);

    fflush(txn_file); // send the buffer to kernel
    fsync(fileno(txn_file)); // force to store kernel buffer on the disk

	pthread_mutex_unlock(&txn_mutex);

	FREE(arg);
}
