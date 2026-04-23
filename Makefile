CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
LDFLAGS  = -lmosquitto -lmysqlclient

TARGET   = mqtt_mysql_server
SRC      = mqtt_mysql_server.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
