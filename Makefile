TARGET = myfs		# 目标文件
SOURCES = my_fs.c utils.c my_cmd.c		# 源文件
HEADERS = my_fs.h		# 头文件
CC = gcc		# 编译器
CFLAGS = -Wall -Wextra -O2		# 编译选项
LDFLAGS =		# 链接选项
OBJECTS = $(SOURCES:.c=.o)		# 目标文件依赖的对象文件

# 默认目标
all: $(TARGET)
# 编译目标文件
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)
# 编译源文件
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
# 清理
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECTS)
