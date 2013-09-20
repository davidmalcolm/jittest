demo: jittest
	./jittest

SOURCE_FILES:=stackvm.cc regvm.cc main.cc
OBJECT_FILES:=stackvm.o regvm.o main.o

# For now, fixup these paths as appropriate:
GCC_SRC_DIR=../gcc-git-various-branches/src
GCC_BUILD_DIR=../gcc-git-various-branches/build

$(OBJECT_FILES): %.o: %.cc stackvm.h regvm.h
	g++ -c -o $@ -g $< -Wall -I$(GCC_SRC_DIR)/gcc/jit/

jittest: $(OBJECT_FILES)
	g++ -o $@ $(OBJECT_FILES) $(GCC_BUILD_DIR)/gcc/libgccjit.so
