/*
** 2003 September 6
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This is the header file for information that is private to the
** VDBE.  This information used to all be at the top of the single
** source code file "vdbe.c".  When that file became too big (over
** 6000 lines long) it was split up into several smaller files and
** this header information was factored out.
*/
#ifndef SQLITE_VDBEINT_H
#define SQLITE_VDBEINT_H

#if defined(SQLITE_BUILDING_FOR_COMDB2)
#include <time.h>
#include <strings.h>
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */

/*
** The maximum number of times that a statement will try to reparse
** itself before giving up and returning SQLITE_SCHEMA.
*/
#ifndef SQLITE_MAX_SCHEMA_RETRY
# define SQLITE_MAX_SCHEMA_RETRY 50
#endif

/*
** VDBE_DISPLAY_P4 is true or false depending on whether or not the
** "explain" P4 display logic is enabled.
*/
#if !defined(SQLITE_OMIT_EXPLAIN) || !defined(NDEBUG) \
     || defined(VDBE_PROFILE) || defined(SQLITE_DEBUG)
# define VDBE_DISPLAY_P4 1
#else
# define VDBE_DISPLAY_P4 0
#endif

/*
** SQL is translated into a sequence of instructions to be
** executed by a virtual machine.  Each instruction is an instance
** of the following structure.
*/
typedef struct VdbeOp Op;

/*
** Boolean values
*/
typedef unsigned Bool;

#if defined(SQLITE_BUILDING_FOR_COMDB2)
enum { VDBESORTER_FIND, VDBESORTER_MOVE, VDBESORTER_WRITE };
/* moved vdbesorter here because is needed in sqlglue.c */
/* Opaque type used by code in vdbesort.c */
typedef struct PmaReader PmaReader;
typedef struct MergeEngine MergeEngine;
typedef struct SorterRecord SorterRecord;
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */

/* Opaque type used by code in vdbesort.c */
typedef struct VdbeSorter VdbeSorter;

#if defined(SQLITE_BUILDING_FOR_COMDB2)
typedef struct SorterList SorterList;
typedef struct SortSubtask SortSubtask;
typedef struct SorterFile SorterFile;

void addVdbeSorterCost(const VdbeSorter *);
void addVdbeToThdCost(int type, int *data);

struct SorterFile {
  sqlite3_file *pFd;              /* File handle */
  i64 iEof;                       /* Bytes of data stored in pFd */
};

/*
** An in-memory list of objects to be sorted.
**
** If aMemory==0 then each object is allocated separately and the objects
** are connected using SorterRecord.u.pNext.  If aMemory!=0 then all objects
** are stored in the aMemory[] bulk memory, one right after the other, and
** are connected using SorterRecord.u.iNext.
*/
struct SorterList {
  SorterRecord *pList;            /* Linked list of records */
  u8 *aMemory;                    /* If non-NULL, bulk memory to hold pList */
  int szPMA;                      /* Size of pList as PMA in bytes */
};

typedef int (*SorterCompare)(SortSubtask*,int*,const void*,int,const void*,int);
struct SortSubtask {
  SQLiteThread *pThread;          /* Background thread, if any */
  int bDone;                      /* Set if thread is finished but not joined */
  VdbeSorter *pSorter;            /* Sorter that owns this sub-task */
  UnpackedRecord *pUnpacked;      /* Space to unpack a record */
  SorterList list;                /* List for thread to write to a PMA */
  int nPMA;                       /* Number of PMAs currently in file */
  SorterCompare xCompare;         /* Compare function to use */
  SorterFile file;                /* Temp file for level-0 PMAs */
  SorterFile file2;               /* Space for other PMAs */
};


struct VdbeSorter {
  int mnPmaSize;                  /* Minimum PMA size, in bytes */
  int mxPmaSize;                  /* Maximum PMA size, in bytes.  0==no limit */
  int mxKeysize;                  /* Largest serialized key seen so far */
  int pgsz;                       /* Main database page size */
  PmaReader *pReader;             /* Readr data from here after Rewind() */
  MergeEngine *pMerger;           /* Or here, if bUseThreads==0 */
  sqlite3 *db;                    /* Database connection */
  KeyInfo *pKeyInfo;              /* How to compare records */
  UnpackedRecord *pUnpacked;      /* Used by VdbeSorterCompare() */
  SorterList list;                /* List of in-memory records */
  int iMemory;                    /* Offset of free space in list.aMemory */
  int nMemory;                    /* Size of list.aMemory allocation in bytes */
  u8 bUsePMA;                     /* True if one or more PMAs created */
  u8 bUseThreads;                 /* True to use background threads */
  u8 iPrev;                       /* Previous thread used to flush PMA */
  u8 nTask;                       /* Size of aTask[] array */
  u8 typeMask;
  SortSubtask aTask[1];           /* One or more subtasks */

  int nfind;
  int nmove;
  int nwrite;
};
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */

/* Elements of the linked list at Vdbe.pAuxData */
typedef struct AuxData AuxData;

/* Types of VDBE cursors */
#define CURTYPE_BTREE       0
#define CURTYPE_SORTER      1
#define CURTYPE_VTAB        2
#define CURTYPE_PSEUDO      3

