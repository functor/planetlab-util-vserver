/* Copyright 2005 Princeton University */

#include <Python.h>

#define _LARGEFILE64_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <assert.h>


/*
 * hash table implementation
 */

typedef ino64_t KEY;
typedef KEY *PKEY;
typedef struct stat64 VAL;
typedef VAL *PVAL;

static inline
unsigned int
HASH(PKEY key){
	return (int) *key;
}

static inline
unsigned int // boolean
EQUAL(PKEY key1, PKEY key2){
	return *key1 == *key2;
}

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif // MIN

#ifndef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif // MAX


/*
 * hash table support for efficient lookup of duplicate inodes
 */

#define Multiplier  (0x9e3779b9)
#define MaxLogBuckets  (((sizeof (unsigned long))*8) - 2)
#define MaxBuckets     (1<<MaxLogBuckets)
#define MinLogBuckets  (4)
#define MinBuckets     (1<<MinLogBuckets)

/* Thresholds for rehashing the table: *)
 * to avoid crazy oscillations, we must have MaxDensity > 2*MinDensity; *)
 * to avoid excessive probes, we must try to keep MaxDensity low. *)
 * Divide by 100 before using
 */
#define MaxDensity 75 /* max numEntries/NUMBER(buckets) */
#define MinDensity 20 /* min numEntries/NUMBER(buckets) */
#define IdealDensity 50
#define BITSIZE(x) (sizeof(x)*8)

#define NEW(type,num) ((type*)malloc(sizeof(type)*num))
#define DISPOSE(ptr) (free((void*)ptr))

/*
 * Generic Hash Entry Type
 */

typedef struct VoidList {
	struct VoidList *tail;
} VoidList, *PVoidList;

typedef struct HashTable {
	PVoidList *buckets;
	unsigned int numBuckets;    // number of buckets
	unsigned int minLogBuckets; // minimum value for Log_2(initial size) 
	unsigned int logBuckets;    // CEILING(Log2(NUMBER(buckets^))) 
	unsigned int maxEntries;    // maximum number of entries 
	unsigned int minEntries;    // minimum number of entries 
	unsigned int numEntries;    // current num of entries in table 
	PVoidList cache;            // cache of removed elements 
	int cacheSize;              // current size of the cache 
	int maxCacheSize;           // maximum size, -1 means unbounded, 0 no cache 
} HashTable, *PHashTable;

/*
 * Hash Prototypes
 */

PHashTable
Init(PHashTable tbl, unsigned int n, int maxCacheSize);

void
Dispose(PHashTable tbl);

unsigned int
Log_2(unsigned int x);

void
NewBuckets(PHashTable tbl, unsigned int logBuckets);

/*
 * Generic Hash Table support
 */

PHashTable
Init(PHashTable tbl, unsigned int n, int maxCacheSize){
	int idealBuckets;
	int minBuckets;
  
	idealBuckets = MIN(((n*100)/IdealDensity),MaxBuckets);
	minBuckets = MAX(MinBuckets, idealBuckets);
	tbl->minLogBuckets = Log_2(minBuckets);

	NewBuckets(tbl, tbl->minLogBuckets);
	tbl->numEntries = 0;
	tbl->maxCacheSize = maxCacheSize;
	tbl->cacheSize = 0;
	tbl->cache = 0;
	return tbl;
} // Init()


/*
 * Internal procedures
 */

unsigned int
Log_2(unsigned int x){
	/* return CEILING(LOG_2(x)) */
	unsigned int log = 0;
	unsigned int n= 1;

	assert(x != 0);
	while ((log < MaxLogBuckets) && (x > n)){
		log++; 
		n += n;
	}
	return log;
}

void
NewBuckets(PHashTable tbl, unsigned int logBuckets){
	/* Allocate "2^logBuckets" buckets. */
	unsigned int numBuckets = 1 << logBuckets;
	PVoidList *b;
	unsigned int i;

	tbl->buckets = NEW(PVoidList, numBuckets);
	tbl->numBuckets = numBuckets;
	b = tbl->buckets;

	for (i=0; i<tbl->numBuckets; i++){
		b[i] = NULL;
	}
	tbl->logBuckets = logBuckets;
	tbl->maxEntries = MaxDensity * numBuckets / 100;
	tbl->minEntries = MinDensity * numBuckets / 100;
}

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 * Type specific hash entry
 */
typedef struct EntryList {
	struct EntryList *tail;
	KEY key;
	VAL val;
}EntryList, *PEntryList;

/*
 * Type specific Hash implementation functions
 */

