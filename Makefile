demo: jittest
	./jittest

jittest: jittest.cc
	g++ $< -o $@ -g
