NAME = program
CC = gcc
CFLAGS = -g #-Werror -Wall -Wextra -g
LDFLAGS = -lwebsockets -lssl -lcrypto -lcurl -ljansson -luuid -lncurses -lpthread
RM = rm -rf
SCRIPT_DIR = scripts

INCLUDES = -I./include
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/thread_queue.c \
       $(SRC_DIR)/log.c \
       $(SRC_DIR)/transaction.c \
       $(SRC_DIR)/websocket.c \
       $(SRC_DIR)/order_handler.c \
       $(SRC_DIR)/account_handler.c \
       $(SRC_DIR)/json_handler.c \
       $(SRC_DIR)/rest_api.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/symbol_handler.c \
       $(SRC_DIR)/curl_pool.c \
       $(SRC_DIR)/sig_handler.c \
       $(SRC_DIR)/ui.c \
       $(SRC_DIR)/rb.c \
       $(SRC_DIR)/queue.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

VALGRIND_FLAGS = --track-origins=yes --leak-check=full --show-leak-kinds=all -v

all		: $(NAME)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(NAME) $(OBJS) $(LDFLAGS)

clean	:
	$(RM) $(OBJ_DIR)/*

fclean	: clean
	$(RM) $(OBJ_DIR) $(NAME)

re		: fclean all

run_sh :
	@echo "Running watchdog..."
	@bash $(SCRIPT_DIR)/watchdog.sh || true
#	@for script in $(SCRIPT_DIR)/*.sh; do \
#		echo "Running $$script..."; \
#		bash $$script; \
#	done

valgrind:
	  valgrind $(VALGRIND_FLAGS) ./program

.PHONY	:	all clean fclean re valgrind run_sh
