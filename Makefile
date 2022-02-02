DUMMY = dummy

BUILD_DIR	:= ./build
OBJ_DIR		:= $(BUILD_DIR)/objects
APP_DIR		:= $(BUILD_DIR)/apps
LIB_DIR		:= $(BUILD_DIR)/lib
TARGET		:= test/test
TARGET_LIB	:= ejson

CC			:= "D:/InstalledSoftwares/msys64/mingw64/bin/gcc.exe"
CXXFLAGS	:=
CCFLAGS		:= -Wall
LDFLAGS		:= 

INCLUDE		:= -Isrc
SRC_LIB		:=							\
	$(wildcard src/ejson/*.c)
TESTS		:=							\
   test									\
   test_populate
SRC			:=							\
   $(wildcard src/$(TARGET).c)

OBJECTS_LIB			:= $(SRC_LIB:%.c=$(OBJ_DIR)/%.o)
DEPENDENCIES_LIB	:=	\
          $(OBJECTS_LIB:.o=.d)

OBJECTS  := $(SRC:%.c=$(OBJ_DIR)/%.o)
DEPENDENCIES :=	\
          $(OBJECTS:.o=.d)

all: build $(LIB_DIR)/lib$(TARGET_LIB).a $(APP_DIR)/$(TARGET) all_test

$(LIB_DIR)/lib$(TARGET_LIB).a: build $(OBJECTS_LIB)
	@mkdir -p $(@D)
	ar rcs $@ $(OBJECTS_LIB)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	-$(CC) $(CCFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS) $(LIB_DIR)/lib$(TARGET_LIB).a
	@mkdir -p $(@D)
	-$(CC) $(CCFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)


-include $(DEPENDENCIES)
-include $(DEPENDENCIES_LIB)

.PHONY: all all_test build clean debug release info

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)

debug: CCFLAGS += -DDEBUG=4 -g
debug: all

release: CCFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Library dir:     ${LIB_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"