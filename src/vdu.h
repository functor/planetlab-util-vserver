#ifndef __VDU_H
#define __VDU_H

typedef ino_t KEY;
typedef KEY *PKEY;
typedef struct stat VAL;
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


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// hash table support for efficient lookup of duplicate inodes */
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define Multiplier  (0x9e3779b9)
#define MaxLogBuckets  (((sizeof (unsigned long))*8) - 2)
#define MaxBuckets     (1<<MaxLogBuckets)
#define MinLogBuckets  (4)
#define MinBuckets     (1<<MinLogBuckets)

// Thresholds for rehashing the table: *)
// to avoid crazy oscillations, we must have MaxDensity > 2*MinDensity; *)
// to avoid excessive probes, we must try to keep MaxDensity low. *)
// Divide by 100 before using
#define MaxDensity 75 /* max numEntries/NUMBER(buckets) */
#define MinDensity 20 /* min numEntries/NUMBER(buckets) */
#define IdealDensity 50
#define BITSIZE(x) (sizeof(x)*8)

#define NEW(type,num) ((type*)malloc(sizeof(type)*num))
#define DISPOSE(ptr) (free((void*)ptr))

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Generic Hash Entry Type
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Hash Prototypes
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

PHashTable
Init(PHashTable tbl, unsigned int n, int maxCacheSize);

void
Dispose(PHashTable tbl);

unsigned int
Log_2(unsigned int x);

void
NewBuckets(PHashTable tbl, unsigned int logBuckets);

//
// Generic Hash Table support
//

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


//
// Internal procedures
//

unsigned int
Log_2(unsigned int x){
  // return CEILING(LOG_2(x))
  unsigned int log = 0;
  unsigned int n= 1;

  assert(x != 0);
  while ((log < MaxLogBuckets) && (x > n)){
    log++; 
    n += n;
  }
  return log;
} // Log_2()

void
NewBuckets(PHashTable tbl, unsigned int logBuckets){
  // Allocate "2^logBuckets" buckets.
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
} // NewBuckets()

#ifndef NULL
#define NULL (void*)0
#endif // !NULL

#ifndef TRUE
#define TRUE 1
#endif // !TRUE

#ifndef FALSE
#define FALSE 0
#endif // !FALSE

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Type specific hash entry
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
typedef struct EntryList {
  struct EntryList *tail;
  KEY key;
  VAL val;
}EntryList, *PEntryList;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Type specific Hash implementation functions
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

static
void
Rehash(PHashTable tbl, unsigned int logBuckets) {
  // Reallocate "2^logBuckets" buckets, and rehash the entries into
  // the new table.

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
unsigned int // boolean
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
} // Get()

static inline 
unsigned int // boolean
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
  
  // found an entry in the hash table given above key
  if (that != NULL){
    res = TRUE;
  }
  else {
    // check if we can reuse something from the cache
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
      Rehash(tbl, tbl->logBuckets + 1); // too crowded
    }
    res = FALSE;
  }
  *val = &that->val;
  return res;

} // Put()

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
	Rehash(tbl, tbl->logBuckets - 1); // too sparse
      }
    }
    return TRUE;
  }
  else {
    return FALSE;
  }
} // Delete()

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
} // Dispose;



#endif // __VDU_H
