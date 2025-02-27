NAME = program
CC = gcc
CFLAGS = #-Werror -Wall -Wextra -g
LDFLAGS = -lwebsockets -lssl -lcrypto -lcurl -ljansson
RM = rm -rf
SCRIPT_DIR = scripts

INCLUDES = -I./include
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/websocket.c \
       $(SRC_DIR)/json_handler.c \
       $(SRC_DIR)/symbol_manager.c \
       $(SRC_DIR)/rb.c \
       $(SRC_DIR)/queue.c
#$(SRC_DIR)/err_log.c \

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all -v

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
	@for script in $(SCRIPT_DIR)/*.sh; do \
		echo "Running $$script..."; \
		bash $$script; \
	done

valgrind:
	  valgrind $(VALGRIND_FLAGS) ./server

.PHONY	:	all clean fclean re valgrind run_sh
