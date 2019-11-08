#VERSION 	=1.00
CC		=g++
DEBUG		=
CFLAGS		=-g  -Wall
SOURCES	 	=$(wildcard *.cpp)
INCLUDES  	=-I ../liveMedia/include/ -I ../groupsock/include/ -I ../UsageEnvironment/include/ -I ../BasicUsageEnvironment/include/
LIB_NAMES 	=-lliveMedia -lUsageEnvironment -lBasicUsageEnvironment -lgroupsock -lfaad -lavcodec -lavutil
LIB_PATH 	=-L../liveMedia/ -L../groupsock/  -L../UsageEnvironment  -L../BasicUsageEnvironment
OBJ			=$(patsubst %.c, %.o, $(SOURCES))
TARGET		=LiveRtspClient

#links
$(TARGET):$(OBJ)
	$(CC) $(OBJ) $(INCLUDES) $(LIB_PATH) $(LIB_NAMES) -o $(TARGET)
	
#compile
%.o: %.c
	$(CC) $(INCLUDES) $(DEBUG) -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	@echo "Remove linked and compiled files......"



