SRCDIR = .
INCDIR = .
BINDIR = .
OBJDIR = .

CC = g++
CFLAGS		= -I${INCDIR} -Wall
LDFLAGS		= #-lpthread -pthread

TARGET = monitor
BIN_TARGET = ${BINDIR}/${TARGET}

SRC = $(wildcard ${SRCDIR}/*.cpp)
OBJ = $(patsubst %.cpp,${OBJDIR}/%.o,$(notdir ${SRC}))

${BIN_TARGET}:${OBJ}
	$(CC) $(OBJ) $(LDFLAGS)  -o $@

${OBJDIR}/%.o:${SRCDIR}/%.cpp
	$(CC) $(CFLAGS) -c  $< -o $@

.PHONY:clean
clean:
	rm $(OBJDIR)/*.o $(BINDIR)/${TARGET} $
