# openedFiles2fifo
LD\_PRELOAD library to watch files opened by an application and send them to a FIFO, without ever blocking

# Build 

Compile with "make" 

# Usage 

By default, it will use the FIFO /tmp/open\_watch\_fifo.  You need to create it in advance with: 

$ mkfifo  /tmp/open\_watch\_fifo.

Then simply export LD\_PRELOAD=/.../openWatch.so. 

Note that the path needs to be absolute unless you copy the library to /usr/lib.  In this case it is enough with: 

export LD\_PRELOAD=openWatch.so.

In one terminal you can do: `cat /tmp/open_watch_fifo`. 

Then go to the previous terminal and simply execute your app. 

You will see the names of the opened files appear. 

If you need some event-driven handling (my initial motivation), you can use something like: 

```
while read filename < /tmp/open_watch_fifo
do
    echo $filename has been opened
done
```

Credits 

Based on this code: 
https://rafalcieslak.wordpress.com/2013/04/02/dynamic-linker-tricks-using-ld_preload-to-cheat-inject-features-and-investigate-programs/

I added proper support to open()'s varargs and robust writing to a FIFO.


