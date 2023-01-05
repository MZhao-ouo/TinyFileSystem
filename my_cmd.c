#include "my_fs.h"

time_t init_time, current_time;

/* 格式化磁盘，即初始化磁盘上的文件系统 */
void my_format() {
    init_time = time(NULL);

    memset(myvhard, 0, SIZE);   // 将虚拟磁盘清零
    block0 *b0 = (block0 *)myvhard; // 指向引导块
    strcpy(b0->information, "Disk Size:1024  Block Num:1000");
    b0->root = 5;
    b0->startblock = myvhard + BLOCK_SIZE * 5;

    fat *fat1;
    fat *fat2;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);    // 指向FAT1
    fat2 = (fat *)(myvhard + BLOCK_SIZE * 2);    // 指向FAT2
    for (int i = 0; i < 5; i++) {
        fat1[i].id = END;
        fat2[i].id = END;
    }
    for (int i = 5; i < 1000; i++) {
        fat1[i].id = FREE;
        fat2[i].id = FREE;
    }
    
    fcb *root = (fcb *)(myvhard + BLOCK_SIZE * 5);   // 指向根目录
    int start_fat = allocate_fat(2);
    memset(root, 0, BLOCK_SIZE * 2);

    current_time = time(NULL);
    fill_fcb(&root[0], ".", 0, current_time-init_time, 0, start_fat, BLOCK_SIZE);
    fill_fcb(&root[1], "..", 0, current_time-init_time, 0, start_fat, BLOCK_SIZE);

    startp = b0->startblock;  // 指向数据区的起始位置，前面有一个引导块，两个FAT1，两个FAT2
    memset(openfilelist, 0, sizeof(openfilelist));   // 初始化用户打开文件表
    fill_useropen(0, (fcb *)startp, "/", 0, 0, 1);   // 将表项0分配给根目录文件
    strcpy(currentdir, openfilelist[0].dir);    // 设置根目录为当前目录
    curdir = 0;
}

void my_cd(char *dirname) {
    if (strcmp(dirname, ".") == 0)
        return;
    // cd复合路径
    for (char *c = dirname; *c != '\0'; c++) {
        if (*c == '/') {
            if (strcmp(dirname, "/") == 0) {
                curdir = 0;
                strcpy(currentdir, "/");
                return;
            }
            if (dirname[0] == '/')    // 绝对路径
                my_cd("/");
            char *token = strtok(dirname, "/");
            while (token != NULL) {
                my_cd(token);
                token = strtok(NULL, "/");
            }
            return;
        }
    }

    if (strlen(dirname) > 8) {
        printf("Directory name \"%s\" too long!\n", dirname);
        return;
    }

    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);       // 指向当前目录的FCB
    
    // 返回上级目录
    if (strcmp(dirname, "..") == 0) {
        if (curdir == 0) {
            printf("Already in root directory!\n");
            return;
        } else {
            strcpy(currentdir, openfilelist[curdir].dir);
            char tmp[80];
            strcpy(tmp, currentdir);
            tmp[strlen(tmp) - strlen(openfilelist[curdir].filename) - 1] = '\0';
            fill_useropen(curdir, &curdirptr[1], tmp, 0, 0, 1);
        }
        return;
    }

    for (int i = 2; i < MAXFCB; i++) {
        if (strcmp(curdirptr[i].filename, dirname) == 0 && curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 0) {
                curdir = 1;
                fill_useropen(curdir, &curdirptr[i], currentdir, 0, 0, 1);
                strcat(currentdir, dirname);
                strcat(currentdir, "/");
            } else {
                printf("It is not a directory!\n");
            }
            return;
        }
    }

    printf("Directory \"%s\" not found!\n", dirname);       // 没有找到目录
}

