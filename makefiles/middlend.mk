SOURCES_MIDDLEND = middlend/main.cpp middlend/optimizator.cpp \
						tree.cpp op_data.cpp tree_logger.cpp library.cpp
TARGET_MIDDLEND = $(TARGET_DIR)/middlend.out

OBJECTS_MIDDLEND = $(addprefix $(OBJ_DIR)/, $(SOURCES_MIDDLEND:.cpp=.o))

$(TARGET_MIDDLEND): $(OBJECTS_MIDDLEND) | $(TARGET_DIR)
	@$(CC) $(CFLAGS) $^ -o $@
	@echo "LINKED MIDDLEND"

middlend_b: $(TARGET_MIDDLEND)

middlend: $(TARGET_MIDDLEND)
	@./$(TARGET_MIDDLEND) files/tree.tr