#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// the variable errno is declared in <error.h> as:
// extern int errno;

// for a few functions the entire range of return type is legal return value.
// In those cases, errno must be zeroed and checked afterwards (these functions
// promise to only return non-zero errno on actual error)
void converting_strtoul(const char* string, const int base)
{
	char *endptr;
	unsigned long val;

	errno = 0;  /** to distinguish success/failure after call */
	val = strtoul(string, &endptr, base);

	/** check for possible errors */
	if (errno != 0) {
		// perror("strtoul");  // same as below
		// fprintf(stderr, "strtoul: %s\n", strerror(errno));
		fprintf(stderr, "strtoul: %m\n");  // same as above, check man fprintf
		exit(EXIT_FAILURE);
	}

	if (endptr == string) {
		fprintf(stderr, "No digits were found");
		exit(EXIT_FAILURE);
	}

	/** if we got here, strtoul was sucess */
	printf("strtoul returned: %lu\n", val);

	if (*endptr != '\0')  /** not necessarily an error */
		printf("Further characters after number: \"%s\"", endptr);

	exit(EXIT_SUCCESS);
}

// A common mistake in checking errno is to forget that any library or system
// call can modify it. For example, this code is buggy:
void wrong_errno()
{
	int fd = 1;
	if (fsync(fd) == -1) {
		fprintf(stderr, "fsync failed\n");
		if (errno == EIO)
			fprintf(stderr, "I/O error on %d\n", fd);
	}
}

// if you need to preserve the value of errno across function invocation, save it
void save_errno() {
	int fd = 1;
	if (fsync(fd) == -1) {
		int err = errno;
		fprintf(stderr, "fsync failed\n");
		if (err == EIO) {
			/** if the error is I/O related, jump ship */
			fprintf(stderr, "I/O error on %d\n", fd);
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv)
{

	if (argc < 2) {
		fprintf(stderr, "usage: %s str [base]\n", *argv);
		exit(EXIT_FAILURE);
	}

	// converting_strtoul(argv[1], (argc > 2 ? atoi(argv[2]) : 0));

	// wrong_errno();

	// save_errno();

	return 0;
}
