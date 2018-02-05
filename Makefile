all: openWatch.so openTest

openWatch.so: openWatch.c
	gcc -shared -fPIC  $< -o $@ -ldl -Wall

openTest: openTest.cpp
	g++ $< -o $@ 
	
tests:	test-creat test-open
	
test-creat: openWatch.so openTest
	./test-creat.sh

test-open: openWatch.so openTest
	./test-open.sh

clean: 
	rm openWatch.so
