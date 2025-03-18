#include "transaction.h"

static FILE *txn_file = NULL;
static pthread_mutex_t txn_mutex = PTHREAD_MUTEX_INITIALIZER;

void rotate_txn_file()
{
	pthread_mutex_lock(&txn_mutex);

    struct stat st;
    if (stat("./transactions/txn_1.csv", &st) == 0 && st.st_size > MAX_TRANSACTION_FILES) {
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
			pr_err("Failed to create new transaction file.");
			pthread_mutex_unlock(&txn_mutex);
			exit(EXIT_FAILURE);
		}
	}
	pthread_mutex_unlock(&txn_mutex);
}

void save_transaction(const char *date, const char *time, const char *code,
					  const char *side, double price, double volume)
{
	pthread_mutex_lock(&txn_mutex);

    rotate_txn_file();

    if (!txn_file) {
		txn_file = fopen("./transactions/txn_1.csv", "a");
		if (!txn_file) {
			pr_err("Failed to open transaction file");
			pthread_mutex_unlock(&txn_mutex);
			return;
		}
    }

    fprintf(txn_file, "%s,%s,%s,%s,%.2f,%.6f\n", date, time, code, side,
			price, volume);
    fflush(txn_file);			// send the buffer to kernel
    fsync(fileno(txn_file));	// force to store kernel buffer on the disk

	pthread_mutex_unlock(&txn_mutex);
}

void save_txn_task(void *arg)
{
	pthread_mutex_lock(&txn_mutex);

	Transaction *txn = (Transaction *)arg;

    rotate_txn_file();

    if (!txn_file) {
        txn_file = fopen("./transactions/txn_1.csv", "a");
		if (!txn_file) {
			pr_err("Failed to open transaction file");
			pthread_mutex_unlock(&txn_mutex);
			return;
		}
    }

    fprintf(txn_file, "%s,%s,%s,%s,%.2f,%.6f\n", txn->date, txn->time,
			txn->code, txn->side, txn->price, txn->volume);
    fflush(txn_file);			// send the buffer to kernel
    fsync(fileno(txn_file));	// force to store kernel buffer on the disk

	pthread_mutex_unlock(&txn_mutex);

	free(arg);
}

void terminate_txn()
{
	pthread_mutex_lock(&txn_mutex);
	if (txn_file) { 
		fclose(txn_file);
		txn_file = NULL;
	}
	pthread_mutex_unlock(&txn_mutex);
	pthread_mutex_destroy(&txn_mutex);
}
