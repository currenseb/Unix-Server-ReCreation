# Unix Server Re-Creation

This assignment implements a small, persistent filesystem shell in C. It
models files and directories with an inode table stored in `inodes_list`, and
stores each inode's contents in a file named after its inode number.

## What it does

- Opens a filesystem directory supplied on the command line (the included
  `fs/` directory is an example filesystem).
- Loads the persistent inode table, creating a root inode named `demo` when
  no table exists.
- Provides these shell commands:
  - `ls` — list entries in the current directory.
  - `cd <directory>`, `cd ..`, and `cd` — navigate directories; `cd` returns
    to the root.
  - `mkdir <name>` — create a directory with `.` and `..` entries.
  - `touch <name>` — create a file.
  - `rm <name>` — remove a file (directories cannot be removed).
  - `exit` — save the inode table and quit.
- Persists changes to `inodes_list` before exiting, including removals made in
  a child process.

## Build and run

```sh
gcc -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L server.c -o server
./server fs
```

The `fs/` directory is the only checked-in filesystem image; it contains the
sample inode table and inode-content files used by the program.
