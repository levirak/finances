#include "main.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

static u32 Flags = 0;
static u32 Year  = 1970;
static u32 Month = 1;
static u32 Day   = 1;

enum Flags {
    YEAR = 0x1 << 0,
    EDIT = 0x1 << 1,
    ALL  = 0x1 << 2,
};

static
bool StringsAreEqual(char *A, char *B) {
    while (*A && *A == *B) {
        ++A;
        ++B;
    }

    return *A == *B;
}

static
mm BufferString(char *Buffer, mm Size, char *Str) {
    char *End = Buffer + Size - 1;
    char *Cur = Str;

    while (*Cur && Buffer < End) {
        *Buffer++ = *Cur++;
    }

    *Buffer = 0;

    return (mm)(Cur - Str);
}

#if 0
static
bool StringLike(char *A, char *B) {
    while (*A && *B != '*' && *A == *B) {
        ++A;
        ++B;
    }

    return *A == *B || *B == '*';
}
#endif

static
void ProcessFlag(char *Arg) {
    Assert(Arg);

    if (StringsAreEqual(Arg, "all")) {
        Flags |= ALL;
    }
    else if (StringsAreEqual(Arg, "edit")) {
        Flags |= EDIT;
    }
    else if (StringsAreEqual(Arg, "year")) {
        Flags |= YEAR;
    }
    else if (StringsAreEqual(Arg, "month")) {
        Flags &= ~YEAR;
    }
    else if (StringsAreEqual(Arg, "help")) {
        /* TODO: print help */
    }
    else if (StringsAreEqual(Arg, "last-month")) {
        --Month;
        if (Month == 0) {
            Month = 12;
            --Year; /* TODO: bounds check */
        }
        Flags &= ~YEAR;
    }
    else if (StringsAreEqual(Arg, "last-year")) {
        --Year; /* TODO: bounds check */
        Flags |= YEAR;
    }
    //else if (StringLike(Arg, "months-ago=*")) { } /* TODO: this */
    //else if (StringLike(Arg, "years-ago=*")) { } /* TODO: this */
    else {
        /* TODO: error on unknown */
    }
}

#define BASE_PATH "/home/levi/Documents/Finances"

s32 main(s32 Count, char **Args) {
    u32 FirstArg = 1;
    char *NewArgs[(mm)Count + (mm)FirstArg];
    s32 NewCount = 0;

    time_t UnixTime = time(0);
    struct tm *Time = localtime(&UnixTime);

    Year  = (u32)Time->tm_year + 1900;
    Month = (u32)Time->tm_mon + 1;
    Day   = (u32)Time->tm_mday;

    printf("Date: %04u/%02u/%02d\n", Year, Month, Day);
    printf("Today: %04u/%02u.tsv\n", Year, Month);

    for (s32 Idx = 1; Idx < Count; ++Idx)
    {
        char *Arg = Args[Idx];

        if (Arg[0] == '-') {
            if (Arg[1] == '-') {
                ProcessFlag(Arg + 2);
            }
            else {
                for (char *Cur = Arg + 1; *Cur; ++Cur) {
                    char Buffer[2] = { *Cur, 0 };
                    char *LongForm = Buffer;

                    switch (*Cur) {
                    case 'a': LongForm = "all";        break;
                    case 'e': LongForm = "edit";       break;
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

    NewArgs[NewCount] = 0;

    printf("Effective: %04u/%02u.tsv\n", Year, Month);

    char Buffer[128];

    if (Flags & ALL) {
        BufferString(Buffer, sizeof Buffer, "0000");
    }
    else if (Flags & YEAR) {
        snprintf(Buffer, sizeof Buffer, "%04d", Year);
    }
    else {
        snprintf(Buffer, sizeof Buffer, "%04d/%02d", Year, Month);
    }

    Assert(FirstArg > 0);
    Assert(NewCount < Count);

    NewArgs[--FirstArg] = Buffer;
    ++NewCount;

    char FileName[1024];
    for (s32 Idx = 0; Idx < NewCount; ++Idx) {
        snprintf(FileName, sizeof FileName, BASE_PATH "/%s.tsv", NewArgs[Idx]);

        if (Flags & EDIT) {
            execlp("nvim", "nvim", "+/^$/-1", FileName, NULL);
        }
        else {
            execlp("spreadsheet", "spreadsheet", FileName, NULL);
        }
    }

#if 0
    s32 Error = execlp("nvim", "nvim", NULL);
    if (Error) {
        fprintf(stderr, "Failed execp\n");
    }
#endif

    return -1; /* We should never make it here */
}
