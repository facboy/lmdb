/* mdb_load.c - memory-mapped database load tool */
/*
 * Copyright 2011-2014 Howard Chu, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "lmdb.h"

#define PRINT	1
#define NOHDR	2
static int mode;

static char *subname = NULL;

static size_t lineno;
static int version;

static int flags;

static char *prog;

static int eof;

static MDB_val kbuf, dbuf;

#define STRLENOF(s)	(sizeof(s)-1)

typedef struct flagbit {
	int bit;
	char *name;
	int len;
} flagbit;

#define S(s)	s, STRLENOF(s)

flagbit dbflags[] = {
	{ MDB_REVERSEKEY, S("reversekey") },
	{ MDB_DUPSORT, S("dupsort") },
	{ MDB_INTEGERKEY, S("integerkey") },
	{ MDB_DUPFIXED, S("dupfixed") },
	{ MDB_INTEGERDUP, S("integerdup") },
	{ MDB_REVERSEDUP, S("reversedup") },
	{ 0, NULL, 0 }
};

static const char hexc[] = "0123456789abcdef";

static void readhdr()
{
	char *ptr;

	while (fgets(dbuf.mv_data, dbuf.mv_size, stdin) != NULL) {
		lineno++;
		if (!strncmp(dbuf.mv_data, "VERSION=", STRLENOF("VERSION="))) {
			version=atoi(dbuf.mv_data+STRLENOF("VERSION="));
			if (version > 3) {
				fprintf(stderr, "%s: line %zd: unsupported VERSION %d\n",
					prog, lineno, version);
				exit(EXIT_FAILURE);
			}
		} else if (!strncmp(dbuf.mv_data, "HEADER=END", STRLENOF("HEADER=END"))) {
			break;
		} else if (!strncmp(dbuf.mv_data, "format=", STRLENOF("format="))) {
			if (!strncmp(dbuf.mv_data+STRLENOF("FORMAT="), "print", STRLENOF("print")))
				mode |= PRINT;
			else if (strncmp(dbuf.mv_data+STRLENOF("FORMAT="), "bytevalue", STRLENOF("bytevalue"))) {
				fprintf(stderr, "%s: line %zd: unsupported FORMAT %s\n",
					prog, lineno, (char *)dbuf.mv_data+STRLENOF("FORMAT="));
				exit(EXIT_FAILURE);
			}
		} else if (!strncmp(dbuf.mv_data, "database=", STRLENOF("database="))) {
			ptr = memchr(dbuf.mv_data, '\n', dbuf.mv_size);
			if (ptr) *ptr = '\0';
			if (subname) free(subname);
			subname = strdup(dbuf.mv_data+STRLENOF("database="));
		} else if (!strncmp(dbuf.mv_data, "type=", STRLENOF("type="))) {
			if (strncmp(dbuf.mv_data+STRLENOF("type="), "btree", STRLENOF("btree")))  {
				fprintf(stderr, "%s: line %zd: unsupported type %s\n",
					prog, lineno, (char *)dbuf.mv_data+STRLENOF("type="));
				exit(EXIT_FAILURE);
			}
		} else {
			int i;
			for (i=0; dbflags[i].bit; i++) {
				if (!strncmp(dbuf.mv_data, dbflags[i].name, dbflags[i].len) &&
					((char *)dbuf.mv_data)[dbflags[i].len] == '=') {
					flags |= dbflags[i].bit;
					break;
				}
			}
			if (!dbflags[i].bit) {
				ptr = memchr(dbuf.mv_data, '=', dbuf.mv_size);
				if (!ptr) {
					fprintf(stderr, "%s: line %zd: unexpected format\n",
						prog, lineno);
					exit(EXIT_FAILURE);
				} else {
					*ptr = '\0';
					fprintf(stderr, "%s: line %zd: unrecognized keyword ignored: %s\n",
						prog, lineno, (char *)dbuf.mv_data);
				}
			}
		}
	}
}

static void badend()
{
	fprintf(stderr, "%s: line %zd: unexpected end of input\n",
		prog, lineno);
}

static int unhex(unsigned char *c2)
{
	int x, c;
	x = *c2++ & 0x4f;
	if (x & 0x40)
		x -= 54;
	c = x << 4;
	x = *c2 & 0x4f;
	if (x & 0x40)
		x -= 54;
	c |= x;
	return c;
}

static int readline(MDB_val *out, MDB_val *buf)
{
	unsigned char *c1, *c2, *end;
	size_t len;
	int c;

	if (!(mode & NOHDR)) {
		c = fgetc(stdin);
		if (c == EOF) {
			eof = 1;
			return EOF;
		}
		if (c != ' ') {
			if (fgets(buf->mv_data, buf->mv_size, stdin) == NULL) {
badend:
				eof = 1;
				badend();
				return EOF;
			}
			if (c == 'D' && !strncmp(buf->mv_data, "ATA=END", STRLENOF("ATA=END")))
				return EOF;
			goto badend;
		}
	}
	if (fgets(buf->mv_data, buf->mv_size, stdin) == NULL) {
		eof = 1;
		return EOF;
	}
	lineno++;

	c1 = buf->mv_data;
	len = strlen((char *)c1);

	/* Is buffer too short? */
	while (c1[len-1] != '\n') {
		buf->mv_data = realloc(buf->mv_data, buf->mv_size*2);
		if (!buf->mv_data) {
			eof = 1;
			fprintf(stderr, "%s: line %zd: out of memory, line too long\n",
				prog, lineno);
			return EOF;
		}
		c1 = buf->mv_data;
		c1 += buf->mv_size;
		if (fgets((char *)c1, buf->mv_size, stdin) == NULL) {
			eof = 1;
			badend();
			return EOF;
		}
		buf->mv_size *= 2;
		len = strlen((char *)c1);
	}
	c1 = c2 = buf->mv_data;
	len = strlen((char *)c1);
	c1[--len] = '\0';
	end = c1 + len;

	if (mode & PRINT) {
		while (c2 < end) {
			if (*c2 == '\\') {
				if (c2[1] == '\\') {
					c1++; c2 += 2;
				} else {
					if (c2+3 >= end || !isxdigit(c2[1]) || !isxdigit(c2[2])) {
						eof = 1;
						badend();
						return EOF;
					}
					*c1++ = unhex(++c2);
					c2 += 2;
				}
			} else {
				c1++; c2++;
			}
		}
	} else {
		/* odd length not allowed */
		if (len & 1) {
			eof = 1;
			badend();
			return EOF;
		}
		while (c2 < end) {
			if (!isxdigit(*c2) || !isxdigit(c2[1])) {
				eof = 1;
				badend();
				return EOF;
			}
			*c1++ = unhex(c2);
			c2 += 2;
		}
	}
	c2 = out->mv_data = buf->mv_data;
	out->mv_size = c1 - c2;

	return 0;
}

