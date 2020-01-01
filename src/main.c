#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

static u32 Flags = 0;
static u32 Number = 1;

static struct tm *Date = 0;
static u32 Year  = 1970;
static u32 Month = 1;
static u32 Day   = 1;

enum Flags {
    ANY  = 0x1 << 0,
    YEAR = 0x1 << 1,
    EDIT = 0x1 << 2,
    NEW  = 0x1 << 3,
    PAY  = 0x1 << 4,
};

static
bool StringsAreEqual(char *A, char *B) {
    while (*A && *A == *B) {
        ++A;
        ++B;
    }

    return *A == *B;
}

#if 0
static
mm BufferString(char *Buffer, mm Size, char *Str) {
    Assert(Size);

    char *End = Buffer + Size - 1;
    char *Cur = Str;

    while (*Cur && Buffer < End) {
        *Buffer++ = *Cur++;
    }

    *Buffer = 0;

    return (mm)(Cur - Str);
}
#endif

static
bool StringStartsWith(char *A, char *B) {
    while (*B && *A == *B) {
        ++A;
        ++B;
    }

    return *A == *B || *B == 0;
}

static
void ProcessFlag(char *Flag) {
    enum {
        ACTION_NONE,
        ACTION_MONTHS,
        ACTION_YEARS,
    } Action = ACTION_NONE;

    if (!Flag) {
        /* ignore null flag */
    }
    if (StringsAreEqual(Flag, "all")) {
        Flags |= YEAR;
        Year = 0;
    }
    else if (StringsAreEqual(Flag, "edit")) {
        Flags |= EDIT;
    }
    else if (StringsAreEqual(Flag, "new")) {
        Flags |= EDIT | NEW;
    }
    else if (StringsAreEqual(Flag, "help")) {
        /* TODO(lrak): print help */
    }
    else if (StringsAreEqual(Flag, "year")) {
        Flags |= YEAR;
    }
    else if (StringsAreEqual(Flag, "month")) {
        Flags &= ~YEAR;

        if (!Year) {
            Year = Date->tm_year + 1900;
        }
    }
    else if (StringsAreEqual(Flag, "last-month")) {
        Flag = "months-ago=1";
        Action = ACTION_MONTHS;
    }
    else if (StringsAreEqual(Flag, "last-year")) {
        Flag = "years-ago=1";
        Action = ACTION_YEARS;
    }
    else if (StringStartsWith(Flag, "years-ago=")) {
        Action = ACTION_YEARS;
    }
    else if (StringStartsWith(Flag, "months-ago=")) {
        Action = ACTION_MONTHS;
    }
    else if (StringStartsWith(Flag, "new-pay")) {
        Flags |= PAY;
    }
    else {
        /* TODO(lrak): error on unknown */
        fprintf(stderr, "WARNING: unknown flag --%s\n", Flag);
    }

    switch (Action) {
        s32 n;
    case ACTION_MONTHS:
        n = atoi(Flag + 11); /* skip past "months-agl=" */

        Flags |= ANY;
        Year  = Date->tm_year + 1900;
        Month = Date->tm_mon + 1;

        /* TODO(lrak): direct calculation, not loop */
        for (s32 i = 0; i < n; ++i) {
            --Month;
            if (Month == 0) {
                Month = 12;
                --Year; /* TODO(lrak): bounds check */
            }
        }

        break;

    case ACTION_YEARS:
        n = atoi(Flag + 10); /* skip past "years-ago=" */

        Flags |= ANY;
        Flags &= ~PAY;
        Year  = Date->tm_year + 1900;
        Month = Date->tm_mon + 1;

        Year -= n; /* TODO(lrak): bounds check */

        break;

    case ACTION_NONE: break;
                      InvalidDefaultCase
    }
}

