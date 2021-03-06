# Get Trilinos as one entity
# MYAPP_TRILINOS_DIR = /Users/brian/Documents/GitHub/Trilinos/build_MPI/usr/local
MYAPP_TRILINOS_DIR = /usr/local/include/trilinos
include $(MYAPP_TRILINOS_DIR)/include/Makefile.export.Trilinos

# Make sure to use same compilers and flags as Trilinos
CXX=$(Trilinos_CXX_COMPILER)
CC=$(Trilinos_C_COMPILER)
FORT=$(Trilinos_Fortran_COMPILER)

CXX_FLAGS=$(Trilinos_CXX_COMPILER_FLAGS) $(USER_CXX_FLAGS)
C_FLAGS=$(Trilinos_C_COMPILER_FLAGS) $(USERC_FLAGS)
FORT_FLAGS=$(Trilinos_Fortran_COMPILER_FLAGS) $(USER_FORT_FLAGS)

MY_DIRS=-I/Users/Brian/Documents
INCLUDE_DIRS=$(Trilinos_INCLUDE_DIRS) $(Trilinos_TPL_INCLUDE_DIRS)
LIBRARY_DIRS=$(Trilinos_LIBRARY_DIRS) $(Trilinos_TPL_LIBRARY_DIRS)
#LIBRARIES=$(Trilinos_LIBRARIES) $(Trilinos_TPL_LIBRARIES)

LIBRARIES=-lml -lifpack -lamesos -lgaleri-xpetra -lgaleri-epetra -laztecoo -lisorropia -lxpetra-sup -lxpetra -lthyratpetra -lthyraepetraext -lthyraepetra -lthyracore -lthyratpetra -lthyraepetraext -lthyraepetra -lthyracore -lepetraext -ltrilinosss -ltpetraext -ltpetrainout -ltpetra -lkokkostsqr -ltpetraclassiclinalg -ltpetraclassicnodeapi -ltpetraclassic -ltpetraext -ltpetrainout -ltpetra -lkokkostsqr -ltpetraclassiclinalg -ltpetraclassicnodeapi -ltpetraclassic -ltriutils -lzoltan -lepetra -lrtop -lkokkoskernels -lteuchoskokkoscomm -lteuchoskokkoscompat -lteuchosremainder -lteuchosnumerics -lteuchoscomm -lteuchosparameterlist -lteuchosparser -lteuchoscore -lteuchoskokkoscomm -lteuchoskokkoscompat -lteuchosremainder -lteuchosnumerics -lteuchoscomm -lteuchosparameterlist -lteuchosparser -lteuchoscore -lkokkosalgorithms -lkokkoscontainers -lkokkoscore -lkokkosalgorithms -lkokkoscontainers -lkokkoscore -lmetis -lparmetis /usr/lib/libdl.dylib /usr/lib/liblapack.dylib /usr/lib/libblas.dylib

LINK_FLAGS=$(Trilinos_EXTRA_LD_FLAGS)

#just assuming that epetra is turned on.
DEFINES=-DMYAPP_EPETRA

SRC_DIR = /Users/brian/Documents/GitHub/TrilinosUQComp/src/
CFLAG = -I$(SRC_DIR)
SRC = $(wildcard $(SRC_DIR)*.cpp)
OBJ = $(SRC:.cpp=.o)
HDR = $(wildcard $(SRC_DIR)*.hpp)
MAIN_HDR = $(wildcard *.hpp)

default: print_info

# Echo trilinos build info just for fun
print_info:
	@echo "\nFound Trilinos!  Here are the details: "
	@echo "   Trilinos_VERSION = $(Trilinos_VERSION)"
	@echo "   Trilinos_PACKAGE_LIST = $(Trilinos_PACKAGE_LIST)"
	@echo "   Trilinos_LIBRARIES = $(Trilinos_LIBRARIES)"
	@echo "   Trilinos_INCLUDE_DIRS = $(Trilinos_INCLUDE_DIRS)"
	@echo "   Trilinos_LIBRARY_DIRS = $(Trilinos_LIBRARY_DIRS)"
	@echo "   Trilinos_TPL_LIST = $(Trilinos_TPL_LIST)"
	@echo "   Trilinos_TPL_INCLUDE_DIRS = $(Trilinos_TPL_INCLUDE_DIRS)"
	@echo "   Trilinos_TPL_LIBRARIES = $(Trilinos_TPL_LIBRARIES)"
	@echo "   Trilinos_TPL_LIBRARY_DIRS = $(Trilinos_TPL_LIBRARY_DIRS)"
	@echo "   Trilinos_BUILD_SHARED_LIBS = $(Trilinos_BUILD_SHARED_LIBS)"
	@echo "End of Trilinos details\n"
	@echo "   SRC_DIR = $(SRC_DIR)"
	@echo "   CFLAG   = $(CFLAG)"
	@echo "   SRC     = $(SRC)"
	@echo "   OBJ     = $(OBJ)"
	@echo "   HDR     = $(HDR)"

#build
build: $(OBJ) main.o
	$(CXX) $(CXX_FLAGS) $^ -o trilinos_mpi $(CFLAG) $(LINK_FLAGS) $(INCLUDE_DIRS) $(DEFINES) $(LIBRARY_DIRS) $(LIBRARIES)

$(OBJ): $(HDR)
main.o: $(MAIN_HDR) $(HDR)

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -o $@ -c $< $(LINK_FLAGS) $(INCLUDE_DIRS) $(CFLAG) $(DEFINES)

.PHONY: clean
clean:
	rm -f *.o *.a *.exe
