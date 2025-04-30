#include "log.h"

/* static variable & functions begin */
static FILE *log_file = NULL;
static int log_index = 1;
static pthread_mutex_t log_mutex;

static void log_emit(int level, const char *msg)
{
	if (log_file) {
		fprintf(log_file, "%s", msg);
		fflush(log_file);
	} else {
		fprintf(stderr, "%s", msg);
	}
}

static void rotate_log_file()
{
	struct stat st;
	if (stat("./log/log_file_1.txt", &st) == 0 && st.st_size > MAX_FILE_SIZE) {
		fclose(log_file);

		char oldest_file[32];
		snprintf(oldest_file, sizeof(oldest_file), "./log/log_file_%d.txt", MAX_LOG_FILES);
		remove(oldest_file);

		for (int i = MAX_LOG_FILES - 1; i >= 1; i--) {
			char old_name[32], new_name[32];
			snprintf(old_name, sizeof(old_name), "./log/log_file_%d.txt", i);
			snprintf(new_name, sizeof(new_name), "./log/log_file_%d.txt", i + 1);
			rename(old_name, new_name);
		}

		log_file = fopen("./log/log_file_1.txt", "w");
		if (!log_file) {
			fprintf(stderr, "[ERR][%s:%d] fopen() failed.\n", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}
	}
}
/* static variable & functions end */


void init_log()
{
	if (pthread_mutex_init(&log_mutex, NULL) != 0) {
        fprintf(stderr, "[ERR][%s:%d] pthread_mutex_init() failed.\n", __FILE__,
				__LINE__);
        exit(EXIT_FAILURE);
    }
	log_file = fopen("./log/log_file_1.txt", "w");
	if (!log_file) {
		fprintf(stderr, "[ERR][%s:%d] fopen() failed.\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
	/* logging lws error */
	//lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, log_emit);
	lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG, log_emit);
}

void destroy_log()
{
	pthread_mutex_lock(&log_mutex);

	fclose(log_file);

	pthread_mutex_unlock(&log_mutex);

	pthread_mutex_destroy(&log_mutex);
}

void logging_task(void *arg)
{
    log_task_arg_t *lta = (log_task_arg_t *)arg;

	pthread_mutex_lock(&log_mutex);

	rotate_log_file();

	fprintf(log_file, "%s\n", lta->buffer);
	fflush(log_file);

	pthread_mutex_unlock(&log_mutex);

	free(lta->buffer);
    free(lta);
}

void do_log(const char *fmt, ...)
{
	log_task_arg_t *lta;
	lta = (log_task_arg_t *)malloc(sizeof(log_task_arg_t));
	if (!lta) {
		fprintf(log_file, "[ERR][%s:%d] malloc() failed.\n", __FILE__, __LINE__);
		return;
	}

	va_list args;
	va_start(args, fmt);

	if (vasprintf(&lta->buffer, fmt, args) == -1) {
		va_end(args);
		free(lta);
		fprintf(log_file, "[ERR][%s:%d] vasprintf() failed.\n", __FILE__, __LINE__);
		return;
	}

	va_end(args);

	if (!enqueue_task(logging_task, (void *)lta)) {
		free(lta->buffer);
		free(lta);
		fprintf(log_file, "[ERR][%s:%d] failed to push logging_task() to queue.\n",
				__FILE__, __LINE__);
		return;
	}
}
