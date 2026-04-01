
CC = gcc
CFLAGS = -Wall -O3
LIBS = -lm -lpthread


EXEC_SEQ = sequencial
EXEC_PAR = paralela
EXEC_OTI = otimizada


SRC_SEQ = sequencial.c
SRC_PAR = paralela.c
SRC_OTI = otimizada.c


all: $(EXEC_SEQ) $(EXEC_PAR) $(EXEC_OTI)


$(EXEC_SEQ): $(SRC_SEQ)
	$(CC) $(CFLAGS) $(SRC_SEQ) -o $(EXEC_SEQ) $(LIBS)

$(EXEC_PAR): $(SRC_PAR)
	$(CC) $(CFLAGS) $(SRC_PAR) -o $(EXEC_PAR) $(LIBS)

$(EXEC_OTI): $(SRC_OTI)
	$(CC) $(CFLAGS) $(SRC_OTI) -o $(EXEC_OTI) $(LIBS)


clean:
	rm -f $(EXEC_SEQ) $(EXEC_PAR) $(EXEC_OTI)


run: $(EXEC_OTI)
	./$(EXEC_OTI) 4 sensores.log
