BUILD_DIR = build

libc-image-lib.a: $(BUILD_DIR)/images.o $(BUILD_DIR)/images-primitives.o
	ar rcs libc-image-lib.a $(BUILD_DIR)/images.o $(BUILD_DIR)/images-primitives.o

	
$(BUILD_DIR)/images.o: images.c
	gcc -c images.c -o $(BUILD_DIR)/images.o -lm
$(BUILD_DIR)/images-primitives.o: images-primitives.c
	gcc -c images-primitives.c -o $(BUILD_DIR)/images-primitives.o -lm