/*
** A VdbeCursor is an superclass (a wrapper) for various cursor objects:
**
**      * A b-tree cursor
**          -  In the main database or in an ephemeral database
**          -  On either an index or a table
**      * A sorter
**      * A virtual table
**      * A one-row "pseudotable" stored in a single register
*/
typedef struct VdbeCursor VdbeCursor;
struct VdbeCursor {
  u8 eCurType;            /* One of the CURTYPE_* values above */
  i8 iDb;                 /* Index of cursor database in db->aDb[] (or -1) */
  u8 nullRow;             /* True if pointing to a row with no data */
  u8 deferredMoveto;      /* A call to sqlite3BtreeMoveto() is needed */
  u8 isTable;             /* True for rowid tables.  False for indexes */
#ifdef SQLITE_DEBUG
  u8 seekOp;              /* Most recent seek operation on this cursor */
  u8 wrFlag;              /* The wrFlag argument to sqlite3BtreeCursor() */
#endif
  Bool isEphemeral:1;     /* True for an ephemeral table */
  Bool useRandomRowid:1;  /* Generate new record numbers semi-randomly */
  Bool isOrdered:1;       /* True if the table is not BTREE_UNORDERED */
  u16 seekHit;            /* See the OP_SeekHit and OP_IfNoHope opcodes */
  Btree *pBtx;            /* Separate file holding temporary table */
  i64 seqCount;           /* Sequence counter */
  int *aAltMap;           /* Mapping from table to index column numbers */

  /* Cached OP_Column parse information is only valid if cacheStatus matches
  ** Vdbe.cacheCtr.  Vdbe.cacheCtr will never take on the value of
  ** CACHE_STALE (0) and so setting cacheStatus=CACHE_STALE guarantees that
  ** the cache is out of date. */
  u32 cacheStatus;        /* Cache is valid if this matches Vdbe.cacheCtr */
  int seekResult;         /* Result of previous sqlite3BtreeMoveto() or 0
                          ** if there have been no prior seeks on the cursor. */
  /* seekResult does not distinguish between "no seeks have ever occurred
  ** on this cursor" and "the most recent seek was an exact match".
  ** For CURTYPE_PSEUDO, seekResult is the register holding the record */

  /* When a new VdbeCursor is allocated, only the fields above are zeroed.
  ** The fields that follow are uninitialized, and must be individually
  ** initialized prior to first use. */
  VdbeCursor *pAltCursor; /* Associated index cursor from which to read */
  union {
    BtCursor *pCursor;          /* CURTYPE_BTREE or _PSEUDO.  Btree cursor */
    sqlite3_vtab_cursor *pVCur; /* CURTYPE_VTAB.              Vtab cursor */
    VdbeSorter *pSorter;        /* CURTYPE_SORTER.            Sorter object */
  } uc;
  KeyInfo *pKeyInfo;      /* Info about index keys needed by index cursors */
  u32 iHdrOffset;         /* Offset to next unparsed byte of the header */
  Pgno pgnoRoot;          /* Root page of the open btree cursor */
  i16 nField;             /* Number of fields in the header */
  u16 nHdrParsed;         /* Number of header fields parsed so far */
  i64 movetoTarget;       /* Argument to the deferred sqlite3BtreeMoveto() */
  u32 *aOffset;           /* Pointer to aType[nField] */
  const u8 *aRow;         /* Data for the current row, if all on one page */
  u32 payloadSize;        /* Total number of bytes in the record */
  u32 szRow;              /* Byte available in aRow */
#ifdef SQLITE_ENABLE_COLUMN_USED_MASK
  u64 maskUsed;           /* Mask of columns used by this cursor */
#endif

#if defined(SQLITE_BUILDING_FOR_COMDB2)
  int nCookFields;
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */

  /* 2*nField extra array elements allocated for aType[], beyond the one
  ** static element declared in the structure.  nField total array slots for
  ** aType[] and nField+1 array slots for aOffset[] */
  u32 aType[1];           /* Type values record decode.  MUST BE LAST */
};


/*
** A value for VdbeCursor.cacheStatus that means the cache is always invalid.
*/
#define CACHE_STALE 0

