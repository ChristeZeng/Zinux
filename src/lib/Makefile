all : printk.o rand.o string.o

printk.o : printk.c
	$(GCC) $(CFLAG) -o printk.o -c printk.c

rand.o : rand.c
	$(GCC) $(CFLAG) -o rand.o -c rand.c

string.o : string.c
	$(GCC) $(CFLAG) -o string.o -c string.c

clean :
	$(shell rm *.o 2>/dev/null)