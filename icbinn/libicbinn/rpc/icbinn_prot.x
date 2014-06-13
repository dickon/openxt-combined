const ICBINN_PROT_PORT          = 4878;
const ICBINN_PROT_MAXPATHLEN    = 4096;
const ICBINN_PROT_MAXNAMELEN    = 1024;
const ICBINN_PROT_MAXDATA	= 65536;
const ICBINN_PROT_FHSIZE	   = 32;


typedef string icbinn_prot_path<ICBINN_PROT_MAXPATHLEN>;
typedef string icbinn_prot_filename<ICBINN_PROT_MAXNAMELEN>;
typedef int icbinn_prot_fd;
typedef int icbinn_prot_errno;

const	ICBINN_PROT_RDONLY	= 0;
const	ICBINN_PROT_WRONLY	= 1;
const	ICBINN_PROT_RDWR	= 2;
const	ICBINN_PROT_CREAT	= 0100;
const 	ICBINN_PROT_TRUNC	= 01000;

enum icbinn_prot_ltype {
	ICBINN_PROT_LTYPE_RDLCK = 0,
	ICBINN_PROT_LTYPE_WRLCK = 1,
	ICBINN_PROT_LTYPE_UNLCK = 2
};

enum icbinn_prot_type {
        ICBINN_PROT_FILE= 0,
        ICBINN_PROT_DIRECTORY=1,
	ICBINN_PROT_LINK=2,
        ICBINN_PROT_UNKNOWN=3
};

enum icbinn_prot_randsrc {
        ICBINN_PROT_RANDOM = 0,
        ICBINN_PROT_URANDOM = 1
};

enum icbinn_prot_stat {
        ICBINN_PROT_OK= 0,
        ICBINN_PROT_ERROR=1
};

union icbinn_prot_errnostat switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
     void;
default:
        icbinn_prot_errno errno_code;
};

struct icbinn_prot_readdirargs {
	icbinn_prot_path dir;
	unsigned int offset;
	unsigned int count;
};

struct icbinn_prot_entry {
	icbinn_prot_filename name;
	enum icbinn_prot_type type;
	icbinn_prot_entry *nextentry;
};

struct icbinn_prot_dirlist {
	icbinn_prot_entry *entries;
	bool eof;
};

union icbinn_prot_readdirres switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
	icbinn_prot_dirlist reply;
default:
	void;
};


struct icbinn_prot_openargs {
	icbinn_prot_path 	name;
	int	mode;
};

struct icbinn_prot_openokres {
	icbinn_prot_fd	fd;
};


union icbinn_prot_openres switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
	icbinn_prot_openokres reply;
case ICBINN_PROT_ERROR:
        icbinn_prot_errno errno_code;
default:
        void;
};	


struct icbinn_prot_statokres {
	unsigned hyper size;
	enum  icbinn_prot_type type;
};

union icbinn_prot_statres switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
	icbinn_prot_statokres reply;
case ICBINN_PROT_ERROR:
        icbinn_prot_errno errno_code;
default:
        void;
};	


struct icbinn_prot_preadargs {
	icbinn_prot_fd fd;
	unsigned hyper offset;
	unsigned hyper count;
};

struct icbinn_prot_preadokres {
        opaque data<ICBINN_PROT_MAXDATA>;
};


union icbinn_prot_preadres switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
	icbinn_prot_preadokres reply;
case ICBINN_PROT_ERROR:
        icbinn_prot_errno errno_code;
default:
	void;
};	


struct icbinn_prot_pwriteargs {
        icbinn_prot_fd  fd;
        unsigned hyper offset;
        opaque data<ICBINN_PROT_MAXDATA>;
};

struct icbinn_prot_lockargs {
	icbinn_prot_fd fd;
	icbinn_prot_ltype type;	
};

struct icbinn_prot_renameargs {
	icbinn_prot_path from;
	icbinn_prot_path to;
};