/*
** When a sub-program is executed (OP_Program), a structure of this type
** is allocated to store the current value of the program counter, as
** well as the current memory cell array and various other frame specific
** values stored in the Vdbe struct. When the sub-program is finished, 
** these values are copied back to the Vdbe from the VdbeFrame structure,
** restoring the state of the VM to as it was before the sub-program
** began executing.
**
** The memory for a VdbeFrame object is allocated and managed by a memory
** cell in the parent (calling) frame. When the memory cell is deleted or
** overwritten, the VdbeFrame object is not freed immediately. Instead, it
** is linked into the Vdbe.pDelFrame list. The contents of the Vdbe.pDelFrame
** list is deleted when the VM is reset in VdbeHalt(). The reason for doing
** this instead of deleting the VdbeFrame immediately is to avoid recursive
** calls to sqlite3VdbeMemRelease() when the memory cells belonging to the
** child frame are released.
**
** The currently executing frame is stored in Vdbe.pFrame. Vdbe.pFrame is
** set to NULL if the currently executing frame is the main program.
*/
typedef struct VdbeFrame VdbeFrame;
struct VdbeFrame {
  Vdbe *v;                /* VM this frame belongs to */
  VdbeFrame *pParent;     /* Parent of this frame, or NULL if parent is main */
  Op *aOp;                /* Program instructions for parent frame */
  i64 *anExec;            /* Event counters from parent frame */
  Mem *aMem;              /* Array of memory cells for parent frame */
  VdbeCursor **apCsr;     /* Array of Vdbe cursors for parent frame */
  u8 *aOnce;              /* Bitmask used by OP_Once */
  void *token;            /* Copy of SubProgram.token */
  i64 lastRowid;          /* Last insert rowid (sqlite3.lastRowid) */
  AuxData *pAuxData;      /* Linked list of auxdata allocations */
#if SQLITE_DEBUG
  u32 iFrameMagic;        /* magic number for sanity checking */
#endif
  int nCursor;            /* Number of entries in apCsr */
  int pc;                 /* Program Counter in parent (calling) frame */
  int nOp;                /* Size of aOp array */
  int nMem;               /* Number of entries in aMem */
  int nChildMem;          /* Number of memory cells for child frame */
  int nChildCsr;          /* Number of cursors for child frame */
  int nChange;            /* Statement changes (Vdbe.nChange)     */
  int nDbChange;          /* Value of db->nChange */
};

/* Magic number for sanity checking on VdbeFrame objects */
#define SQLITE_FRAME_MAGIC 0x879fb71e

/*
** Return a pointer to the array of registers allocated for use
** by a VdbeFrame.
*/
#define VdbeFrameMem(p) ((Mem *)&((u8 *)p)[ROUND8(sizeof(VdbeFrame))])

/*
** Internally, the vdbe manipulates nearly all SQL values as Mem
** structures. Each Mem struct may cache multiple representations (string,
** integer etc.) of the same value.
*/
struct sqlite3_value {
  union MemValue {
    double r;           /* Real value used when MEM_Real is set in flags */
    i64 i;              /* Integer value used when MEM_Int is set in flags */
    int nZero;          /* Extra zero bytes when MEM_Zero and MEM_Blob set */
    const char *zPType; /* Pointer type when MEM_Term|MEM_Subtype|MEM_Null */
    FuncDef *pDef;      /* Used only when flags==MEM_Agg */
#if defined(SQLITE_BUILDING_FOR_COMDB2)
    OpFunc *pOpFunc;    /* Used when flags==MEM_OpFunc */
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
  } u;
#if defined(SQLITE_BUILDING_FOR_COMDB2)
  union {
    dttz_t     dt;    /* Datetime support */
    intv_t     tv;    /* Interval and Decimal (hack) support */
  } du;
  const char *tz;     /* timezone pointer */
  int dtprec;         /* Preferred datetime precision upon conversion.
                         If the Mem is already a DATEIME object, no conversion
                         is needed. The precision is ignored (eg, data is from
                         ondisk datetime or from api datetime parameter). */
  u32 flags;          /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
#else /* defined(SQLITE_BUILDING_FOR_COMDB2) */
  u16 flags;          /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
  u8  enc;            /* SQLITE_UTF8, SQLITE_UTF16BE, SQLITE_UTF16LE */
  u8  eSubtype;       /* Subtype for this value */
  int n;              /* Number of characters in string value, excluding '\0' */
  char *z;            /* String or BLOB value */
  /* ShallowCopy only needs to copy the information above */
  char *zMalloc;      /* Space to hold MEM_Str or MEM_Blob if szMalloc>0 */
  int szMalloc;       /* Size of the zMalloc allocation */
  u32 uTemp;          /* Transient storage for serial_type in OP_MakeRecord */
  sqlite3 *db;        /* The associated database connection */
  void (*xDel)(void*);/* Destructor for Mem.z - only valid if MEM_Dyn */
#ifdef SQLITE_DEBUG
  Mem *pScopyFrom;    /* This Mem is a shallow copy of pScopyFrom */
  u16 mScopyFlags;    /* flags value immediately after the shallow copy */
#endif
};

/*
** Size of struct Mem not including the Mem.zMalloc member or anything that
** follows.
*/
#define MEMCELLSIZE offsetof(Mem,zMalloc)