static void usage()
{
	fprintf(stderr, "usage: %s dbpath [-V] [-f input] [-n] [-s name] [-N] [-T]\n", prog);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i, rc;
	MDB_env *env;
	MDB_txn *txn;
	MDB_cursor *mc;
	MDB_dbi dbi;
	char *envname;
	int envflags = 0, putflags = 0;

	prog = argv[0];

	if (argc < 2) {
		usage(prog);
	}

	/* -f: load file instead of stdin
	 * -n: use NOSUBDIR flag on env_open
	 * -s: load into named subDB
	 * -N: use NOOVERWRITE on puts
	 * -T: read plaintext
	 * -V: print version and exit
	 */
	while ((i = getopt(argc, argv, "f:ns:NTV")) != EOF) {
		switch(i) {
		case 'V':
			printf("%s\n", MDB_VERSION_STRING);
			exit(0);
			break;
		case 'f':
			if (freopen(optarg, "r", stdin) == NULL) {
				fprintf(stderr, "%s: %s: reopen: %s\n",
					prog, optarg, strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		case 'n':
			envflags |= MDB_NOSUBDIR;
			break;
		case 's':
			subname = strdup(optarg);
			break;
		case 'N':
			putflags = MDB_NOOVERWRITE|MDB_NODUPDATA;
			break;
		case 'T':
			mode |= NOHDR;
			break;
		default:
			usage(prog);
		}
	}

	if (optind != argc - 1)
		usage(prog);

	envname = argv[optind];
	rc = mdb_env_create(&env);

	if (subname) {
		mdb_env_set_maxdbs(env, 2);
	}

	rc = mdb_env_open(env, envname, envflags, 0664);
	if (rc) {
		printf("mdb_env_open failed, error %d %s\n", rc, mdb_strerror(rc));
		goto env_close;
	}

	kbuf.mv_size = mdb_env_get_maxkeysize(env) * 2 + 2;
	kbuf.mv_data = malloc(kbuf.mv_size);
	dbuf.mv_size = 4096;
	dbuf.mv_data = malloc(dbuf.mv_size);

	while(!eof) {
		MDB_val key, data;
		int batch = 0;
		flags = 0;

		if (!(mode & NOHDR))
			readhdr();
		
		rc = mdb_txn_begin(env, NULL, 0, &txn);
		if (rc) {
			printf("mdb_txn_begin failed, error %d %s\n", rc, mdb_strerror(rc));
			goto env_close;
		}

		rc = mdb_open(txn, subname, flags, &dbi);
		if (rc) {
			printf("mdb_open failed, error %d %s\n", rc, mdb_strerror(rc));
			goto txn_abort;
		}

		rc = mdb_cursor_open(txn, dbi, &mc);
		if (rc) {
			printf("mdb_cursor_open failed, error %d %s\n", rc, mdb_strerror(rc));
			goto txn_abort;
		}

		while(1) {
			rc = readline(&key, &kbuf);
			if (rc == EOF)
				break;
			if (rc)
				goto txn_abort;

			rc = readline(&data, &dbuf);
			if (rc)
				goto txn_abort;
			
			rc = mdb_cursor_put(mc, &key, &data, putflags);
			if (rc == MDB_KEYEXIST && putflags)
				continue;
			if (rc)
				goto txn_abort;
			batch++;
			if (batch == 100) {
				rc = mdb_txn_commit(txn);
				if (rc) {
					fprintf(stderr, "%s: line %zd: txn_commit: %s\n",
						prog, lineno, mdb_strerror(rc));
					goto env_close;
				}
				rc = mdb_txn_begin(env, NULL, 0, &txn);
				if (rc) {
					printf("mdb_txn_begin failed, error %d %s\n", rc, mdb_strerror(rc));
					goto env_close;
				}
				rc = mdb_cursor_open(txn, dbi, &mc);
				if (rc) {
					printf("mdb_cursor_open failed, error %d %s\n", rc, mdb_strerror(rc));
					goto txn_abort;
				}
				batch = 0;
			}
		}
		rc = mdb_txn_commit(txn);
		txn = NULL;
		if (rc) {
			fprintf(stderr, "%s: line %zd: txn_commit: %s\n",
				prog, lineno, mdb_strerror(rc));
			goto env_close;
		}
		mdb_dbi_close(env, dbi);
	}

txn_abort:
	mdb_txn_abort(txn);
env_close:
	mdb_env_close(env);

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
