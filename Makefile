#
# Set these two directories
#

HOMEBIN=${HOME}/Dropbox\ \(Edinboro\ University\)/Edinboro/Research/MIP_based_solver/simpler_version
CPLEX_DIR=$(HOME)/ILOG/CPLEX_Studio126/cplex

CC=gcc

#
# This can be left unchanged
#

SOLSTRUCT=tree

LDFLAGS= -O3 -L$(CPLEX_DIR)/lib/x86-64_linux/static_pic -lcplex -lpthread -lm
CFLAGS=  -O3 -I$(CPLEX_DIR)/include/ilcplex

OBJ = max_$(SOLSTRUCT).o callbacks.o MIP_solver.o
HEADER = max_$(SOLSTRUCT).h bb-bicriteria.h callbacks.h MIP_solver.h

all: ${HOMEBIN}/mip_solve

clean:
	@rm *.o
	@[ -f ${HOMEBIN}/mip_solve ]

${HOMEBIN}/mip_solve: $(OBJ) Makefile $(HEADER)
	@echo Linking $(@F)
	@$(CC) -DSOL_$(SOLSTRUCT) -o ${HOMEBIN}/mip_solve $(OBJ) $(LDFLAGS)

%.o: %.c Makefile $(HEADER)
	@echo [${CC}] $<
	@$(CC) -DSOL_$(SOLSTRUCT) $(CFLAGS) -c $< 