void my_mkdir(char *dirname) {
    if (strlen(dirname) > 8) {
        printf("Directory name is too long!\n");
        return;
    }
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);
    for (int i = 0; i < MAXFCB; i++) {
        if (curdirptr[i].free == 1 && strcmp(curdirptr[i].filename, dirname) == 0) {
            printf("Filename exist!\n");
            return;
        }
    }

    int start_fat = allocate_fat(1);
    int flag = 0;
    fcb *newdir = (fcb *)(myvhard + BLOCK_SIZE * start_fat);

    for (int i = 0; i < MAXFCB; i++) {
        if (curdirptr[i].free == 0) {
            current_time = time(NULL);
            fill_fcb(&curdirptr[i], dirname, 0, current_time-init_time, 0, start_fat, BLOCK_SIZE);
            flag = 1;
            break;
        }
    }
    if (flag == 0) {
        printf("Directory is full!\n");
        return;
    }
    // 初始化新目录的FCB
    for(int i = 0; i < MAXFCB; i++)
        newdir[i].free = 0;
    fill_fcb(&newdir[0], ".", 0, current_time-init_time, 0, start_fat, BLOCK_SIZE);
    fill_fcb(&newdir[1], "..", 0, current_time-init_time, 0, openfilelist[curdir].first, BLOCK_SIZE);

    openfilelist[curdir].modify_stamp = time(NULL) - init_time;
    openfilelist[curdir].fcbstate = 1;
}

void my_rmdir(char *dirname) {
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);

    for (int i = 0; i < MAXFCB; i++) {
        if (strcmp(curdirptr[i].filename, dirname) == 0 && curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 0) {
                // 判断目录是否为空
                fcb *targetDirPtr = (fcb *)(myvhard + BLOCK_SIZE * curdirptr[i].first);
                for (int j = 2; j < MAXFCB; j++) {
                    if (targetDirPtr[j].free == 1) {
                        printf("Directory is not empty!\n");
                        return;
                    }
                }
                curdirptr[i].free = 0;
                free_fat(curdirptr[i].first);
                openfilelist[curdir].modify_stamp = time(NULL) - init_time;
                openfilelist[curdir].fcbstate = 1;
            } else {
                printf("It is not a directory!\n");
            }
            return;
        }
    }

    printf("Directory not found!\n");
}

void my_ls() {
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);
    char attribute[80];

    printf("Name\tAttri\tcTime\t\t\tmTime\t\t\tSize\n");
    for (int i = 0; i < MAXFCB; i++) {
        if (curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 0)
                strcpy(attribute, "Dir");
            else
                strcpy(attribute, "File");
            time_t ctimecal_ptr = curdirptr[i].create_stamp + init_time;
            struct tm *ctr = localtime(&ctimecal_ptr);
            time_t mtimecal_ptr = curdirptr[i].modify_stamp + init_time;
            struct tm *mtr = localtime(&mtimecal_ptr);
            printf("%s\t%s\t", curdirptr[i].filename, attribute);
            printf("%d-%d-%d %d:%d:%d\t", (1900+ctr->tm_year), (1+ctr->tm_mon), ctr->tm_mday, ctr->tm_hour, ctr->tm_min, ctr->tm_sec);
            printf("%d-%d-%d %d:%d:%d\t", (1900+mtr->tm_year), (1+mtr->tm_mon), mtr->tm_mday, mtr->tm_hour, mtr->tm_min, mtr->tm_sec);
            printf("%dB\n", curdirptr[i].length);
        }
    }
}

