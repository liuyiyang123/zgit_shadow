#define _XOPEN_SOURCE 700

#include "zNativeUtils.h"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>

#ifdef _Z_BSD
    #include <netinet/in.h>
#endif

#include <time.h>
#include <errno.h>

static void
zdaemonize(const char *zpWorkDir);

static void *
zget_one_line(char *zpBufOUT, _i zSiz, FILE *zpFile);

static _i
zget_str_content(char *zpBufOUT, size_t zSiz, FILE *zpFile);

static void
zsleep(_d zSecs);

static void *
zthread_system(void *zpCmd);

static _i
zdel_linebreak(char *zpStr);

struct zNativeUtils__ zNativeUtils_ = {
    .daemonize = zdaemonize,
    .sleep = zsleep,
    .system = zthread_system,
    .read_line = zget_one_line,
    .read_hunk = zget_str_content,
    .del_lb = zdel_linebreak
};

// /*
//  * Functions for base64 coding [and decoding(TO DO)]
//  */
// char zBase64Dict[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// char *
// zstr_to_base64(const char *zpOrig) {
//     _i zOrigLen = strlen(zpOrig);
//     _i zMax = (0 == zOrigLen % 3) ? (zOrigLen / 3 * 4) : (1 + zOrigLen / 3 * 4);
//     _i zResLen = zMax + (4- (zMax % 4));
//
//     char zRightOffset[zMax], zLeftOffset[zMax];
//
//     char *zRes;
//     zMem_Alloc(zRes, char, zResLen);
//
//     _i i, j;
//
//     for (i = j = 0; i < zMax; i++) {
//         if (3 == (i % 4)) {
//             zRightOffset[i] = 0;
//             zLeftOffset[i] = 0;
//         } else {
//             zRightOffset[i] = zpOrig[j]>>(2 * ((j % 3) + 1));
//             zLeftOffset[i] = zpOrig[j]<<(2 * (2 - (j % 3)));
//             j++;
//         }
//     }
//
//     _c mask = 63;
//     zRes[0] = zRightOffset[0] & mask;
//
//     for (i = 1; i < zMax; i++) { zRes[i] = (zRightOffset[i] | zLeftOffset[i-1]) & mask; }
//     zRes[zMax - 1] = zLeftOffset[zMax - 2] & mask;
//
//     for (i = 0; i < zMax; i++) { zRes[i] = zBase64Dict[(_i)zRes[i]]; }
//     for (i = zMax; i < zResLen; i++) { zRes[i] = '='; }
//
//     return zRes;
// }

/*
 * Daemonize a linux process to daemon.
 */
static void
zclose_fds(pid_t zPid) {
    struct dirent *zpDir_;
    char zStrPid[8], zPath[64];

    sprintf(zStrPid, "%d", zPid);

    strcpy(zPath, "/proc/");
    strcat(zPath, zStrPid);
    strcat(zPath, "/fd");

    _i zFD;
    DIR *zpDir = opendir(zPath);
    while (NULL != (zpDir_ = readdir(zpDir))) {
        zFD = strtol(zpDir_->d_name, NULL, 10);
        if (2 != zFD) { close(zFD); }
    }
    closedir(zpDir);
}

// 这个版本的daemonize会保持标准错误输出描述符处于打开状态
static void
zdaemonize(const char *zpWorkDir) {
    zIgnoreAllSignal();

//  sigset_t zSigToBlock;
//  sigfillset(&zSigToBlock);
//  pthread_sigmask(SIG_BLOCK, &zSigToBlock, NULL);

    umask(0);
    zCheck_Negative_Return(chdir(NULL == zpWorkDir? "/" : zpWorkDir),);

    pid_t zPid = fork();
    zCheck_Negative_Return(zPid,);

    if (zPid > 0) { exit(0); }

    setsid();
    zPid = fork();
    zCheck_Negative_Return(zPid,);

    if (zPid > 0) { exit(0); }

    zclose_fds(getpid());

    _i zFD = open("/dev/null", O_RDWR);
    dup2(zFD, 1);
//  dup2(zFD, 2);
}

/*
 * Fork a child process to exec an outer command.
 * The "zppArgv" must end with a "NULL"
 */
// void
// zfork_do_exec(const char *zpCommand, char **zppArgv) {
//     pid_t zPid = fork();
//     zCheck_Negative_Exit(zPid);
//
//     if (0 == zPid) {
//         execve(zpCommand, zppArgv, NULL);
//     } else {
//         waitpid(zPid, NULL, 0);
//     }
// }

/*
 * 以返回是否是 NULL 为条件判断是否已读完所有数据
 * 可重入，可用于线程
 * 适合按行读取分别处理的场景
 */
