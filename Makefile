# Variables
CXX = g++
CXXFLAGS = -Wall -g -std=c++11 -pthread

SRC_DIR = src
INC_DIR = inc
BUILD_DIR = bin

THIRD_PARTY_SRC_DIR = $(INC_DIR)\third_party\SOLO-motor-controllers-CPP-library\src
THIRD_PARTY_INC_DIR = $(INC_DIR)\third_party\SOLO-motor-controllers-CPP-library\inc

TARGET = $(BUILD_DIR)\server.exe

# Source and object files
SOURCES = $(SRC_DIR)\server.cpp # Add more source files here if needed
OBJECTS = $(BUILD_DIR)\server.o # Corresponding object files

# Third-party library source and object files
THIRD_PARTY_SOURCES = \
    $(THIRD_PARTY_SRC_DIR)\SOLOMotorControllers.cpp \
    $(THIRD_PARTY_SRC_DIR)\SOLOMotorControllersSerial.cpp \
    $(THIRD_PARTY_SRC_DIR)\SOLOMotorControllersUtils.cpp

THIRD_PARTY_OBJECTS = \
    $(BUILD_DIR)\third_party_SOLOMotorControllers.o \
    $(BUILD_DIR)\third_party_SOLOMotorControllersSerial.o \
    $(BUILD_DIR)\third_party_SOLOMotorControllersUtils.o

# Includes
INCLUDES = -I$(INC_DIR) -I$(THIRD_PARTY_INC_DIR) -I$(THIRD_PARTY_SRC_DIR)

LIBS = -lsetupapi

# Build target
all: $(BUILD_DIR) $(TARGET)

# Create build directory if it does not exist
$(BUILD_DIR):
	cmd /c mkdir $(BUILD_DIR)

# Link the target executable with the third-party objects
$(TARGET): $(OBJECTS) $(THIRD_PARTY_OBJECTS)
	$(CXX) $(OBJECTS) $(THIRD_PARTY_OBJECTS) $(LIBS) -o $(TARGET)

# Compile source files
$(BUILD_DIR)\server.o: $(SRC_DIR)\server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile third-party source files
$(BUILD_DIR)\third_party_SOLOMotorControllers.o: $(THIRD_PARTY_SRC_DIR)\SOLOMotorControllers.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)\third_party_SOLOMotorControllersSerial.o: $(THIRD_PARTY_SRC_DIR)\SOLOMotorControllersSerial.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)\third_party_SOLOMotorControllersUtils.o: $(THIRD_PARTY_SRC_DIR)\SOLOMotorControllersUtils.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up build artifacts
clean:
	powershell -Command "if (Test-Path -Path '$(BUILD_DIR)') { Remove-Item -Path '$(BUILD_DIR)' -Recurse -Force -Verbose }"

# Phony targets
.PHONY: all clean