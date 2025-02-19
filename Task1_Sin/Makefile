CXX = g++
CXXFLAGS = -std=c++14 -Wall
LDFLAGS = 
SRC = SinArray.cpp
OBJ = $(BUILD_DIR)/$(SRC:.cpp=.o)
TARGET = $(BUILD_DIR)/SinArray
BUILD_DIR = build_make

#cd Tasks/Task1_Sin
#make USE_FLOAT=ON - float
#make USE_FLOAT=OFF - double
#cd build_make 
#./SinArray

ifeq ($(USE_FLOAT), ON)
	CXXFLAGS += -DUSE_FLOAT
	TYPE=float
else
	TYPE=double
endif

$(TARGET): $(OBJ) | $(BUILD_DIR)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

print-type:
	@echo $(TYPE)

all: $(TARGET)
