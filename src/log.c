#include "log.h"

static FILE *log_file = NULL;
static int log_index = 1;

void log_emit(int level, const char *msg)
{
	if (log_file) {
		fprintf(log_file, "%s", msg);
		fflush(log_file);
	} else {
		fprintf(stderr, "%s", msg);
	}
}

void init_logging()
{
	log_file = fopen("./log/log_file_1.txt", "w");
	if (!log_file) {
		fprintf(stderr, "Failed to open log_file\n");
		exit(EXIT_FAILURE);
	}
	lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, log_emit);
}

void terminate_logging()
{
	fclose(log_file);
}

void log_err(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	rotate_log_file();

	vfprintf(log_file, fmt, args);
	fputc('\n', log_file);
	fflush(log_file);

	va_end(args);
}

void rotate_log_file()
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
			fprintf(stderr, "Failed to create new log file\n");
			exit(EXIT_FAILURE);
		}
	}
}