static void *
zget_one_line(char *zpBufOUT, _i zSiz, FILE *zpFile) {
    char *zpRes = fgets(zpBufOUT, zSiz, zpFile);
    if (NULL == zpRes && (0 == feof(zpFile))) {
        zPrint_Err(0, NULL, "<fgets> ERROR!");
        exit(1);
    }
    return zpRes;
}

/*
 * 以返回值小于 zSiz 为条件判断是否到达末尾（读完所有数据 )
 * 可重入，可用于线程
 * 适合一次性大量读取所有文本内容的场景
 */
static _i
zget_str_content(char *zpBufOUT, size_t zSiz, FILE *zpFile) {
    size_t zCnt;
    zCheck_Negative_Exit( zCnt = read(fileno(zpFile), zpBufOUT, zSiz) );
    return zCnt;
}

// // 注意：fread 版的实现会将行末的换行符处理掉
// _i
// zget_str_content_1(char *zpBufOUT, size_t zSiz, FILE *zpFile) {
//     size_t zCnt = fread(zpBufOUT, zBytes(1), zSiz, zpFile);
//     if (zCnt < zSiz && (0 == feof(zpFile))) {
//         zPrint_Err(0, NULL, "<fread> ERROR!");
//         exit(1);
//     }
//     return zCnt;
// }

/*
 * 纳秒级sleep，小数点形式赋值
 */
static void
zsleep(_d zSecs) {
    struct timespec zNanoSec_;
    zNanoSec_.tv_sec = (_i) zSecs;
    zNanoSec_.tv_nsec  = (zSecs - zNanoSec_.tv_sec) * 1000000000;
    nanosleep( &zNanoSec_, NULL );
}

/*
 * 纳秒时间，用于两个时间之间精确差值[ 计数有问题，且 CentOS-6 上不可用 ]
 */
// _d
// zreal_time() {
//     struct timespec zNanoSec_;
//     if (0 > clock_gettime(CLOCK_REALTIME, &zNanoSec_)) {
//         return -1.0;
//     } else {
//         return (zNanoSec_.tv_sec + (((_d) zNanoSec_.tv_nsec) / 1000000000));
//     }
// }

/*
 * 用于在单独线程中执行外部命令，如：定时拉取远程代码时，可以避免一个拉取动作卡住，导致后续的所有拉取都被阻塞
 */
static void *
zthread_system(void *zpCmd) {
    if (NULL != zpCmd) { system((char *) zpCmd); }
    return NULL;
}

// /*
//  * 用途：
//  *   从字符串取按指定分割符逐一取出每个字段
//  * 返回值:
//  *   下一个字段的第一个字符在源字符串中的下标（index）
//  * 参数：
//  *   zpOffSet：定义一个整型变量赋值为0，之后循环传入此同一个变量即可
//  *   zpBufOUT：每一次循环后，存放的是取出的字段（子字符串，将原分割符替换为了'\0'）
//  *   zStrLen：是使用 strlen() 函数获得的源字符串的长度（不含 '\0'）
//  * 取值完毕判断条件：
//  *   以返回值大于 (zStrLen + 1) 为条件终止循环取字段
//  */
// _i
// zget_str_field(char *zpBufOUT, char *zpStr, _i zStrLen, char zDelimiter, _i *zpOffSet) {
//     _i i = 0;
//     for (; (*zpOffSet < zStrLen) && (zpStr[*zpOffSet] != zDelimiter); (*zpOffSet)++) {
//         zpBufOUT[i++] = zpStr[*zpOffSet];
//     }
//     zpBufOUT[i] = '\0';
//     (*zpOffSet)++;
//     return *zpOffSet;
// }

// /*
//  *  检查一个目录是否已存在
//  *  返回：1表示已存在，0表示不存在，-1表示出错
//  */
// _i
// zCheck_Dir_Existence(char *zpDirPath) {
//     _i zFd;
//     if (-1 == (zFd = open(zpDirPath, O_RDONLY | O_DIRECTORY))) {
//         if (EEXIST == errno) {
//             return 1;
//         } else {
//             return -1;
//         }
//     }
//     close(zFd);
//     return 0;
// }

/*
 * 去除用字符串末尾的一个或多个换行符LB (Line Break)
 * 返回新的字符串长度，不含最后的 '\0'
 */
static _i
zdel_linebreak(char *zpStr) {
    char *zpStrPtr = zpStr;
    _ui zStrLen = strlen(zpStr);

    while ('\n' == zpStrPtr[zStrLen - 1]) { zStrLen--; }
    zpStrPtr[zStrLen] = '\0';

    return zStrLen;
}