/* One or more of the following flags are set to indicate the validOK
** representations of the value stored in the Mem struct.
**
** If the MEM_Null flag is set, then the value is an SQL NULL value.
** For a pointer type created using sqlite3_bind_pointer() or
** sqlite3_result_pointer() the MEM_Term and MEM_Subtype flags are also set.
**
** If the MEM_Str flag is set then Mem.z points at a string representation.
** Usually this is encoded in the same unicode encoding as the main
** database (see below for exceptions). If the MEM_Term flag is also
** set, then the string is nul terminated. The MEM_Int and MEM_Real 
** flags may coexist with the MEM_Str flag.
*/
#define MEM_Null      0x0001   /* Value is NULL (or a pointer) */
#define MEM_Str       0x0002   /* Value is a string */
#define MEM_Int       0x0004   /* Value is an integer */
#define MEM_Real      0x0008   /* Value is a real number */
#define MEM_Blob      0x0010   /* Value is a BLOB */
#if defined(SQLITE_BUILDING_FOR_COMDB2)
#define MEM_Datetime  0x00020  /* Value is a datetime */
#define MEM_Interval  0x00040  /* Value is an interval/decimal */
#define MEM_Small     0x00080  /* Value is a small float */
#define MEM_AffMask   0x000ff  /* Mask of affinity bits */
#define MEM_FromBind  0x00100  /* Value originates from sqlite3_bind() */
#define MEM_Undefined 0x00200  /* Value is undefined */
#define MEM_Cleared   0x00400  /* NULL set by OP_Null, not from data */
#define MEM_Comdb2    0x00800  /* Value is already in comdb2 row format */
#define MEM_TypeMask  0x306ff  /* Mask of type bits */

#else /* defined(SQLITE_BUILDING_FOR_COMDB2) */
#define MEM_AffMask   0x001f   /* Mask of affinity bits */
#define MEM_FromBind  0x0020   /* Value originates from sqlite3_bind() */
/* Available          0x0040   */
#define MEM_Undefined 0x0080   /* Value is undefined */
#define MEM_Cleared   0x0100   /* NULL set by OP_Null, not from data */
#define MEM_TypeMask  0xc1df   /* Mask of type bits */
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */


/* Whenever Mem contains a valid string or blob representation, one of
** the following flags must be set to determine the memory management
** policy for Mem.z.  The MEM_Term flag tells us whether or not the
** string is \000 or \u0000 terminated
*/
#if defined(SQLITE_BUILDING_FOR_COMDB2)
#define MEM_Term      0x000800 /* String in Mem.z is zero terminated */
#define MEM_Dyn       0x001000 /* Need to call Mem.xDel() on Mem.z */
#define MEM_Static    0x002000 /* Mem.z points to a static string */
#define MEM_Ephem     0x004000 /* Mem.z points to an ephemeral string */
#define MEM_Agg       0x008000 /* Mem.z points to an agg function context */
#define MEM_Zero      0x010000 /* Mem.i contains count of 0s appended to blob */
#define MEM_Subtype   0x020000 /* Mem.eSubtype is valid */
#define MEM_Xor       0x040000 /* Mem.z needs XOR; <DESCEND> keys */
#define MEM_OpFunc    0x080000 /* Mem.u is a custom function */
#define MEM_Master    0x100000 /* Value will be set on master */
#else /* defined(SQLITE_BUILDING_FOR_COMDB2) */
#define MEM_Term      0x0200   /* String in Mem.z is zero terminated */
#define MEM_Dyn       0x0400   /* Need to call Mem.xDel() on Mem.z */
#define MEM_Static    0x0800   /* Mem.z points to a static string */
#define MEM_Ephem     0x1000   /* Mem.z points to an ephemeral string */
#define MEM_Agg       0x2000   /* Mem.z points to an agg function context */
#define MEM_Zero      0x4000   /* Mem.i contains count of 0s appended to blob */
#define MEM_Subtype   0x8000   /* Mem.eSubtype is valid */
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
#ifdef SQLITE_OMIT_INCRBLOB
  #undef MEM_Zero
  #define MEM_Zero 0x0000
#endif

/* Return TRUE if Mem X contains dynamically allocated content - anything
** that needs to be deallocated to avoid a leak.
*/
#define VdbeMemDynamic(X)  \
  (((X)->flags&(MEM_Agg|MEM_Dyn))!=0)

/*
** Clear any existing type flags from a Mem and replace them with f
*/
#define MemSetTypeFlag(p, f) \
   ((p)->flags = ((p)->flags&~(MEM_TypeMask|MEM_Zero))|f)

/*
** True if Mem X is a NULL-nochng type.
*/
#define MemNullNochng(X) \
  ((X)->flags==(MEM_Null|MEM_Zero) && (X)->n==0 && (X)->u.nZero==0)

/*
** Return true if a memory cell is not marked as invalid.  This macro
** is for use inside assert() statements only.
*/
#ifdef SQLITE_DEBUG
#define memIsValid(M)  ((M)->flags & MEM_Undefined)==0
#endif

/*
** Each auxiliary data pointer stored by a user defined function 
** implementation calling sqlite3_set_auxdata() is stored in an instance
** of this structure. All such structures associated with a single VM
** are stored in a linked list headed at Vdbe.pAuxData. All are destroyed
** when the VM is halted (if not before).
*/
struct AuxData {
  int iAuxOp;                     /* Instruction number of OP_Function opcode */
  int iAuxArg;                    /* Index of function argument. */
  void *pAux;                     /* Aux data pointer */
  void (*xDeleteAux)(void*);      /* Destructor for the aux data */
  AuxData *pNextAux;              /* Next element in list */
};