void my_create(char *filename) {
    if (strlen(filename) > 8) {
        printf("Filename is too long!\n");
        return;
    }
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);
    for (int i = 0; i < MAXFCB; i++) {
        if (curdirptr[i].free == 1 && strcmp(curdirptr[i].filename, filename) == 0) {
            printf("Filename exist!\n");
            return;
        }
    }
    int start_fat = allocate_fat(1);
    int flag = 0;
    fcb *newblock = (fcb *)(myvhard + BLOCK_SIZE * start_fat);

    int index = 0;
    for (index = 0; index < MAXFCB; index++) {
        if (curdirptr[index].free == 0) {
            current_time = time(NULL);
            fill_fcb(&curdirptr[index], filename, 1, current_time-init_time, 0, start_fat, BLOCK_SIZE);
            flag = 1;
            break;
        }
    }
    if (flag == 0) {
        printf("Directory is full!\n");
        return;
    }
    memset((void *)newblock, 0, BLOCK_SIZE);

    my_open(filename);
    openfilelist[curdir].modify_stamp = time(NULL) - init_time;
    openfilelist[curdir].fcbstate = 1;
}

void my_rm(char *filename) {
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);

    for (int i = 0; i < MAXFCB; i++) {
        if (strcmp(curdirptr[i].filename, filename) == 0 && curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 1) {
                for (int j = 0; j < MAXOPENFILE; j++) {
                    if (strcmp(openfilelist[j].filename, filename) == 0 && strcmp(openfilelist[j].dir, currentdir) == 0 && openfilelist[j].topenfile == 1) {
                        printf("File is opened!\n");
                        return;
                    }
                }
                curdirptr[i].free = 0;
                free_fat(curdirptr[i].first);
                openfilelist[curdir].modify_stamp = time(NULL) - init_time;
                openfilelist[curdir].fcbstate = 1;
            } else {
                printf("It is not a file!\n");
            }
            return;
        }
    }

    printf("File not found!\n");
}

void my_open(char *filename) {
    fcb *curdirptr = (fcb *)(myvhard + BLOCK_SIZE * openfilelist[curdir].first);

    for (int i = 0; i < MAXFCB; i++) {
        if (strcmp(curdirptr[i].filename, filename) == 0 && curdirptr[i].free == 1) {
            if (curdirptr[i].attribute == 1) {
                for (int j = 0; j < MAXOPENFILE; j++) {
                    if (strcmp(openfilelist[j].filename, filename) == 0 && strcmp(openfilelist[j].dir, currentdir) == 0 && openfilelist[j].topenfile == 1) {
                        printf("File has opened!\n");
                        return;
                    }
                }
                int newfile_fd = find_fd();
                if (newfile_fd == -1) {
                    printf("Open file list is full!Open error\n");
                    return;
                }
                fill_useropen(newfile_fd, &curdirptr[i], currentdir, 0, 0, 1);
                printf("File \"%s\" open success!fd is %d\n", filename, newfile_fd);
                return;
            } else {
                printf("It is not a file!\n");
                return;
            }
        }
    }
    printf("File not found!\n");
}

void my_close(int fd) {
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("fd is illegal!\n");
        return;
    }
    if (openfilelist[fd].topenfile == 0) {
        printf("File is not opened!\n");
        return;
    }

    if (openfilelist[fd].fcbstate == 1) {
        // 找出其父目录的磁盘
        int block = get_dir_block(openfilelist[fd].dir);
        fcb *parentdir = (fcb *)(myvhard + BLOCK_SIZE * block);
        for (int i = 0; i < MAXFCB; i++) {
            if (strcmp(parentdir[i].filename, openfilelist[fd].filename) == 0) {
                fill_fcb(&parentdir[i], openfilelist[fd].filename, openfilelist[fd].attribute, openfilelist[fd].create_stamp, openfilelist[fd].modify_stamp, openfilelist[fd].first, openfilelist[fd].length);
            }
        }
    }

    openfilelist[fd].topenfile = 0;
    printf("File \"%s\" close success!\n", openfilelist[fd].filename);
}

