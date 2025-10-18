# Makefile for simple C++ project

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

TARGET = main
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	del $(TARGET).exe 2>nul || true

