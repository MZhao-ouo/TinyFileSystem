#include "my_fs.h"

int fill_useropen(int index, fcb *fcbptr, char *dir, int count, char fcbstate, char topenfile) {
    strcpy(openfilelist[index].filename, fcbptr->filename);
    strcpy(openfilelist[index].exname, fcbptr->exname);
    openfilelist[index].attribute = fcbptr->attribute;
    openfilelist[index].time = fcbptr->time;
    openfilelist[index].date = fcbptr->date;
    openfilelist[index].first = fcbptr->first;
    openfilelist[index].length = fcbptr->length;

    strcpy(openfilelist[index].dir, dir);
    openfilelist[index].count = count;
    openfilelist[index].fcbstate = fcbstate;
    openfilelist[index].topenfile = topenfile;

    return TRUE;
}

int fill_fcb(fcb *fcbptr, char *filename, char *exname, unsigned char attribute, unsigned short time, unsigned short date, unsigned short first, unsigned long length) {
    strcpy(fcbptr->filename, filename);
    strcpy(fcbptr->exname, exname);
    fcbptr->attribute = attribute;
    fcbptr->time = time;
    fcbptr->date = date;
    fcbptr->first = first;
    fcbptr->length = length;
    fcbptr->free = 1;

    return TRUE;
}

int allocate_fat(int num) {
    int first = -1;
    int last = -1;
    for (int i = 5; i < 1000; i++) {
        if (fat1[i].id == FREE && fat2[i].id == FREE) {
            if (first == -1) {
                first = i;
            } else {
                fat1[last].id = i;
                fat2[last].id = i;
            }
            num--;
            if (num == 0) {
                fat1[i].id = END;
                fat2[i].id = END;
                break;
            } else {
                last = i;
            }
        }
    }
    
    return first;
}
