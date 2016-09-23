CC := clang
LIBS := -lm -lGL `pkg-config --cflags glfw3` `pkg-config --static --libs glfw3`

FLAGS := -Werror -fno-builtin -ffast-math
FLAGS += -Wextra -pedantic -Wall
#Clang specific
FLAGS += -Weverything

# Disable cast align warning to allow push allocator
FLAGS += -Wno-cast-align -Wno-unused-function

DEBUG=0
DOUBLE=0
CUSTOM_SORT=1

ifeq ($(DEBUG),1)
	FLAGS += -O0 -DDEBUG -ggdb3
else
	FLAGS += -O3 -DNDEBUG
endif

ifeq ($(DOUBLE),1)
	FLAGS += -DMPE_POLY2TRI_USE_DOUBLE
endif

ifeq ($(CUSTOM_SORT),1)
	FLAGS += -DMPE_POLY2TRI_USE_CUSTOM_SORT
endif


all:
	$(CC) $(FLAGS) testbed/main.c -o main $(LIBS)
