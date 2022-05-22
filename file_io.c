#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

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
void snippet9() {
	/** compare the len is always less than equals to SSIZE_MAX */

	// write the program where this len is greater than SSIZE_MAX and checking
	// the above condition is actually helpful, use dynamic space allocation
	// for it

	/** generally if you are reading something big, there's also option of
	 * realloc with some estimated size of slen (reads a chunk from whole data)
	 * for each read and the realloc for further read */
}

int main(int argc, char **argv)
{
	snippet8();
	return 0;
}
