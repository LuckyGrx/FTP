CC = gcc -std=gnu99

#upload_test:upload_test.o
#	$(CC) -o upload_test upload_test.o -lpthread

#upload_test.o:upload_test.c
#	$(CC) -c upload_test.c



stress_test:stress_test.o
	$(CC) -o stress_test stress_test.o

stress_test.o:stress_test.c
	$(CC) -c stress_test.c

clean:
	rm *.o