/*
** The "context" argument for an installable function.  A pointer to an
** instance of this structure is the first argument to the routines used
** implement the SQL functions.
**
** There is a typedef for this structure in sqlite.h.  So all routines,
** even the public interface to SQLite, can use a pointer to this structure.
** But this file is the only place where the internal details of this
** structure are known.
**
** This structure is defined inside of vdbeInt.h because it uses substructures
** (Mem) which are only defined there.
*/
struct sqlite3_context {
  Mem *pOut;              /* The return value is stored here */
  FuncDef *pFunc;         /* Pointer to function information */
  Mem *pMem;              /* Memory cell used to store aggregate context */
  Vdbe *pVdbe;            /* The VM that owns this context */
  int iOp;                /* Instruction number of OP_Function */
  int isError;            /* Error code returned by the function. */
  u8 skipFlag;            /* Skip accumulator loading if true */
  u8 argc;                /* Number of arguments */
  sqlite3_value *argv[1]; /* Argument set */
};

/* A bitfield type for use inside of structures.  Always follow with :N where
** N is the number of bits.
*/
typedef unsigned bft;  /* Bit Field Type */

/* The ScanStatus object holds a single value for the
** sqlite3_stmt_scanstatus() interface.
*/
typedef struct ScanStatus ScanStatus;
struct ScanStatus {
  int addrExplain;                /* OP_Explain for loop */
  int addrLoop;                   /* Address of "loops" counter */
  int addrVisit;                  /* Address of "rows visited" counter */
  int iSelectID;                  /* The "Select-ID" for this loop */
  LogEst nEst;                    /* Estimated output rows per loop */
  char *zName;                    /* Name of table or index */
};

/* The DblquoteStr object holds the text of a double-quoted
** string for a prepared statement.  A linked list of these objects
** is constructed during statement parsing and is held on Vdbe.pDblStr.
** When computing a normalized SQL statement for an SQL statement, that
** list is consulted for each double-quoted identifier to see if the
** identifier should really be a string literal.
*/
typedef struct DblquoteStr DblquoteStr;
struct DblquoteStr {
  DblquoteStr *pNextStr;   /* Next string literal in the list */
  char z[8];               /* Dequoted value for the string */
};

/*
** An instance of the virtual machine.  This structure contains the complete
** state of the virtual machine.
**
** The "sqlite3_stmt" structure pointer that is returned by sqlite3_prepare()
** is really a pointer to an instance of this structure.
*/
struct Vdbe {
  sqlite3 *db;            /* The database connection that owns this statement */
  Vdbe *pPrev,*pNext;     /* Linked list of VDBEs with the same Vdbe.db */
  Parse *pParse;          /* Parsing context used to create this Vdbe */
  ynVar nVar;             /* Number of entries in aVar[] */
  u32 magic;              /* Magic number for sanity checking */
  int nMem;               /* Number of memory locations currently allocated */
  int nCursor;            /* Number of slots in apCsr[] */
  u32 cacheCtr;           /* VdbeCursor row cache generation counter */
  int pc;                 /* The program counter */
  int rc;                 /* Value to return */
  int nChange;            /* Number of db changes made since last reset */
  int iStatement;         /* Statement number (or 0 if has no opened stmt) */
  i64 iCurrentTime;       /* Value of julianday('now') for this statement */
  i64 nFkConstraint;      /* Number of imm. FK constraints this VM */
  i64 nStmtDefCons;       /* Number of def. constraints when stmt started */
  i64 nStmtDefImmCons;    /* Number of def. imm constraints when stmt started */
  Mem *aMem;              /* The memory locations */
  Mem **apArg;            /* Arguments to currently executing user function */
  VdbeCursor **apCsr;     /* One element of this array for each open cursor */
  Mem *aVar;              /* Values for the OP_Variable opcode. */

  /* When allocating a new Vdbe object, all of the fields below should be
  ** initialized to zero or NULL */

