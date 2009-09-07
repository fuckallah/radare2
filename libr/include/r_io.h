#ifndef _LIB_R_IO_H_
#define _LIB_R_IO_H_

#include "r_types.h"
#include "list.h"

#define R_IO_READ 0
#define R_IO_WRITE 1
#define R_IO_RDWR 2 // ???

#define R_IO_NFDS 32

#define R_IO_SEEK_SET 0
#define R_IO_SEEK_CUR 1
#define R_IO_SEEK_END 2

#define IO_MAP_N 128
struct r_io_map_t {
        int fd;
	int flags;
        ut64 delta;
        ut64 from;
        ut64 to;
        struct list_head list;
};

/* stores write and seek changes */
#define R_IO_UNDOS 64
struct r_io_undo_t {
	struct list_head undo_w_list;
	int w_init;
	int w_lock;
	ut64 seek[R_IO_UNDOS];
	int fd[R_IO_UNDOS];
	int idx;
	int lim;
};

struct r_io_t {
	int fd;
	ut64 seek;
	char *redirect;
	/* write mask */
	void (*printf)(const char *str, ...);
	int write_mask_fd;
	ut8 *write_mask_buf;
	int write_mask_len;
	struct r_io_handle_t *plugin;
	struct list_head io_list;
	ut64 last_align;
	struct list_head sections;
	/* maps */
	struct list_head maps;
        struct list_head desc;
};

//struct r_io_handle_fd_t {
// ... store io changes here
//};

struct r_io_handle_t {
        void *handle;
        char *name;
        char *desc;
        void *widget;
        int (*init)();
	struct r_io_undo_t undo;
        struct debug_t *debug; // ???
        int (*system)(struct r_io_t *io, int fd, const char *);
        int (*open)(struct r_io_t *io, const char *, int rw, int mode);
        int (*read)(struct r_io_t *io, int fd, ut8 *buf, int count);
        ut64 (*lseek)(struct r_io_t *io, int fildes, ut64 offset, int whence);
        int (*write)(struct r_io_t *io, int fd, const ut8 *buf, int count);
        int (*close)(struct r_io_t *io, int fd);
        int (*handle_open)(struct r_io_t *io, const char *);
        //int (*handle_fd)(struct r_io_t *, int);
	int fds[R_IO_NFDS];
};

struct r_io_list_t {
	struct r_io_handle_t *plugin;
	struct list_head list;
};

/* io/handle.c */
R_API struct r_io_t *r_io_new();
R_API struct r_io_t *r_io_free(struct r_io_t *io);
R_API int r_io_handle_init(struct r_io_t *io);
R_API int r_io_handle_open(struct r_io_t *io, int fd, struct r_io_handle_t *plugin);
R_API int r_io_handle_close(struct r_io_t *io, int fd, struct r_io_handle_t *plugin);
R_API int r_io_handle_generate(struct r_io_t *io);
R_API int r_io_handle_add(struct r_io_t *io, struct r_io_handle_t *plugin);
R_API int r_io_handle_list(struct r_io_t *io);
// TODO: _del ??
R_API struct r_io_handle_t *r_io_handle_resolve(struct r_io_t *io, const char *filename);
R_API struct r_io_handle_t *r_io_handle_resolve_fd(struct r_io_t *io, int fd);

/* io/io.c */
R_API struct r_io_t* r_io_init(struct r_io_t *io);
R_API int r_io_set_write_mask(struct r_io_t *io, const ut8 *buf, int len);
R_API int r_io_open(struct r_io_t *io, const char *file, int flags, int mode);
R_API int r_io_open_as(struct r_io_t *io, const char *urihandler, const char *file, int flags, int mode);
R_API int r_io_redirect(struct r_io_t *io, const char *file);
R_API int r_io_set_fd(struct r_io_t *io, int fd);
R_API int r_io_read(struct r_io_t *io, ut8 *buf, int len);
R_API int r_io_read_at(struct r_io_t *io, ut64 addr, ut8 *buf, int len);
R_API ut64 r_io_read_i(struct r_io_t *io, ut64 addr, int sz, int endian);
R_API int r_io_write(struct r_io_t *io, const ut8 *buf, int len);
R_API int r_io_write_at(struct r_io_t *io, ut64 addr, const ut8 *buf, int len);
R_API ut64 r_io_seek(struct r_io_t *io, ut64 offset, int whence);
R_API int r_io_system(struct r_io_t *io,  const char *cmd);
R_API int r_io_close(struct r_io_t *io, int fd);
R_API ut64 r_io_size(struct r_io_t *io, int fd);