int  my_write(int fd) {
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("fd is illegal !\n");
        return -1;
    }
    int wstyle;
    while (1) {
        printf("Input: 0=truncate write, 1=overwrite write, 2=append write\n");
        scanf("%d", &wstyle);
        if (wstyle > 2 || wstyle < 0) {
            printf("Command error!\n");
        }
        else {
            break;
        }
    }
    char text[MAX_TEXT_SIZE] = "\0";
    char textTmp[MAX_TEXT_SIZE] = "\0";
    char Tmp[MAX_TEXT_SIZE] = "\0";
    char Tmp2[4] = "\0";

    printf("Please enter the file data, and end the file with END\n");
    getchar();
    while (fgets(Tmp, 100, stdin)) {
        for (int i = 0; i < strlen(Tmp) - 1; i++)
        {
            textTmp[i] = Tmp[i];
            textTmp[i + 1] = '\0';
        }
        if (strlen(Tmp) >= 3) 
        {
            Tmp2[0] = Tmp[strlen(Tmp) - 4];
            Tmp2[1] = Tmp[strlen(Tmp) - 3];
            Tmp2[2] = Tmp[strlen(Tmp) - 2];
            Tmp2[3] = '\0';
        }
        if (strcmp(textTmp, "END") == 0 || strcmp(Tmp2, "END") == 0)
        {
            break;
        }
        textTmp[strlen(textTmp)] = '\n';
        strcat(text, textTmp);

    }
    text[strlen(text)] = '\0';
    
    do_write(fd, text, strlen(text) + 1, wstyle);
    openfilelist[fd].fcbstate = 1;
    openfilelist[fd].modify_stamp = time(NULL) - init_time;
    return 1;
}
int  do_write(int fd, char* text, int len, char wstyle) {
    int blockNum = openfilelist[fd].first;
    fat* fatPtr = (fat*)(myvhard + BLOCK_SIZE) + blockNum;

    fat* fat1 = (fat*)(myvhard + BLOCK_SIZE);
    if (wstyle == 0) {
        openfilelist[fd].count = 0;
        openfilelist[fd].length = 0;
    }
    else if (wstyle == 1) {
        if (openfilelist[fd].attribute == 1 && openfilelist[fd].length != 0) {
            openfilelist[fd].count -= 1;
        }
    }
    else if (wstyle == 2) {
        if (openfilelist[fd].attribute == 0) {
            openfilelist[fd].count = openfilelist[fd].length;
        }
        else if (openfilelist[fd].attribute == 1 && openfilelist[fd].length != 0) {
            openfilelist[fd].count = openfilelist[fd].length - 1;
        }
    }


    int off = openfilelist[fd].count;

    while (off >= BLOCK_SIZE) {
        blockNum = fatPtr->id;
        if (blockNum == END) {
            blockNum = allocate_fat(1);
            if (blockNum == END) {
                printf("Insufficient disc blocks!\n");
                return -1;
            }
            else {
                //update FAT
                fatPtr->id = blockNum;
                fatPtr = (fat*)(myvhard + BLOCK_SIZE + blockNum);
                fatPtr->id = END;
            }
        }
        fatPtr = (fat*)(myvhard + BLOCK_SIZE) + blockNum;
        off -= BLOCK_SIZE;
    }

    unsigned char* buf = (unsigned char*)malloc(BLOCK_SIZE * sizeof(unsigned char));
    if (buf == NULL) {
        printf("Failed to request memory space!\n");
        return -1;
    }


    fcb* dBlock = (fcb*)(myvhard + BLOCK_SIZE * blockNum);
    fcb* dFcb = (fcb*)(text);
    unsigned char* blockPtr = (unsigned char*)(myvhard + BLOCK_SIZE * blockNum);				
    int lenTmp = 0;
    char* textPtr = text;
    fcb* dFcbBuf = (fcb*)(buf);

    while (len > lenTmp) {
        memcpy(buf, blockPtr, BLOCK_SIZE);
        for (; off < BLOCK_SIZE; off++) {
            *(buf + off) = *textPtr;
            textPtr++;
            lenTmp++;
            if (len == lenTmp) {
                break;
            }
        }
        memcpy(blockPtr, buf, BLOCK_SIZE);

        if (off == BLOCK_SIZE && len != lenTmp) {
            off = 0;
            blockNum = fatPtr->id;
            if (blockNum == END) {
                blockNum = allocate_fat(1);
                if (blockNum == END) {
                    printf("Run out of disc blocks!\n");
                    return -1;
                }
                else {
                    blockPtr = (unsigned char*)(myvhard + BLOCK_SIZE * blockNum);
                    fatPtr->id = blockNum;
                    fatPtr = (fat*)(myvhard + BLOCK_SIZE) + blockNum;
                    fatPtr->id = END;
                }
            }
            else {
                blockPtr = (unsigned char*)(myvhard + BLOCK_SIZE * blockNum);
                fatPtr = (fat*)(myvhard + BLOCK_SIZE) + blockNum;
            }
        }
    }
    openfilelist[fd].count += len;

    if (openfilelist[fd].count > openfilelist[fd].length)
        openfilelist[fd].length = openfilelist[fd].count;
    free(buf);
    int i = blockNum;
    while (1) {
        if (fat1[i].id != END) {
            int next = fat1[i].id;
            fat1[i].id = FREE;
            i = next;
        }
        else {
            break;
        }
    }
    fat1[blockNum].id = END;
    memcpy((fat*)(myvhard + BLOCK_SIZE * 3), (fat*)(myvhard + BLOCK_SIZE), 2 * BLOCK_SIZE);
    return len;

}