  Op *aOp;                /* Space to hold the virtual machine's program */
  int nOp;                /* Number of instructions in the program */
  int nOpAlloc;           /* Slots allocated for aOp[] */
  Mem *aColName;          /* Column names to return */
  Mem *pResultSet;        /* Pointer to an array of results */
  char *zErrMsg;          /* Error message written here */
  VList *pVList;          /* Name of variables */
#ifndef SQLITE_OMIT_TRACE
  i64 startTime;          /* Time when query started - used for profiling */
#endif
#ifdef SQLITE_DEBUG
  int rcApp;              /* errcode set by sqlite3_result_error_code() */
  u32 nWrite;             /* Number of write operations that have occurred */
#endif
  u16 nResColumn;         /* Number of columns in one row of the result set */
  u8 errorAction;         /* Recovery action to do in case of an error */
  u8 minWriteFileFormat;  /* Minimum file format for writable database files */
  u8 prepFlags;           /* SQLITE_PREPARE_* flags */
  bft expired:2;          /* 1: recompile VM immediately  2: when convenient */
  bft explain:2;          /* True if EXPLAIN present on SQL command */
  bft doingRerun:1;       /* True if rerunning after an auto-reprepare */
  bft changeCntOn:1;      /* True to update the change-counter */
  bft runOnlyOnce:1;      /* Automatically expire on reset */
  bft usesStmtJournal:1;  /* True if uses a statement journal */
  bft readOnly:1;         /* True for statements that do not write */
  bft bIsReader:1;        /* True for statements that read */
#if defined(SQLITE_BUILDING_FOR_COMDB2)
  bft recording:1;
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
  yDbMask btreeMask;      /* Bitmask of db->aDb[] entries referenced */
  yDbMask lockMask;       /* Subset of btreeMask that requires a lock */
  u32 aCounter[7];        /* Counters used by sqlite3_stmt_status() */
  char *zSql;             /* Text of the SQL statement that generated this */
#ifdef SQLITE_ENABLE_NORMALIZE
  char *zNormSql;         /* Normalization of the associated SQL statement */
  DblquoteStr *pDblStr;   /* List of double-quoted string literals */
#endif
  void *pFree;            /* Free this when deleting the vdbe */
  VdbeFrame *pFrame;      /* Parent frame */
  VdbeFrame *pDelFrame;   /* List of frame objects to free on VM reset */
  int nFrame;             /* Number of frames in pFrame list */
  u32 expmask;            /* Binding to these vars invalidates VM */
  SubProgram *pProgram;   /* Linked list of all sub-programs used by VM */
  AuxData *pAuxData;      /* Linked list of auxdata allocations */
#ifdef SQLITE_ENABLE_STMT_SCANSTATUS
  i64 *anExec;            /* Number of times each op has been executed */
  int nScan;              /* Entries in aScan[] */
  ScanStatus *aScan;      /* Scan definitions for sqlite3_stmt_scanstatus() */
#endif
#if defined(SQLITE_BUILDING_FOR_COMDB2)
  int *updCols;           /* list of columns modified in this update */
  Table **tbls;           /* list of tables to be open. */ 
  u16 numTables;
  u16 vTableFlags;        /* Pre-acquire rwlocks / mutexes for certain vtables */
  u16 numVTableLocks;
  char **vTableLocks;
  u16 hasVTables;
  u8 hasScalarFunc;
  char tzname[TZNAME_MAX];/* timezone info for datetime support */
  int dtprec;             /* datetime precision - make it u32 to silence compiler */
  struct timespec tspec;  /* time of prepare, used for stable now() */
  u8 oeFlag;              /* ON CONFLICT action */
  u8 upsertIdx;           /* ON CONFLICT target */
  i64 luaStartTime;       /* start time for Lua running a query */
  i64 luaRows;            /* number of rows processed by Lua */
  double luaSavedCost;    /* saved cost for this Lua thread */
  char **oldColNames;     /* Column names returned by old-sqlite version */
  char **oldColDeclTypes; /* Column decltypes returned by old-sqlite version */
  int oldColCount;        /* Column count (refer: sqlitex)*/
  u8 fingerprint_added;   /* Whether fingerprint was added? Only used in SP code */
  int fdb_warn_this_op;   /* Warn about this opcode which is ineligible for cursor hint */
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
};

/*
** The following are allowed values for Vdbe.magic
*/
#define VDBE_MAGIC_INIT     0x16bceaa5    /* Building a VDBE program */
#define VDBE_MAGIC_RUN      0x2df20da3    /* VDBE is ready to execute */
#define VDBE_MAGIC_HALT     0x319c2973    /* VDBE has completed execution */
#define VDBE_MAGIC_RESET    0x48fa9f76    /* Reset and ready to run again */
#define VDBE_MAGIC_DEAD     0x5606c3c8    /* The VDBE has been deallocated */

/*
** Structure used to store the context required by the 
** sqlite3_preupdate_*() API functions.
*/
struct PreUpdate {
  Vdbe *v;
  VdbeCursor *pCsr;               /* Cursor to read old values from */
  int op;                         /* One of SQLITE_INSERT, UPDATE, DELETE */
  u8 *aRecord;                    /* old.* database record */
  KeyInfo keyinfo;
  UnpackedRecord *pUnpacked;      /* Unpacked version of aRecord[] */
  UnpackedRecord *pNewUnpacked;   /* Unpacked version of new.* record */
  int iNewReg;                    /* Register for new.* values */
  i64 iKey1;                      /* First key value passed to hook */
  i64 iKey2;                      /* Second key value passed to hook */
  Mem *aNew;                      /* Array of new.* values */
  Table *pTab;                    /* Schema object being upated */          
  Index *pPk;                     /* PK index if pTab is WITHOUT ROWID */
};

/*
** Function prototypes
*/
void sqlite3VdbeError(Vdbe*, const char *, ...);
void sqlite3VdbeFreeCursor(Vdbe *, VdbeCursor*);
void sqliteVdbePopStack(Vdbe*,int);
int SQLITE_NOINLINE sqlite3VdbeFinishMoveto(VdbeCursor*);
int sqlite3VdbeCursorMoveto(VdbeCursor**, int*);
int sqlite3VdbeCursorRestore(VdbeCursor*);
u32 sqlite3VdbeSerialTypeLen(u32);
u8 sqlite3VdbeOneByteSerialTypeLen(u8);
u32 sqlite3VdbeSerialType(Mem*, int, u32*);
u32 sqlite3VdbeSerialPut(unsigned char*, Mem*, u32);
#if defined(SQLITE_BUILDING_FOR_COMDB2)
static inline
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
u32 sqlite3VdbeSerialGet(const unsigned char*, u32, Mem*);
void sqlite3VdbeDeleteAuxData(sqlite3*, AuxData**, int, int);

