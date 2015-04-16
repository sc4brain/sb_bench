CC = gcc
DEFINE=
INCLUDE=
CFLAGS=-g -Wall
LDFLAGS=-lglut -lGLU -lGL -lm

OBJS=sb.o glm.o tb.o trackball.o
TARGET=sb obj

all: $(TARGET)

sb: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.o *~ $(TARGET)

obj:
	tar xvzfm obj.tar.gz