/* io/map.c */
R_API void r_io_map_init(struct r_io_t *io);
R_API int r_io_map_add(struct r_io_t *io, int fd, int flags, ut64 delta, ut64 offset, ut64 size);
R_API int r_io_map_del(struct r_io_t *io, int fd);
R_API int r_io_map_list(struct r_io_t *io);
R_API int r_io_map(struct r_io_t *io, const char *file, ut64 offset);
R_API int r_io_map_read_at(struct r_io_t *io, ut64 off, ut8 *buf, int len);
//R_API int r_io_map_read_rest(struct r_io_t *io, ut64 off, ut8 *buf, ut64 len);
R_API int r_io_map_write_at(struct r_io_t *io, ut64 off, const ut8 *buf, int len);

/* sections */
struct r_io_section_t {
	char comment[256];
	ut64 from;
	ut64 to;
	ut64 vaddr;
	ut64 paddr; // offset on disk
	int rwx;
	struct list_head list;
};

// TODO: rename SECTION to PERMISION or so
enum {
	R_IO_SECTION_R = 4,
	R_IO_SECTION_W = 2,
	R_IO_SECTION_X = 1,
};

R_API int r_io_section_rm(struct r_io_t *io, int idx);
R_API void r_io_section_add(struct r_io_t *io, ut64 from, ut64 to, ut64 vaddr, ut64 physical, int rwx, const char *comment);
R_API void r_io_section_set(struct r_io_t *io, ut64 from, ut64 to, ut64 vaddr, ut64 physical, int rwx, const char *comment);
R_API void r_io_section_list(struct r_io_t *io, ut64 addr, int rad);
R_API struct r_io_section_t * r_io_section_get(struct r_io_t *io, ut64 addr);
R_API void r_io_section_list_visual(struct r_io_t *io, ut64 seek, ut64 len);
R_API ut64 r_io_section_get_vaddr(struct r_io_t *io, ut64 addr);
R_API struct r_io_section_t * r_io_section_get_i(struct r_io_t *io, int idx);
R_API void r_io_section_init(struct r_io_t *io);
R_API int r_io_section_overlaps(struct r_io_t *io, struct r_io_section_t *s);
R_API ut64 r_io_section_align(struct r_io_t *io, ut64 addr, ut64 vaddr, ut64 paddr);

struct r_io_desc_t {
	int fd;
	int flags;
        char name[4096];
	struct r_io_handle_t *handle;
        struct list_head list;
};

R_API int r_io_desc_init(struct r_io_t *io);
R_API int r_io_desc_add(struct r_io_t *io, int fd, const char *file, int flags, struct r_io_handle_t *handle);
R_API int r_io_desc_del(struct r_io_t *io, int fd);
R_API struct r_io_desc_t *r_io_desc_get(struct r_io_t *io, int fd);
R_API int r_io_desc_generate(struct r_io_t *io);
#if 0
#define CB_READ int (*cb_read)(struct r_io_t *user, int pid, ut64 addr, ut8 *buf, int len)
#define CB_WRITE int (*cb_write)(struct r_io_t *user, int pid, ut64 addr, const ut8 *buf, int len)
#define CB_IO int (*cb_io)(void *user, CB_READ, CB_WRITE)
R_API int r_io_hook(struct r_io_t *io, CB_IO);
#endif
/* plugins */
extern struct r_io_handle_t r_io_plugin_dbg;
extern struct r_io_handle_t r_io_plugin_ptrace;

#endif
