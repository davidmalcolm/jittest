demo: jittest
	./jittest

SOURCE_FILES:=stackvm.cc regvm.cc main.cc
OBJECT_FILES:=stackvm.o regvm.o main.o

$(OBJECT_FILES): %.o: %.cc stackvm.h regvm.h
	g++ -c -o $@ -g $< -Wall

jittest: $(OBJECT_FILES)
	g++ -o $@ $(OBJECT_FILES) -lgccjit

clean:
	rm -f *.o jittest
