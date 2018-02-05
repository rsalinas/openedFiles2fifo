test: openWatch.so
	LD_PRELOAD=$(PWD)/$< cat /proc/uptime /proc/uptime

openWatch.so: openWatch.c
	gcc -shared -fPIC  $< -o $@ -ldl -Wall

clean: 
	rm openWatch.so
