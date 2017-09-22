#
# Set these two directories
#

HOMEBIN:=${HOME}/Software/BOMILP_solver/bin
CPLEX_DIR:=/opt/ibm/ILOG/CPLEX_Studio126/cplex


CC:=gcc

#
# This can be left unchanged
#

SOLSTRUCT:=tree

LDFLAGS:=-O3 -L$(CPLEX_DIR)/lib/x86-64_linux/static_pic -lcplex -lpthread -lm
CFLAGS:=-O3 -I$(CPLEX_DIR)/include/ilcplex -Iutil


# SRC_PATH:=$(HOMEBIN)/..
# OBJ_PATH:=${HOMEBIN}/../obj
# SOURCES:=$(shell find $(HOMEBIN)/.. -type f -name *.c)
# OBJECTS:=$(patsubst $(SRC_PATH)/%,$(OBJ_PATH)/%,$(SOURCES:.c=.o))

OBJ:=max_$(SOLSTRUCT).o callbacks.o MIP_solver.o esa_threads.o esa_threadpool.o
HEADER:=max_$(SOLSTRUCT).h bb-bicriteria.h callbacks.h MIP_solver.h

all: ${HOMEBIN}/mip_solve

clean:
	@rm *.o
	@[ -f ${HOMEBIN}/mip_solve ]

${HOMEBIN}/mip_solve: $(OBJ) Makefile $(HEADER)
	@echo Linking $(@F)
	@$(CC) -DSOL_$(SOLSTRUCT) -o ${HOMEBIN}/mip_solve $(OBJ) $(LDFLAGS)

%.o: %.c Makefile $(HEADER)
	# @mkdir -p $(dir $@)
	@echo [${CC}] $<
	@$(CC) -DSOL_$(SOLSTRUCT) -DDEBUG $(CFLAGS) -c $<
