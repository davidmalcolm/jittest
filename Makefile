demo: jittest
	./jittest

jittest: stackvm.cc stackvm.h
	g++ $< -o $@ -g
