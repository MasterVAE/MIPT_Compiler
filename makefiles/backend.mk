SOURCES_BACKEND = backend/main.cpp backend/compilator.cpp tree.cpp op_data.cpp tree_logger.cpp\
						library.cpp backend/scope.cpp
TARGET_BACKEND = $(TARGET_DIR)/backend.out

OBJECTS_BACKEND = $(addprefix $(OBJ_DIR)/, $(SOURCES_BACKEND:.cpp=.o))

$(TARGET_BACKEND): $(OBJECTS_BACKEND) | $(TARGET_DIR)
	@$(CC) $(CFLAGS) $^ -o $@
	@echo "LINKED BACKEND"

backend_b: $(TARGET_BACKEND)

backend: $(TARGET_BACKEND)
	@./$(TARGET_BACKEND) files/tree.tr files/code.s