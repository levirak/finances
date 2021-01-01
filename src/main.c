#include "main.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MaxPath 1024 /* TODO(lrak): 1024 characters should be enough for anyone */

static struct flags {
    bool Any: 1; /* indicates that "any records were specified by flags" */

    enum { /* record type to access */
        RECORD_MONTH = 0, /* default */
        RECORD_YEAR,
        RECORD_PAY,
        RECORD_TEMPLATE,
    } Record: 2;

    bool Edit: 1;     /* edit any records specified*/
    bool NewPay: 1;   /* make a new payment record */
    bool Pay: 1;      /* access the payment record */
    bool Create: 1;   /* arg specified records should be created */
    bool Template: 1; /* access the template record */
} Flags = { 0 };

static_assert(sizeof Flags == sizeof (s32), "Flags struct is larger than expected");

static char *ProgramName = "finances";

static struct tm *Date = 0;
static s32 Year  = 1970;
static s32 Month = 1;
static s32 Day   = 1;

static
bool StringsAreEqual(char *A, char *B) {
    while (*A && *A == *B) {
        ++A;
        ++B;
    }

    return *A == *B;
}

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
        ACTION_TEMPLATE,
    } Action = ACTION_NONE;

    if (StringsAreEqual(Flag, "all")) {
        Flags.Any = 1;
        Flags.Record = RECORD_YEAR;
        Year = 0;
    }
    else if (StringsAreEqual(Flag, "edit")) {
        Flags.Edit = 1;
    }
    else if (StringsAreEqual(Flag, "new-pay")) {
        Flags.Record = RECORD_PAY;
        Flags.NewPay = 1;
    }
    else if (StringsAreEqual(Flag, "help")) {
        printf("I.O.U. one help page. Sorry.\n");
        /* TODO(lrak): print help */
    }
    else if (StringsAreEqual(Flag, "year")) {
        Flags.Any = 1;
        Flags.Record = RECORD_YEAR;
    }
    else if (StringsAreEqual(Flag, "month")) {
        Flags.Any = 1;
        Flags.Record = RECORD_MONTH;

        if (!Year) {
            Year = Date->tm_year + 1900;
        }
    }
    else if (StringsAreEqual(Flag, "pay")) {
        Flags.Any = 1;
        Flags.Record = RECORD_PAY;
    }
    else if (StringsAreEqual(Flag, "create")) {
        Flags.Create = 1;
    }
    else if (StringsAreEqual(Flag, "last-month")) {
        Flag = "months-ago=1";
        Action = ACTION_MONTHS;
    }
    else if (StringsAreEqual(Flag, "last-year")) {
        Flag = "years-ago=1";
        Action = ACTION_YEARS;
    }
    else if (StringsAreEqual(Flag, "template")) {
        Action = ACTION_TEMPLATE;
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

        Flags.Any = 1;
        Flags.Record = RECORD_MONTH;
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

        Flags.Any = 1;
        Flags.Record = RECORD_YEAR;
        Year  = Date->tm_year + 1900;
        Month = Date->tm_mon + 1;

        Year -= n;

        break;

    case ACTION_TEMPLATE:
        Flags.Any = 1;
        Flags.Record = RECORD_TEMPLATE;

        break;

    case ACTION_NONE: break;
    InvalidDefaultCase;
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

static
void WritePath(char *Buffer, mm BufSize, char *Root, s32 Year, s32 Month)
{
    if (Flags.Record == RECORD_YEAR) {
        snprintf(Buffer, BufSize, "%s/%04d.tsv", Root, Year);
    }
    else {
        snprintf(Buffer, BufSize, "%s/%04d/%02d.tsv", Root, Year, Month);
    }
}

static
void WritePayDirPath(char *Buffer, mm BufSize, char *Root, s32 Year)
{
    snprintf(Buffer, BufSize, "%s/%04d/pay", Root, Year);
}

static
void WritePayPath(char *Buffer, mm BufSize, char *Root, s32 Year, s32 Number)
{
    snprintf(Buffer, BufSize, "%s/%04d/pay/%02d.tsv", Root, Year, Number);
}

/* NOTE: assums that `Out` is at least as big as `Path` */
static
char *DirName(char *Path, char *Out)
{
    strcpy(Out, Path);

    char *Last = 0;

    for (char *Cur = Out; *Cur; ++Cur) {
        if (*Cur == '/') Last = Cur;
    }

    if (!Last) {
        /* there is no directory component, use CWD */
        strcpy(Out, ".");
    }
    else if (Last == Out) {
        /* this file lives in the root directory */
        strcpy(Out, "/");
    }
    else {
        /* truncate the string at the last dirsep */
        *Last = 0;
    }

    return Out;
}

static
void Copy(char *SourcePath, char *DestinationPath) {
    /* TODO(lrak): return value */
    fd Template = open(SourcePath, O_RDONLY);

    if (Template < 0) {
        fprintf(stderr, "ERROR: Could not open template %s\n",
                SourcePath);
    }
    else {
        struct stat Stat;
        fstat(Template, &Stat);
        mode_t Mode = Stat.st_mode;

        char DestDir[MaxPath];
        EnsureDirectory(DirName(DestinationPath, DestDir));

        /* get ahold of our new file */
        fd Out = open(DestinationPath, O_WRONLY | O_CREAT | O_EXCL, Mode);

        if (Out < 0) {
            /* TODO(lrak): error handling */
            NotImplemented;
        }
        else {
            char Buffer[1024];

            smm BytesRead = 0;
            while ((BytesRead = read(Template, Buffer, sizeof Buffer)) > 0) {
                smm BytesWritten = write(Out, Buffer, BytesRead);

                if (BytesWritten != BytesRead) {
                    /* TODO(lrak): error handling */
                    NotImplemented;
                }
            }

            CheckEq(close(Out), 0);
        }

        CheckEq(close(Template), 0);
    }
}

s32 main(s32 ArgCount, char **Args, char **Env)
{
    char *Home = 0;

    static char RootPath[512] = "";
    static char SourcePath[MaxPath];
    static char DestinationPath[MaxPath];

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

    char *Finances = "Documents/Finances";
    snprintf(RootPath, sizeof RootPath, "%s/%s", Home, Finances);



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
                case 't': ProcessFlag("template");   break;

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

    if (Flags.Any || ModifiedArgCount == 1) switch (Flags.Record) {
    case RECORD_MONTH:
    case RECORD_YEAR: {
        WritePath(DestinationPath, MaxPath, RootPath, Year, Month);

        if (Flags.Edit || Flags.Create) {
            if ((access(DestinationPath, F_OK) == -1)) {
                WritePath(SourcePath, MaxPath, RootPath, 0, Month);
                Copy(SourcePath, DestinationPath);
            }
        }

        Delegate(DestinationPath, Flags.Edit);
    } break;

    case RECORD_TEMPLATE: {
        WritePath(DestinationPath, MaxPath, RootPath, 0, Month);
        Delegate(DestinationPath, Flags.Edit);
    } break;

    case RECORD_PAY: {
        DIR *Pay;

        WritePayDirPath(DestinationPath, MaxPath, RootPath, Year);

        if (!EnsureDirectory(DestinationPath)) {
            NotImplemented;
        }
        else if (!(Pay = opendir(DestinationPath))) {
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

            if (Flags.NewPay) {
                Number += 1;
            }

            WritePayPath(DestinationPath, MaxPath, RootPath, Year, Number);

            if (Flags.Edit || Flags.Create || Flags.NewPay) {
                if ((access(DestinationPath, F_OK) == -1)) {
                    WritePayPath(SourcePath, MaxPath, RootPath, 0, 0);
                    Copy(SourcePath, DestinationPath);
                }
            }

            Delegate(DestinationPath, Flags.Edit);
        }
    } break;

    InvalidDefaultCase;
    }


    for (s32 Index = 1; Index < ModifiedArgCount; ++Index) {
        char *Arg = Args[Index];

        snprintf(DestinationPath, MaxPath, "%s/%s.tsv", RootPath, Arg);

        if (Flags.Create) {
            /* TODO(lrak): try to create this file if it doesn't exist */
        }

        Delegate(DestinationPath, Flags.Edit);
    }

    return 0;
}
