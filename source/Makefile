# 4T - Terminal Task Time Tracker
# 29 Jul 2025
# makefile

objs = main.o cxa.o
flags = -Wall -Wextra -Wpedantic
name = 4T

all: $(name)

$(name): $(objs)
	cc	-o $(name) $(objs)
%.o: %.c
	cc	-c $< $(flags)
clean:
	rm	-rf $(objs) $(name)
