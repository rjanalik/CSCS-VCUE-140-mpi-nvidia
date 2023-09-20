CC=cc

test-mpi: test-mpi.c
	$(CC) $? -o $@

clean:
	rm -f test-mpi