struct icbinn_prot_ftruncateargs {
        icbinn_prot_fd  fd;
        unsigned hyper length;
};


struct icbinn_prot_randargs {
	unsigned hyper count;
	enum icbinn_prot_randsrc src;
};

struct icbinn_prot_fallocateargs {
       icbinn_prot_fd fd;
       int mode;
       unsigned hyper offset;
       unsigned hyper length;
};

struct icbinn_prot_randokres {
        opaque data<ICBINN_PROT_MAXDATA>;
};

struct icbinn_prot_symlinkargs {
	icbinn_prot_path oldpath;
	icbinn_prot_path newpath;
};

struct icbinn_prot_readlinkargs {
	icbinn_prot_path path;
	unsigned hyper bufsz;
};

struct icbinn_prot_readlinkokres {
        opaque data<ICBINN_PROT_MAXDATA>;
};

union icbinn_prot_readlinkres switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
	icbinn_prot_readlinkokres reply;
case ICBINN_PROT_ERROR:
        icbinn_prot_errno errno_code;
default:
	void;
};

union icbinn_prot_randres switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
	icbinn_prot_randokres reply;
default:
	void;
};

struct icbinn_prot_statfsokres {
       unsigned hyper bsize;
       unsigned hyper blocks;
       unsigned hyper bfree;
       unsigned hyper bavail;
};

union icbinn_prot_statfsres switch (icbinn_prot_stat status) {
case ICBINN_PROT_OK:
     icbinn_prot_statfsokres reply;
case ICBINN_PROT_ERROR:
     icbinn_prot_errno errno_code;
default:
	void;
};


program ICBINN_PROT_PROGRAM {
	version ICBINN_PROT_VERSION {
	void 			ICBINN_PROT_NULL(void) = 0;
	icbinn_prot_readdirres	ICBINN_PROT_READDIR(icbinn_prot_readdirargs) = 1;
	icbinn_prot_statres	ICBINN_PROT_STAT(icbinn_prot_path) = 2;
	icbinn_prot_openres	ICBINN_PROT_OPEN(icbinn_prot_openargs) = 3;
	icbinn_prot_errnostat	ICBINN_PROT_LOCK(icbinn_prot_lockargs) = 4;
	icbinn_prot_preadres	ICBINN_PROT_PREAD(icbinn_prot_preadargs) = 5;
	icbinn_prot_errnostat	ICBINN_PROT_PWRITE(icbinn_prot_pwriteargs) = 6;
	icbinn_prot_errnostat	ICBINN_PROT_CLOSE(icbinn_prot_fd) = 7;
	icbinn_prot_errnostat	ICBINN_PROT_MKDIR(icbinn_prot_path) = 8;
	icbinn_prot_errnostat	ICBINN_PROT_RMDIR(icbinn_prot_path) = 9;
	icbinn_prot_errnostat	ICBINN_PROT_RENAME(icbinn_prot_renameargs) = 10;
	icbinn_prot_errnostat	ICBINN_PROT_UNLINK(icbinn_prot_path) = 11;
	icbinn_prot_errnostat	ICBINN_PROT_FTRUNCATE(icbinn_prot_ftruncateargs) = 12;
	icbinn_prot_randres	ICBINN_PROT_RAND(icbinn_prot_randargs) = 13;
        icbinn_prot_errnostat   ICBINN_PROT_FALLOCATE(icbinn_prot_fallocateargs) = 14;
	icbinn_prot_errnostat   ICBINN_PROT_SYMLINK(icbinn_prot_symlinkargs) = 15;
	icbinn_prot_readlinkres ICBINN_PROT_READLINK(icbinn_prot_readlinkargs) = 16;
	icbinn_prot_statfsres   ICBINN_PROT_STATFS(icbinn_prot_path) = 17;
	icbinn_prot_errnostat   ICBINN_PROT_FSYNC(icbinn_prot_fd) = 18;
	} = 1;
} = 1827368391;
