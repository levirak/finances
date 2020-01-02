#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

static u32 Flags = 0;

static struct tm *Date = 0;
static s32 Year  = 1970;
static s32 Month = 1;
static s32 Day   = 1;

enum Flags {
    ANY     = 1 << 0,
    YEAR    = 1 << 1,
    EDIT    = 1 << 2,
    NEW_PAY = 1 << 3,
    PAY     = 1 << 4,
    CREATE  = 1 << 5,
};

static char *ProgramName;



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
    Assert(Flag);

    enum {
        ACTION_NONE,
        ACTION_MONTHS,
        ACTION_YEARS,
    } Action = ACTION_NONE;

    if (StringsAreEqual(Flag, "all")) {
        Flags |= YEAR;
        Year = 0;
    }
    else if (StringsAreEqual(Flag, "edit")) {
        Flags |= EDIT;
    }
    else if (StringsAreEqual(Flag, "new-pay")) {
        Flags |= PAY | NEW_PAY;
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
    else if (StringsAreEqual(Flag, "pay")) {
        Flags |= PAY;
    }
    else if (StringsAreEqual(Flag, "create")) {
        Flags |= CREATE;
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
    else {
        /* TODO(lrak): error on unknown */
        fprintf(stderr, "WARNING: unknown flag --%s\n", Flag);
    }

    switch (Action) {
        s32 n;
    case ACTION_MONTHS:
        n = atoi(Flag + 11); /* skip past "months-ago=" */

        Flags |= ANY;
        Flags &= ~PAY;
        Year  = Date->tm_year + 1900;
        Month = Date->tm_mon + 1;

        /* TODO(lrak): direct calculation, not loop */
        for (s32 i = 0; i < n; ++i) {
            --Month;
            if (Month == 0) {
                Month = 12;
                --Year;
            }
        }

        break;

    case ACTION_YEARS:
        n = atoi(Flag + 10); /* skip past "years-ago=" */

        Flags |= ANY | YEAR;
        Flags &= ~PAY;
        Year  = Date->tm_year + 1900;
        Month = Date->tm_mon + 1;

        Year -= n;

        break;

    case ACTION_NONE: break;
    InvalidDefaultCase
    }
}

static inline
bool EnsureDirectory(char *Path) {
    bool Success = false;

    /* TODO(lrak): don't do a fork */
    pid_t Id = fork();
    if (Id == 0) {
        /* child, becomes some other program */

        execlp("mkdir", ProgramName, "-p", Path, NULL);

        Unreachable;
    }
    else {
        /* parent, waits for child to finish */

        s32 ChildStatus = -1;
        if (waitpid(Id, &ChildStatus, 0) < 0) {
            /* TODO(lrak): error handling */
            NotImplemented;
        }

        Success = (ChildStatus == 0);
    }

    return Success;
}

static
void Delegate(char *Path, bool Edit)
{
    static s32 NumPrinted = 0;

    /* put a space between multiple calls */
    if (NumPrinted++) printf("\n");

    pid_t Id = fork();
    if (Id == 0) {
        /* child, becomes some other program */
        if (Edit) {
            execlp("nvim", ProgramName, "+/^$/-1", Path, NULL);
        }
        else {
            execlp("spreadsheet", ProgramName, Path, NULL);
        }

        Unreachable;
    }
    else {
        /* parent, waits for child to finish */

        if (waitpid(Id, 0, 0) < 0) {
            NotImplemented;
        }
    }
}

s32 main(s32 ArgCount, char **Args, char **Env)
{
    char *Home = 0;
    char *Finances = "Documents/Finances";

    /* TODO(lrak): 1024 characters should be enough for anyone */
    static char AbsolutePath[1024];
    char *BufferEnd = AbsolutePath + sizeof AbsolutePath;

    ProgramName = Args[0];

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

    s32 ModifiedArgCount = 1;
    for (s32 Index = 1; Index < ArgCount; ++Index)
    {
        static bool Armed = true;
        char *Arg = Args[Index];

        if (Armed && Arg[0] == '-') {
            if (Arg[1] == '-') {
                if (!Arg[2]) {
                    /* got argument "--", disarm the flag parser */
                    Armed = false;
                }
                else {
                    ProcessFlag(Arg + 2);
                }
            }
            else for (char *Cur = Arg + 1; *Cur; ++Cur) {
                switch (*Cur) {
                case 'a': ProcessFlag("all");        break;
                case 'p': ProcessFlag("pay");        break;
                case 'e': ProcessFlag("edit");       break;
                case 'n': ProcessFlag("new-pay");    break;
                case 'y': ProcessFlag("year");       break;
                case 'm': ProcessFlag("month");      break;
                case 'l': ProcessFlag("last-month"); break;
                case 'L': ProcessFlag("last-year");  break;
                case 'h': ProcessFlag("help");       break;
                case 'c': ProcessFlag("create");     break;

                default:
                    /* TODO(lrak): error on unknown */
                    fprintf(stderr, "WARNING: unrecognized flag -%c\n", *Cur);
                    break;
                }
            }

        }
        else {
            Assert(ModifiedArgCount < ArgCount);
            Args[ModifiedArgCount++] = Arg;
        }
    }

    char *RelativePath = AbsolutePath +
        snprintf(AbsolutePath, sizeof AbsolutePath, "%s/%s/", Home, Finances);
    Assert(RelativePath < BufferEnd);

    if (Flags & ANY || (ModifiedArgCount == 1 && !(Flags & PAY))) {
        if (Flags & YEAR) {
            snprintf(RelativePath, RelativePath - BufferEnd, "%04d.tsv", Year);
        }
        else {
            snprintf(RelativePath, RelativePath - BufferEnd,
                     "%04d/%02d.tsv", Year, Month);
        }

        if (Flags & EDIT) {
            /* TODO(lrak): ensure file exists. Create it if it doesn't */
        }

        Delegate(AbsolutePath, Flags & EDIT);
    }

    if (Flags & PAY) {
        DIR *Pay;

        char *FileComponent = RelativePath +
            snprintf(RelativePath, RelativePath - BufferEnd, "%04d/pay", Year);
        Assert(FileComponent < BufferEnd);

        if (!EnsureDirectory(AbsolutePath)) {
            NotImplemented;
        }
        else if (!(Pay = opendir(AbsolutePath))) {
            NotImplemented;
        }
        else {
            struct dirent *Dir;
            u32 Number = 0;

            while ((Dir = readdir(Pay))) {
                if (StringStartsWith(Dir->d_name, ".")) continue;

                char *Tail;
                s64 ThisNumber = strtol(Dir->d_name, &Tail, 10);
                if (ThisNumber > (s64)Number && (!*Tail || *Tail == '.')) {
                    Number = (u32)ThisNumber;
                }
            }

            closedir(Pay);

            if (Flags & NEW_PAY) {
                Number += 1;
            }

            snprintf(FileComponent, FileComponent - BufferEnd,
                     "/%02d.tsv", Number);

            if (Flags & NEW_PAY) {
                FILE *NewFile = fopen(AbsolutePath, "w");
                Assert(NewFile);

                /* TODO(lrak): find a better templating mechanism (0000/pay/00.tsv?) */
                fprintf(NewFile,
                        "=sum(A2:)\tMeans\tDescription\n"
                        "\n"
                        "#:width 16 20 64\n"
                        "#:align r\n"
                        "#:print head_sep\n"
                        "# vim: noet ts=21 sw=21\n");

                fclose(NewFile);
            }

            Delegate(AbsolutePath, Flags & NEW_PAY);
        }
    }

    for (s32 Index = 1; Index < ModifiedArgCount; ++Index) {
        char *Arg = Args[Index];

        snprintf(RelativePath, RelativePath - BufferEnd, "%s.tsv", Arg);

        if (Flags & CREATE) {
            /* TODO(lrak): try to create this file if it doesn't exist */
        }

        Delegate(AbsolutePath, Flags & EDIT);
    }

    return 0;
}