int sqlite2BtreeKeyCompare(BtCursor *, const void *, int, int, int *);
#if defined(SQLITE_BUILDING_FOR_COMDB2)
static inline
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
int sqlite3VdbeIdxKeyCompare(sqlite3*,VdbeCursor*,UnpackedRecord*,int*);
int sqlite3VdbeIdxRowid(sqlite3*, BtCursor*, i64*);
int sqlite3VdbeExec(Vdbe*);
#ifndef SQLITE_OMIT_EXPLAIN
int sqlite3VdbeList(Vdbe*);
#endif
int sqlite3VdbeHalt(Vdbe*);
int sqlite3VdbeChangeEncoding(Mem *, int);
int sqlite3VdbeMemTooBig(Mem*);
int sqlite3VdbeMemCopy(Mem*, const Mem*);
void sqlite3VdbeMemShallowCopy(Mem*, const Mem*, int);
void sqlite3VdbeMemMove(Mem*, Mem*);
int sqlite3VdbeMemNulTerminate(Mem*);
int sqlite3VdbeMemSetStr(Mem*, const char*, int, u8, void(*)(void*));
void sqlite3VdbeMemSetInt64(Mem*, i64);
#if defined(SQLITE_BUILDING_FOR_COMDB2)
int sqlite3VdbeMemSetMasterResolve(Mem*);
int sqlite3VdbeMemSetDatetime(Mem*, dttz_t*, const char *tz);
int sqlite3VdbeMemSetInterval(Mem *pMem, intv_t *tv);
int sqlite3VdbeMemSetDecimal(Mem*, decQuad*);
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
#ifdef SQLITE_OMIT_FLOATING_POINT
# define sqlite3VdbeMemSetDouble sqlite3VdbeMemSetInt64
#else
  void sqlite3VdbeMemSetDouble(Mem*, double);
#endif
void sqlite3VdbeMemSetPointer(Mem*, void*, const char*, void(*)(void*));
void sqlite3VdbeMemInit(Mem*,sqlite3*,u16);
void sqlite3VdbeMemSetNull(Mem*);
#ifndef SQLITE_OMIT_INCRBLOB
void sqlite3VdbeMemSetZeroBlob(Mem*,int);
#else
int sqlite3VdbeMemSetZeroBlob(Mem*,int);
#endif
#if defined(SQLITE_BUILDING_FOR_COMDB2) || defined(SQLITE_DEBUG)
int sqlite3VdbeMemIsRowSet(const Mem*);
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) || defined(SQLITE_DEBUG) */
int sqlite3VdbeMemSetRowSet(Mem*);
int sqlite3VdbeMemMakeWriteable(Mem*);
int sqlite3VdbeMemStringify(Mem*, u8, u8);
i64 sqlite3VdbeIntValue(Mem*);
int sqlite3VdbeMemIntegerify(Mem*);
double sqlite3VdbeRealValue(Mem*);
int sqlite3VdbeBooleanValue(Mem*, int ifNull);
void sqlite3VdbeIntegerAffinity(Mem*);
int sqlite3VdbeMemRealify(Mem*);
int sqlite3VdbeMemNumerify(Mem*);
#if defined(SQLITE_BUILDING_FOR_COMDB2)
int sqlite3VdbeMemCast(Vdbe *, Mem*,u8,u8);
#else /* defined(SQLITE_BUILDING_FOR_COMDB2) */
void sqlite3VdbeMemCast(Mem*,u8,u8);
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
int sqlite3VdbeMemFromBtree(BtCursor*,u32,u32,Mem*);
#if defined(SQLITE_BUILDING_FOR_COMDB2)
int sqlite3VdbeMemDatetimefy(Mem*);
int sqlite3VdbeMemDatetimefyTz(Mem*, const char*);
int sqlite3VdbeMemDecimalfy(Mem*);
int sqlite3VdbeMemIntervalfy(Mem *pMem, int type);
int sqlite3VdbeMemIntervalAndInterval(const Mem *first, const Mem *secnd, int op, Mem * res);
int sqlite3VdbeMemIntervalAndInt(const Mem *a, const Mem *b, int op, Mem * res);
int sqlite3VdbeMemIntAndInterval(const Mem *a, const Mem *b, int op, Mem * res);
int sqlite3VdbeMemDatetimeAndDatetime(const Mem *first, const Mem *secnd, int op, Mem * res);
int sqlite3VdbeMemDatetimeAndInterval(const Mem *a, const Mem *b, int op, Mem * res);
int sqlite3VdbeMemIntervalAndDatetime(const Mem *a, const Mem *b, int op, Mem * res);
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
void sqlite3VdbeMemRelease(Mem *p);
int sqlite3VdbeMemFinalize(Mem*, FuncDef*);
#ifndef SQLITE_OMIT_WINDOWFUNC
int sqlite3VdbeMemAggValue(Mem*, Mem*, FuncDef*);
#endif
#ifndef SQLITE_OMIT_EXPLAIN
const char *sqlite3OpcodeName(int);
#endif
int sqlite3VdbeMemGrow(Mem *pMem, int n, int preserve);
int sqlite3VdbeMemClearAndResize(Mem *pMem, int n);
int sqlite3VdbeCloseStatement(Vdbe *, int);
#ifdef SQLITE_DEBUG
int sqlite3VdbeFrameIsValid(VdbeFrame*);
#endif
void sqlite3VdbeFrameMemDel(void*);      /* Destructor on Mem */
void sqlite3VdbeFrameDelete(VdbeFrame*); /* Actually deletes the Frame */
int sqlite3VdbeFrameRestore(VdbeFrame *);
#ifdef SQLITE_ENABLE_PREUPDATE_HOOK
void sqlite3VdbePreUpdateHook(Vdbe*,VdbeCursor*,int,const char*,Table*,i64,int);
#endif
int sqlite3VdbeTransferError(Vdbe *p);