s32 main(s32 Count, char **Args, char **Env) {
    s32 FirstArg = 2;
    char *NewArgs[(mm)Count + (mm)FirstArg]; /* C99 VLA. TODO(lrak): remove? */
    s32 NewCount = 0;
    char *Home = 0;
    char *Path = "Documents/Finances";

    for (char **Cur = Env; *Cur; ++Cur) {
        if (StringStartsWith(*Cur, "HOME=")) {
            Home = *Cur + 5;
        }
    }

    if (!Home) {
        fprintf(stderr, "ERROR: no HOME environment variable\n");
        return -1;
    }

    time_t UnixTime = time(0);
    Date = localtime(&UnixTime);

    Year  = (u32)Date->tm_year + 1900;
    Month = (u32)Date->tm_mon + 1;
    Day   = (u32)Date->tm_mday;

    for (s32 Idx = 1; Idx < Count; ++Idx)
    {
        char *Arg = Args[Idx];

        if (Arg[0] == '-') {
            if (Arg[1] == '-') {
                ProcessFlag(Arg + 2);
            }
            else {
                for (char *Cur = Arg + 1; *Cur; ++Cur) {
                    char *LongForm = 0;

                    switch (*Cur) {
                    case 'a': LongForm = "all";        break;
                    case 'p': LongForm = "new-pay";    break;
                    case 'e': LongForm = "edit";       break;
                    case 'n': LongForm = "new";        break;
                    case 'y': LongForm = "year";       break;
                    case 'm': LongForm = "month";      break;
                    case 'l': LongForm = "last-month"; break;
                    case 'L': LongForm = "last-year";  break;
                    case 'h': LongForm = "help";       break;
                    }

                    ProcessFlag(LongForm);
                }
            }
        }
        else {
            Assert(NewCount < Count);
            NewArgs[FirstArg + NewCount++] = Arg;
        }
    }

    NewArgs[FirstArg + NewCount] = 0;

    if (Flags & PAY) {
        /* TODO(lrak): 1024 characters should be enough for anyone */
        static char Buffer[1024];
        snprintf(Buffer, sizeof Buffer, "%s/%s/%u/pay", Home, Path, Year);

        DIR *Pay = opendir(Buffer);
        Assert(Pay); /* TODO(lrak): error checking */

        Number = 0;
        struct dirent *Dir;
        while ((Dir = readdir(Pay))) {
            if (StringStartsWith(Dir->d_name, ".")) continue;

            char *Tail;
            s64 Num = strtol(Dir->d_name, &Tail, 10);
            if (Num > (s64)Number && (!*Tail || *Tail == '.')) {
                Number = (u32)Num;
            }
        }

        closedir(Pay);

        if (Flags & NEW) {
            Number += 1;
        }

        if (Flags & EDIT) {
            snprintf(Buffer, sizeof Buffer, "%s/%s/%u/pay/%02d.tsv",
                            Home, Path, Year, Number);

            FILE *NewFile = fopen(Buffer, "w");
            Assert(NewFile);

            /* TODO(lrak): find a better templating mechanism (0000/pay/00.tsv?) */
            fprintf(NewFile, "=sum(A2:)\tMeans\tDescription\n\n");
            fprintf(NewFile, "#:width 16 20 64\n");
            fprintf(NewFile, "#:align r\n");
            fprintf(NewFile, "#:print head_sep\n");
            fprintf(NewFile, "# vim: noet ts=21 sw=21\n");

            fclose(NewFile);
        }

        snprintf(Buffer, sizeof Buffer, "%u/pay/%02d", Year, Number);

        Assert(FirstArg > 0);
        Assert(NewCount < Count);

        NewArgs[--FirstArg] = Buffer;
        ++NewCount;
    }

    if (!NewCount || Flags & ANY) {
        /* TODO(lrak): 128 characters should be enough for anyone */
        static char Buffer[128];

        if (Flags & YEAR) {
            snprintf(Buffer, sizeof Buffer, "%04d", Year);
        }
        else {
            snprintf(Buffer, sizeof Buffer, "%04d/%02d", Year, Month);
        }

        Assert(FirstArg > 0);
        Assert(NewCount < Count);

        NewArgs[--FirstArg] = Buffer;
        ++NewCount;
    }


    char FileName[1024]; /* TODO(lrak): 1024 characters should be enough for anyone */
    for (s32 Idx = 0; Idx < NewCount; ++Idx) {
        char *Arg = NewArgs[FirstArg + Idx];

        snprintf(FileName, sizeof FileName, "%s/%s/%s.tsv", Home, Path, Arg);
        if (Idx) printf("\n");

        pid_t Id = fork();
        if (Id == 0) {
            /* child, becomes some other program */
            if (Flags & EDIT) {
                execlp("nvim", Args[0], "+/^$/-1", FileName, NULL);
            }
            else {
                execlp("spreadsheet", Args[0], FileName, NULL);
            }

            /* unreachable */
            InvalidCodePath;
        }
        else {
            /* parent, waits for child to finish */

            if (waitpid(Id, 0, 0) < 0) {
                /* TODO(lrak): error handling */
                InvalidCodePath;
            }

        }
    }

    return 0;
}