static
void
Rehash(PHashTable tbl, unsigned int logBuckets) {
	/* Reallocate "2^logBuckets" buckets, and rehash the entries into
	 * the new table.
	 */

	PVoidList *oldBucketPointer;
	PEntryList *ob, obi;
	PEntryList *nb, *nbh;
	PEntryList that, tail;
	unsigned int index; 
	unsigned int i;
	unsigned int oldNumBuckets;

	return;
	assert(logBuckets <= MaxLogBuckets);
	assert(logBuckets >= tbl->minLogBuckets);
	oldBucketPointer = tbl->buckets;
	ob = (PEntryList*)tbl->buckets;
	oldNumBuckets = tbl->numBuckets;

	NewBuckets(tbl, logBuckets);
	nb = (PEntryList*)tbl->buckets;

	for(i=0;i<oldNumBuckets;i++){
		obi = ob[i];
		that = obi;
		while (that != NULL) {
			index = (HASH(&(that->key))*Multiplier)>>(BITSIZE(unsigned long)-tbl->logBuckets);
			nbh = &(nb[index]);
			tail = that->tail;
			that->tail = *nbh;
			*nbh = that;
			that = tail;
		}
	}
	DISPOSE (oldBucketPointer);
}

static inline
unsigned int /* boolean */
Get(PHashTable tbl, PKEY key, PVAL *val){
	PEntryList that;
	unsigned int index;

	index = (HASH(key)*Multiplier)>>(BITSIZE(unsigned long)-tbl->logBuckets);
	that = (PEntryList)tbl->buckets[index];
	while ((that != NULL) && !EQUAL(key,&(that->key))) {
		that = that->tail;
	}
	if (that != NULL){
		*val = &that->val;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

static inline 
unsigned int /* boolean */
Put(PHashTable tbl, PKEY key, PVAL *val){
	PEntryList that;
	PEntryList *first;
	unsigned int index;
	unsigned int res;

	index = (HASH(key)*Multiplier)>>(BITSIZE(unsigned long)-tbl->logBuckets);
	first = (PEntryList*)&(tbl->buckets[index]);
	that = *first;
	while ((that != NULL) && !EQUAL(key, &(that->key))){
		that = that->tail;
	}
  
	/* found an entry in the hash table given above key */
	if (that != NULL){
		res = TRUE;
	}
	else {
		/* check if we can reuse something from the cache */
		if (tbl->cache != NULL) {
			that = (PEntryList)tbl->cache;
			tbl->cache = (PVoidList)tbl->cache->tail;
			that->key = *key;
			that->tail = *first;
			*first = that;
		}
		else {
			that = NEW(EntryList,1);
			that->key = *key;
			that->tail = *first;
			*first = that;
		}
		that->val = **val;

		tbl->numEntries++;
		if ((tbl->logBuckets < MaxLogBuckets)
		    && (tbl->numEntries > tbl->maxEntries)){
			Rehash(tbl, tbl->logBuckets + 1); /* too crowded */
		}
		res = FALSE;
	}
	*val = &that->val;
	return res;

}

static inline
int
Delete(PHashTable tbl,PKEY key){
	PEntryList that, prev;
	PEntryList *first;
	unsigned int index;

	index = (HASH(key)*Multiplier)>>(BITSIZE(unsigned long)-tbl->logBuckets);
	first = (PEntryList*)&(tbl->buckets[index]);
	that = *first;
	prev = NULL;

	while ((that != NULL) && !EQUAL(key, &(that->key))){
		prev = that;
		that = that->tail;
	}
	if (that != NULL) {
		if (prev == NULL) {
			*first = that->tail;
		}
		else {
			prev->tail = that->tail;
		}
		if ((tbl->maxCacheSize == -1)||(tbl->cacheSize < tbl->maxCacheSize)) {
			that->tail = (PEntryList)tbl->cache;
			tbl->cache = (PVoidList)that;
			tbl->cacheSize++;
		}
		else {
			DISPOSE (that);
		}
		tbl->numEntries--;
		if (tbl->maxCacheSize == 0) {
			if ((tbl->logBuckets > tbl->minLogBuckets)
			    && (tbl->numEntries < tbl->minEntries)) {
				Rehash(tbl, tbl->logBuckets - 1); /* too sparse */
			}
		}
		return TRUE;
	}
	else {
		return FALSE;
	}
}

typedef void (*callback)(PKEY key, PVAL val);

void
Iterate(PHashTable tbl, callback fn)
{
	PVoidList that;
	unsigned int i;
  
	for(i=0;i<tbl->numBuckets;i++) {
		that = tbl->buckets[i];
		while ( that != (PVoidList)0 ) {
			PEntryList entry = (PEntryList)that;
			fn(&entry->key,&entry->val);
			that = that->tail;
		}
	}
}

void
Dispose(PHashTable tbl)
{
	PVoidList that, next;
	unsigned int i;

	for(i=0;i<tbl->numBuckets;i++) {
		that = tbl->buckets[i];
		while( that != NULL) {
			next = that->tail;
			DISPOSE (that);
			tbl->numEntries--;
			that = next;
		}
	}
	DISPOSE(tbl->buckets);
	assert(tbl->numEntries = 0);
}

static int /* boolean */
INOPut(PHashTable tbl, ino64_t* key, struct stat64 **val){
	return Put(tbl, key, val);
}

__extension__ typedef long long		longlong;

struct stats {
	longlong inodes;
	longlong blocks;
	longlong size;
};

static short verbose = 0;

static int vdu_onedir (PHashTable tbl, struct stats *__s, char const *path)
{
	char const *foo = path;
	struct stat64 dirst, st;
	struct dirent *ent;
	char *name;
	DIR *dir;
	int dirfd;
	int res = 0;
	longlong dirsize, dirinodes, dirblocks;

	dirsize = dirinodes = dirblocks = 0;

	// A handle to speed up chdir
	if ((dirfd = open (path,O_RDONLY)) == -1) {
		return -1;
	}

	if (fchdir (dirfd) == -1) {
		return -1;
	}

	if (fstat64 (dirfd,&dirst) != 0) {
		return -1;
	}

	if ((dir = opendir (".")) == NULL) {
		return -1;
	}

	/* Walk the directory entries and compute the sum of inodes,
	 * blocks, and disk space used. This code will recursively descend
	 * down the directory structure. 
	 */

	while ((ent=readdir(dir))!=NULL){
		if (lstat64(ent->d_name,&st)==-1){
			continue;
		}
	
		dirinodes ++;

		if (S_ISREG(st.st_mode)){
			if (st.st_nlink > 1){
				struct stat64 *val;
				int nlink;

				/* Check hash table if we've seen this inode
				 * before. Note that the hash maintains a
				 * (inode,struct stat) key value pair.
				 */

				val = &st;

				(void) INOPut(tbl,&st.st_ino,&val);

				/* Note that after the INOPut call "val" refers to the
				 * value entry in the hash table --- not &st.  This
				 * means that if the inode has been put into the hash
				 * table before, val will refer to the first st that
				 * was put into the hashtable.  Otherwise, if it is
				 * the first time it is put into the hash table, then
				 * val will be equal to this &st.
				 */
				nlink = val->st_nlink;
				nlink --;

				/* val refers to value in hash tbale */
				if (nlink == 0) {

					/* We saw all hard links to this particular inode
					 * as part of this sweep of vdu. So account for
					 * the size and blocks required by the file.
					 */

					dirsize += val->st_size;
					dirblocks += val->st_blocks;

					/* Do not delete the (ino,val) tuple from the tbl,
					 * as we need to handle the case when we are
					 * double counting a file due to a bind mount.
					 */
					val->st_nlink = 0;

				} else if (nlink > 0) {
					val->st_nlink = nlink;
				} else /* if(nlink < 0) */ {
					/* We get here when we are double counting nlinks
					   due a bind mount. */

					/* DO NOTHING */
				}
			} else {
				dirsize += st.st_size;
				dirblocks += st.st_blocks;
			}

		} else if (S_ISDIR(st.st_mode)) {
			if ((st.st_dev == dirst.st_dev) &&
			    (strcmp(ent->d_name,".")!=0) &&
			    (strcmp(ent->d_name,"..")!=0)) {

				dirsize += st.st_size;
				dirblocks += st.st_blocks;

				name = strdup(ent->d_name);
				if (name==0) {
					return -1;
				}
				res |= vdu_onedir(tbl,__s,name);
				free(name);
				fchdir(dirfd);
			}
		} else {
			/* dirsize += st.st_size; */
			/* dirblocks += st.st_blocks; */
		}
	}
	closedir (dir);
	close (dirfd);
	__s->inodes += dirinodes;
	__s->blocks += dirblocks;
	__s->size   += dirsize;
	if (verbose) {
		printf("%16lld %16lld %16lld %s\n",dirinodes, dirblocks, dirsize,foo);
		printf("%16lld %16lld %16lld %s\n",__s->inodes, __s->blocks, __s->size,foo);
	}

	return res;
}


static PyObject *
do_vdu(PyObject *self, PyObject *args)
{
	PyObject *tuple;

	const char *path;
	int res;
	struct stats s;
	HashTable tbl;
	int  cwd_fd;

	if (!PyArg_ParseTuple(args, "s", &path))
		return Py_None;

	/* init of tbl and stats */
	s.inodes = s.blocks = s.size = 0;
	(void) Init(&tbl,0,0);

	cwd_fd = open(".", O_RDONLY);
	res = vdu_onedir(&tbl, &s, path);
	fchdir(cwd_fd);
	close(cwd_fd);

	/* deallocate whatever has been added to tbl */
	Dispose(&tbl);

	/* create a python (inode, block, size) tuple */
	tuple = Py_BuildValue("(L,L,L)",
			      s.inodes,
			      s.blocks>>1, /* NOTE: div by 2 to adjust
					    * 512b block count to 1K
					    * block count 
					    */
			      s.size);
	return (res == -1) ? PyErr_SetFromErrno(PyExc_OSError) : tuple;
}

static PyMethodDef  methods[] = {
	{ "vdu", do_vdu, METH_VARARGS,
	  "perform vdu operation on directory tree" },
	{ NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
initvduimpl(void)
{
	Py_InitModule("vduimpl", methods);
}