int sqlite3VdbeSorterInit(sqlite3 *, int, VdbeCursor *);
void sqlite3VdbeSorterReset(sqlite3 *, VdbeSorter *);
void sqlite3VdbeSorterClose(sqlite3 *, VdbeCursor *);
int sqlite3VdbeSorterRowkey(const VdbeCursor *, Mem *);
int sqlite3VdbeSorterNext(sqlite3 *, const VdbeCursor *);
int sqlite3VdbeSorterRewind(const VdbeCursor *, int *);
int sqlite3VdbeSorterWrite(const VdbeCursor *, Mem *);
int sqlite3VdbeSorterCompare(const VdbeCursor *, Mem *, int, int *);

#ifdef SQLITE_DEBUG
  void sqlite3VdbeIncrWriteCounter(Vdbe*, VdbeCursor*);
  void sqlite3VdbeAssertAbortable(Vdbe*);
#else
# define sqlite3VdbeIncrWriteCounter(V,C)
# define sqlite3VdbeAssertAbortable(V)
#endif

#if !defined(SQLITE_OMIT_SHARED_CACHE) 
  void sqlite3VdbeEnter(Vdbe*);
#else
# define sqlite3VdbeEnter(X)
#endif

#if !defined(SQLITE_OMIT_SHARED_CACHE) && SQLITE_THREADSAFE>0
  void sqlite3VdbeLeave(Vdbe*);
#else
# define sqlite3VdbeLeave(X)
#endif

#ifdef SQLITE_DEBUG
void sqlite3VdbeMemAboutToChange(Vdbe*,Mem*);
int sqlite3VdbeCheckMemInvariants(Mem*);
#endif

#ifndef SQLITE_OMIT_FOREIGN_KEY
int sqlite3VdbeCheckFk(Vdbe *, int);
#else
# define sqlite3VdbeCheckFk(p,i) 0
#endif

#ifdef SQLITE_DEBUG
  void sqlite3VdbePrintSql(Vdbe*);
  void sqlite3VdbeMemPrettyPrint(Mem *pMem, char *zBuf);
#endif
#ifndef SQLITE_OMIT_UTF16
  int sqlite3VdbeMemTranslate(Mem*, u8);
  int sqlite3VdbeMemHandleBom(Mem *pMem);
#endif

#ifndef SQLITE_OMIT_INCRBLOB
  int sqlite3VdbeMemExpandBlob(Mem *);
  #define ExpandBlob(P) (((P)->flags&MEM_Zero)?sqlite3VdbeMemExpandBlob(P):0)
#else
  #define sqlite3VdbeMemExpandBlob(x) SQLITE_OK
  #define ExpandBlob(P) SQLITE_OK
#endif

#if defined(SQLITE_BUILDING_FOR_COMDB2)
int convMem2ClientDatetime(Mem *pMem, void *out);
int convMem2ClientDatetimeStr(Mem *pMem, void *out, int outlen, int *outdtsz);
int convDttz2ClientDatetime(const dttz_t *, const char *tzname, void *out, int sqltype);
const char *get_clnt_tz();

int sqliteVdbeMemDecimalBasicArithmetics(Mem *a, Mem *b, int opcode, Mem * res, int flipped);

extern int gbl_decimal_rounding;
extern int dec_ctx_init( void * crt, int type, int rounding);
extern int dfp_conv_check_status( void *pctx, char *from, char *to);

int sqlite3LockStmtTables(sqlite3_stmt *);

Mem* sqlite3GetCachedResultRow(sqlite3_stmt *pStmt, int *nColumns);

#define sqlite3IsFixedLengthSerialType(t) ( (t)<12 || ((unsigned int)t)>=(SQLITE_MAX_U32-2) )
#endif /* defined(SQLITE_BUILDING_FOR_COMDB2) */
#endif /* !defined(SQLITE_VDBEINT_H) */