int  do_read(int fd, int len, char* text) {
    int lenTmp = len;
    unsigned char* buf = (unsigned char*)malloc(1024);
    if (buf == NULL) {
        printf("do_read failed to request memory space\n");
        return -1;
    }

    int off = openfilelist[fd].count;
    int blockNum = openfilelist[fd].first;
    fat* fatPtr = (fat*)(myvhard + BLOCK_SIZE) + blockNum;
    while (off >= BLOCK_SIZE) {
        off -= BLOCK_SIZE;
        blockNum = fatPtr->id;
        if (blockNum == END) {
            printf("The block sought by do_read does not exist\n");
            return -1;
        }
        fatPtr = (fat*)(myvhard + BLOCK_SIZE) + blockNum;
    }
    unsigned char* blockPtr = myvhard + BLOCK_SIZE * blockNum;
    memcpy(buf, blockPtr, BLOCK_SIZE);
    char* textPtr = text;
    fcb* debug = (fcb*)text;
    while (len > 0) {
        if (BLOCK_SIZE - off > len) {
            memcpy(textPtr, buf + off, len);
            textPtr += len;
            off += len;
            openfilelist[fd].count += len;
            len = 0;
        }
        else {
            memcpy(textPtr, buf + off, BLOCK_SIZE - off);
            textPtr += BLOCK_SIZE - off;
            off = 0;
            len -= BLOCK_SIZE - off;
            blockNum = fatPtr->id;
            if (blockNum == END) {
                printf( "The len length is too long, exceeds the file size!\n");
                break;
            }
            fatPtr = (fat*)(myvhard + BLOCK_SIZE) + blockNum;
            blockPtr = myvhard + BLOCK_SIZE * blockNum;
            memcpy(buf, blockPtr, BLOCK_SIZE);
        }
    }
    free(buf);
    return lenTmp - len;
}

int  my_read(int fd, int len) {
    if (fd >= MAXOPENFILE || fd < 0) {
        printf("File does not exist\n");
        return -1;
    }
    openfilelist[fd].count = 0;
    char text[MAX_TEXT_SIZE] = "\0";

    if (len > openfilelist[fd].length)
    {
        printf("The read length has exceeded the file size and the default is to read to the end of the file.\n");
        len = openfilelist[fd].length;
    }
    do_read(fd, len, text);
    puts(text);
    return 1;
}

void my_exitsys() {
    FILE *fp = fopen("./my_disk", "wb");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
    free(myvhard);
    
    exit(0);
}
