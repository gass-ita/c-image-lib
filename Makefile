BUILD_DIR = build

libc-image-lib.a: $(BUILD_DIR)/images.o $(BUILD_DIR)/images-primitives.o $(BUILD_DIR)/images-parser.o | $(BUILD_DIR)
	ar rcs libc-image-lib.a $(BUILD_DIR)/images.o $(BUILD_DIR)/images-primitives.o $(BUILD_DIR)/images-parser.o

	
$(BUILD_DIR)/images.o: images.c | $(BUILD_DIR)
	gcc -c images.c -o $(BUILD_DIR)/images.o -lm

$(BUILD_DIR)/images-primitives.o: images-primitives.c | $(BUILD_DIR)
	gcc -c images-primitives.c -o $(BUILD_DIR)/images-primitives.o -lm

$(BUILD_DIR)/images-parser.o: images-parser.c | $(BUILD_DIR)
	gcc -c images-parser.c -o $(BUILD_DIR)/images-parser.o -lm

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) libc-image-lib.a