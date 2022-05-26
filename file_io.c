#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/******************
** opening files **
******************/

// a simple open call for reading
void snippet1() {
	int fd;
	fd = open("<some_file>", O_RDONLY);
	if (fd == -1) {
		/** some error */
	}

	/** Various flags to open are
	 * O_RDONLY,O_WRONLY,O_RDWR,O_CREAT,O_APPEND,O_TRUNC,O_ASYNC,O_CLOEXEC,
	 * O_EXCL,O_DIRECT,O_DIRECTORY,O_LARGEFILE,O_NOATIME+,O_NOCTTY,
	 * O_NOFOLLOW,O_NONBLOCK,O_SYNC
	 * */
}

void snippet2() {
	int fd;
	fd = open("<some file>", O_WRONLY|O_TRUNC);
	if (fd == -1) {
		/** some error */
	}
	/** the above call to open will fail if file doesn't exist */
}

// permission for new files
void snippet3() {
	int fd;
	/** it is necessary (but not mandetory) to set mode arg, when using O_CREAT
	 * flag. The result could be enexpected though */
	fd = open("<some file>", O_WRONLY|O_CREAT|O_TRUNC,
			S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	/** or simply do
	 * fd = open("<some file>", O_WRONLY|O_CREAT|O_TRUNC, 0664);
	 * */
	if (fd == -1) {
		/** some error */
	}
	/** the actual perminssion-bits that hit the disk are determined by
	 * binary-ANDing the compliment umask bit->user's file creation mask bit
	 * (usual umask 022) with mode bits. Thus above 0664 becomes 0644 */
}

// the creat() function
void snippet4() {
	int fd;
	fd = creat("<some-file>", 0664);
	if (fd == -1) {
		/** some error */
	}
	/** the above creat() call is equal to
	  * fd = open("<some-file>", O_WRONLY|O_CREAT|O_TRUNC, 0664);
	  */
}

// on-most linux arch, creat() is a system call (now exist for
// backward-compatibility) even though it can be implemented in user space as
// simply (where it doesn't exist)
/* int creat(char *filename, int permission) {
	return open(filename, O_WRONLY|O_CREAT|O_TRUNC, permission);
} */


/***********************
** Reading via read() **
***********************/
void snippet5() {
	int fd;
	/** open file */
	unsigned long word;
	ssize_t nr;

	/** read a couple of bytes into 'word' from 'fd' */
	nr = read(fd, &word, sizeof (unsigned long));
	if (nr == -1) {
		/** some error */
	}

	/** above case doesn't handle all error from read, let's see some of the
	 * possibilities
	 * 1. call results value equal to len (3rd arg), this is correct.
	 * 2. less than len but greater than zero, read bytes are stored in buf
	 *       (2nd arg). This can occur either because of signal interruption
	 *       midway of read, more than zero but less than len bytes worth data
	 *       available,EOF reached before len bytes,error occured midway of
	 *       read. Reissue the read() with updated len and buf value.
	 * 3. call returns 0, which indicates EOF (nothing to do).
	 * 4. call blocks because no data currently available, willn't happen in
	 *       non-blocking mode
	 * 5. call returns -1 and errno set to EINTR. This indicates signal was
	 *       recieved before any bytes were read. Reissue the call
	 * 6. call returns -1 and errno set to EAGAIN. Indicates read would be
	 *       block because no data is currently available and call should be
	 *       reissued later. Happens only in non-blocking mode
	 * 7. call returns -1 and errno set to something other than EINTR an
	 *       EAGAIN. Indicates more serious error, simply Reissuing call is
	 *       unlikely to succeed
	 * */
}

// reading all the bytes
void snippet6() {
	const unsigned long BUFSIZE = 100;
	char buf[BUFSIZE];
	size_t len = sizeof (buf), offset = 0;
	ssize_t ret;
	int fd = STDIN_FILENO; // STDIN (=1)

	while (len != 0 && (ret = read(fd, buf + offset, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("read (snippet6)");
			break;
		}
		len -= ret;
		offset += ret;
		printf("ret: %zu\n", ret);
	}
	// printf("addr: %p, %p\n", buf, (buf + 10));
	printf("printing: %s\n", buf);

	/** the snippet handles all 5 conditions (not talking for 1 and 7) */
	/** some other returns values: EBADF,EFAULT,EINVAL,EIO */
}

// you can remove the offset logic and simply do (buf += ret) for dynamically
// allocated space (though it's not a good idea, consider below snippet)
void snippet7() {
	const unsigned long BUFSIZE = 50;
	char *buf = (char *)malloc(BUFSIZE * sizeof(char));
	size_t len = BUFSIZE - 1;
	int fd = STDIN_FILENO;
	ssize_t ret = 0;  // generally will tell EOF

	while (len != 0 && (ret = read(fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("read (snippet 7)");
			break;
		}
		len -= ret;
		buf += ret;
		printf("ret: %zu\n", ret);
	}
	printf("printing: %s\n", buf);

	if (buf != NULL) {
		free(buf); // problem here and for printing
	}
	/** so using a seperate offset indicator is probably better (whether space
	 * is indicated on stack or heap) */
}

// nonblocking reads
void snippet8() {
	/** file need to be opened with O_NONBLOCK for this. When performing
	 * nonblocking reads you must check for EAGAIN or confuse some serious
	 * error with mere lack of data */
	// int fd = STDIN_FILENO;
	const char *filename = "test.txt";
	const unsigned long BUFSIZE = 50;
	char buf[BUFSIZE];
	size_t len = BUFSIZE - 1, offset = 0;
	ssize_t ret = 0;
	int fd = 0;

	fd = open(filename, O_RDONLY|O_NONBLOCK);
	if (fd == -1) {
		perror("error opening file");
		return;
	}

start:
	ret = read(fd, buf, len);
	if (ret == -1) {
		if (errno == EINTR) {
			goto start;
		}
		if (errno == EAGAIN) {  // how to produce this ?
			printf("%m, reading again later");
		} else {
			/** some error */
		}
	}
	printf("%s, %zu\n", buf, ret);
}

/** size limits on read */
/** compare the len is always less than equals to SSIZE_MAX */

// write the program where this len is greater than SSIZE_MAX and checking
// the above condition is actually helpful, use dynamic space allocation
// for it

/** generally if you are reading something big, there's also option of
 * realloc with some estimated size of slen (reads a chunk from whole data)
 * for each read and the realloc for further read */


/*************************
** writing with write() **
*************************/

// sample example
void snippet9()
{
	const char *buf = "M ship is solid";
	size_t ret = 0;

	ret = write(STDOUT_FILENO, buf, strlen(buf));
	if (ret == -1) {
		/** some error */
	}

	/** but just like read this is not right, caller also need to check for the
	 * partial write */
}

// check for partial write
void snippet10()
{
	unsigned long word = 1720;
	size_t len = sizeof (word), ret = 0;

	ret = write(STDOUT_FILENO, &word, len);
	if (ret == -1) {
		/** some error, check errno */
	} else if (ret != len) {
		/** possible error, but errno not set */
	}
}

// using loop for write
void snippet11()
{
	/** write() system call is less likely to return partial write, than a
	 * read() system call to return partial read() */
	const char *buf = "This is a good string";
	size_t ret = 0, offset = 0;
	size_t len = strlen(buf);

	while (len != 0 && (ret = write(STDOUT_FILENO, buf + offset, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("write");
			break;
		}
		offset += ret;
		len -= ret;
	}

	/** For regular files, you don't need to perform write in loop, unlike the
	 * other filetypes like - socket. Another benefit of using loop is that a
	 * second call to write may reveal what caused the first call to perform
	 * only a partial write */
}

/** APPEND mode guarantees that any number of process always write to end the
 * same file (if the processes want to do so), without causing race condition
 * */

/** write() with count (3rd arg for write()) greater than SSIZE_MAX will return
 * some undefined result */

// Behavior of write()
/** Since write() calls are much faster (and thus the disparity in performance
 * of processor and hdd), kernel after performing few checks copies data to
 * write to a buffer. Later, in background, it calls all the "dirty buffers"
 * (the ones who have data which is new than what is on disk), sorts optimally
 * and writes them out to disk, which is known as "disk writeback".
 *
 * Possible issues with delayed write()
 * In case of system crash, there's a chance that data hasn't made its way to
 * disk.
 * Inability to enforce "write ordering", which isn't a problem most of the
 * time, but is still an issue.
 * Reporting of certain I/O Errors. Say, an I/O error occured during writeback
 * -say physical driver failure-can't be reported back to the process which
 * issued the write() call. Dirty buffers inside kernel are not associated with
 * process at all.
 *
 * Seeing all these potential issues, Kernel attempts to minimize the risk of
 * deffered writes and thus institutes a "maximum buffer age" to insure data is
 * written is timely manner (/proc/sys/vm/dirty_expire_centisecs)
 *
 * Forcing writeback or making all writes synchronous is shown later */


/*********************
** Synchronized I/O **
*********************/

/** In rare cases, where (because buffering provides significant performance
 * imporvement and issues of delayed writes should not be overstated)
 * application wants to control when data hits the disk, kernel provides some
 * options */

// fsync() and fdatasync()
void snippet12()
{
	int ret;
	int fd = STDOUT_FILENO;  // an fd must be open for writing
	ret = fsync(fd);  // returns 0 on success and -1 on failure (errno is updated)
	if (ret == -1) {
		/** error */
	}
	ret = fdatasync(fd);
	if (ret == -1) {
		/** error */
	}

	/** fsync() writes back both data and metadata (maybe also contained in
	 * dirty buffer), while fsync only flushes data and metadata required for
	 * process access for file in future */
}

void snippet13()
{
	int ret = 0;
	int fd = STDOUT_FILENO;

	if (fsync(fd) == -1) {
		/** we prefer fsync(), but let's try fdatasync() if fsync() failes,
		 * just in case */
		if (errno == EINVAL) {
			if (fdatasync(fd) == -1) {
				perror("fdatasync");
			}
		} else {
			perror("fsync");
		}
	}
}
// There's also "void sync(void)" which sync data and metadata for all dirty
// buffers (the buffer can still be on disk-cache though and not written on
// disk)


// the O_SYNC flag
void snippet14()
{
	int fd = -1;

	if ((fd = open("test.txt", O_WRONLY|O_SYNC)) == -1) {
		perror("open");
		EXIT_FAILURE;
	}
	/** using O_SYNC makes sure that writes are Synchronized just like the
	 * reads are (this generally shouldn't be used) */
}

// POSIX defines two other synchronized-I/O related open() flags: O_DSYNC & O_RSYNC
// O_DSYNC is just like using fdatasync() after each write request
// O_RSYNC is the synchronization of read requests as well as write() requests

// Direct I/O
/** using O_DIRECT flag with open() tells kernel to minimize the I/O
 * management. User-space will directly initiate I/O bypassing page-cache. */


/******************
** Closing files **
******************/

void snippet15()
{
	int fd = -1;  // intentional
	if (close(fd) == -1) {
		perror("close");
		EXIT_FAILURE;
	}
	/** Note that closing the files also have some side-effects, visit the
	 * section in book */

	/** close(fd) can return several errors like EBADF (invalid fd). Other than
	 * EBADF, fd is always closed and associated data structure is always
	 * freed. close(fd) doesn't returns EINTR
	 * */
}


/*************************
** Seeking with lseek() **
*************************/

// set file position to some value
void snippet16()
{
	int fd = -1; // intentional (use something valid in real use-case)
	off_t ret = 0;
	if ((ret = lseek(fd, 1825, SEEK_SET)) == (off_t)-1) {
		/** some error */
	} else {
		/** offset to 1825 for fd set successfully */
	}

	/** lseek() returns the resulting offset on success or -1 on failure with
	 * errno updated */

	/** Errors:
	 * EBADF: invalid fd
	 * EINVAL: seek origin is not SEEK_SET, SEEK_CUR or SEEK_END, or resulting
	 * file position would be negative (read more)
	 * EOVERFLOW: resulting file offset can't be contained in off_t
	 * ESPIPE: fd is associated with unseekable object, like pipe, FIFO or
	 * socket
	 * */
}

// set file position of fd to end
void snippet17()
{
	int fd = -1; // intentional
	off_t ret = 0;
	if ((ret = lseek(fd, 0, SEEK_END)) == (off_t)-1) {
		/** some error */
	}
	// file position set to last
}

// get current file offset
void snippet18()
{
	int fd = -1; // intentional
	off_t ret = 0;
	if ((ret = lseek(fd, 0, SEEK_CUR)) == (off_t)-1) {
		/** some error */
	}
	// you get current file position for fd, hurray !!!
}

// seeking past the end of file
void snippet19()
{
	int fd = -1; // intentional
	off_t ret = 0;
	if ((ret = lseek(fd, 1000, SEEK_END)) == (off_t)-1) {
		/** error */
	}
	/** a read() request to this newly created file position will return EOF
	 * and write() will create "holes" (empty space between will be padded with
	 * zeroes) */
}


/********************************
** Positional Reads and Writes **
********************************/

// using pread()
void snippet20()
{
	const char *filename = "./test.txt";
	char buf[50];
	int fd = -1;
	ssize_t ret = 0;
	size_t len = sizeof(buf);
	off_t pos = 15, offset = 0;

	if ((fd = open(filename, O_RDONLY)) == -1) {
		perror("read");
		EXIT_FAILURE;
	}
	while (len != 0 && (ret = pread(fd, buf, len, pos + offset)) != 0) {
		if (ret == -1) {
			if (ret == EINTR) {
				continue;
			}
			perror("pread");
			break;
		}
		offset += ret;
		len -= ret;
	}
	if (close(fd) == -1) {
		perror("close");
	}

	printf("%s", buf);

	/** pread() and pwrite() don't update current file position for the given
	 * fd. These are generally used to avoid potential race conditions which
	 * may occur with the use of lseek(), because threads share file table and
	 * current file position is stored in file table.
	 * For error, any valid read() & lseek() is possible for pread() and any
	 * valid write() & lseek() error is possible for pwrite() */
}

// using pwrite()
void snippet21()
{
	const char *filename = "./test.txt";
	const char *input = "and this is the lemon";
	size_t len = strlen(input);
	off_t pos = 100, offset = 0;
	int fd = -1;
	ssize_t ret = 0;

	if ((fd = open(filename, O_WRONLY)) == -1) {
		perror("open");
		EXIT_FAILURE;
	}

	while (len != 0 && (ret = pwrite(fd, input, len, pos + offset)) != -1) {
		if (ret == -1) {
			if (ret == EINTR) {
				continue;
			}
			perror("pwrite");
			break;
		}
		offset += ret;
		len -= ret;
	}
	if (close(fd) == -1) {
		perror("close");
	}
}

// using ftruncate() for truncating files
void snippet22()
{
	const char *filename = "./test.txt";
	int fd = -1;

	if ((fd = open(filename, O_WRONLY)) == -1) {
		perror("read");
		EXIT_FAILURE;
	}

	if (ftruncate(fd, 85) == -1) {  // will truncate the file size to 85 bytes len
		/** check updated errno if needed */
		perror("ftruncate");
		EXIT_FAILURE;
	}

	/** truncate() and ftruncate() don't change the current file position */
}

// using truncate() for truncating files
void snippet23()
{
	const char *filename = "./test.txt";

	// if the len provided is more than the current len, it'll create the holes
	// (sparse file)
	if (truncate(filename, 40) == -1) {
		/** check updated errno if needed */
		perror("truncate");
		EXIT_FAILURE;
	}
}

int main(int argc, char **argv)
{
	snippet23();
	return 0;
}
