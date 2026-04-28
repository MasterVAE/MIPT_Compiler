SOURCES_FRONTEND = frontend/main.cpp frontend/read.cpp frontend/tokenizator.cpp verificator.cpp\
						tree.cpp op_data.cpp tree_logger.cpp library.cpp
TARGET_FRONTEND = $(TARGET_DIR)/frontend.out

OBJECTS_FRONTEND = $(addprefix $(OBJ_DIR)/, $(SOURCES_FRONTEND:.cpp=.o))

$(TARGET_FRONTEND): $(OBJECTS_FRONTEND) | $(TARGET_DIR)
	@$(CC) $(CFLAGS) $^ -o $@
	@echo "LINKED FRONTEND"

frontend_b: $(TARGET_FRONTEND)

frontend: $(TARGET_FRONTEND)
	@./$(TARGET_FRONTEND) files/prog.upl files/tree.tr