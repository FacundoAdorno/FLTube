# Variables
CXX      = $(shell fltk-config --cxx)
DEBUG    = -g
CXXFLAGS = $(shell fltk-config --use-gl --use-images --cxxflags) -Iinclude `pkg-config --cflags libcurl`
#LDFLAGS  = $(shell fltk-config --use-gl --use-images --ldflags) `pkg-config --libs libcurl`
LDSTATIC  = $(shell fltk-config --use-gl --use-images --ldstaticflags) `pkg-config --libs libcurl`
LINK     = $(CXX)

BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
FLUID_PATHFILE = fluid/FLTube_View.fl
# Archivos
TARGET = $(BUILD_DIR)/fltube_022
SOURCES_LIST = fltube_utils.cxx FLTube_View.cxx FLTube.cxx
SOURCES = $(addprefix $(SRC_DIR)/, $(SOURCES_LIST))
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cxx=$(BUILD_DIR)/%.o)

# Reglas
.SUFFIXES: .o .cxx

# Regla principal
all: build $(TARGET)

# Regla para enlazar el ejecutable
$(TARGET): $(OBJECTS)
	$(LINK) -o $@ $^ $(LDSTATIC)

# Regla para compilar archivos objeto
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cxx
	$(CXX) $(CXXFLAGS) $(DEBUG) -c $< -o $@

# Regla para crear el directorio build
build:
	mkdir -p build

# Regla para compilar archivos FLUID
compile_fluid:
	cd `dirname $(FLUID_PATHFILE)` && fluid -o ../$(SRC_DIR)/FLTube_View.cxx -h ../$(INCLUDE_DIR)/FLTube_View.h -c `basename $(FLUID_PATHFILE)`

# Regla para limpiar archivos objeto y el ejecutable
clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET) 2> /dev/null && rmdir $(BUILD_DIR)
