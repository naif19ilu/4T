objs = main.o front.o back.o cxa.o
flags = -Wall -Wextra -Wpedantic
final = 4T

all: $(final)

$(final): $(objs)
	cc -o $(final) $(objs)
%.o: %.c
	cc -c $< $(flags)
clean:
	rm -rf $(final) $(objs)
