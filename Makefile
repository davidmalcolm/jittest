# For now, fixup these paths as appropriate:
GCC_SRC_DIR=../gcc-git-jit/src
GCC_BUILD_DIR=../gcc-git-jit/build

demo: jittest
	LD_LIBRARY_PATH=$(GCC_BUILD_DIR) ./jittest

SOURCE_FILES:=stackvm.cc regvm.cc main.cc
OBJECT_FILES:=stackvm.o regvm.o main.o

$(OBJECT_FILES): %.o: %.cc stackvm.h regvm.h
	g++ -c -o $@ -g $< -Wall -I$(GCC_SRC_DIR)/gcc/jit/

jittest: $(OBJECT_FILES)
	g++ -o $@ $(OBJECT_FILES) $(GCC_BUILD_DIR)/gcc/libgccjit.so

clean:
	rm -f *.o jittest
