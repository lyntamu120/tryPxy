#include <stdio.h>
#include <string.h>

//return 0 means haven't found yet, return 1 means have found it
int obtainHeader(int *iter, char* buf, char* recvHeader, int rBytes) {
    const char *split = "\r\n\r\n";
    int hasFind = 0;
    if (strstr(buf, split) != NULL) { //this the last packet that contains header information
        hasFind = 1;
    }
    memcpy(recvHeader + (*iter), buf, rBytes);
    *iter = *iter + rBytes;
    return hasFind;
}
