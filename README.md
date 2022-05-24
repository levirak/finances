`finances` is a simple tool for automating my workflow for recording my
personal finances.

Requires the following executables to be in the path:
* `mkdir`
* `git`
* `nvim`
* [`tabulate`](https://github.com/levirak/tabulate).

# Motivation

Now that I've got all my income and expenses data in lightning fast text files,
I've got a new problem: it takes way to long for me to `cd` over to them and
edit them. On top of that, I've got to remember what day it is, and if this is
the first time I'm adding something to this file this month I've got a bunch of
recurring expenses I need to fill in.

# Solution

It didn't take long for me to arrive at a workflow that was so predictable I
could automate it, so I did. From here on my monthly finances sheets would live
in `~/Documents/Finances`, and structured so that a month's file would live at
`YYYY/MM.tsv` and the year's summary would live at `YYYY.tsv`. If I went to
edit a month, but it's file didn't exist, it would automatically copy in the
template that lived at `0000/MM.tsv`. Most of this carried over seamlessly from
how I had been doing things, but this program formalized it.

# Language Choice

C is my favorite language because it doesn't do anything for me. I enjoy
working with it because it necessitates me to engage with systems that are
usually abstracted away in other languages, and in turn I find that this helps
me appreciate those other languages more. For similar reasons I have chosen for
my build system to be a hand written makefile.

I usually write for the most recent version of GNU C because I usually have a
newer `gcc` installed. On top of that, I write in a personal dialect of C that
I don't let get too alien. As a taste of that dialect, I use `s32` in place of
`int` and `u64` in place of `unsigned long`, and I have added the keyword
`InvalidCodePath` as a debug trap. All additions I write with are placed in
[`main.h`](src/main.h), which I include in every source file.
