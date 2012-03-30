// encoding: utf-8

/*
 * ioposrw.c -
 *
 * Author: dearblue
 */

#include <ruby.h>
#include <ruby/io.h>
#include <ruby/intern.h>


#define NOGVL_FUNC(FUNC) ((rb_blocking_function_t *)(FUNC))

VALUE rb_thread_io_blocking_region(rb_blocking_function_t *, void *, int);


static inline void
rb_sys_fail_path(VALUE path)
{
    rb_sys_fail(NIL_P(path) ? NULL : RSTRING_PTR(path));
}


#ifndef RUBY_WIN32_H
#   include <unistd.h>
#else

#include <windows.h>
#include <io.h>

static ssize_t
pread(int d, void *buf, size_t nbytes, off_t offset)
{
    HANDLE file = (HANDLE)_get_osfhandle(d);
    OVERLAPPED ol;
    memset(&ol, 0, sizeof(ol));
    ol.Offset = offset;
    ol.OffsetHigh = offset >> 32;
    DWORD read;
    DWORD status = ReadFile(file, buf, nbytes, &read, &ol);
    if (status == 0) {
        DWORD err = GetLastError();
        if (err == ERROR_HANDLE_EOF) { return 0; }
        errno = rb_w32_map_errno(err);
        return -1;
    }

    return read;
}

static ssize_t
pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
    HANDLE file = (HANDLE)_get_osfhandle(d);
    OVERLAPPED ol;
    memset(&ol, 0, sizeof(ol));
    ol.Offset = offset;
    ol.OffsetHigh = offset >> 32;
    DWORD wrote;
    DWORD status = WriteFile(file, buf, nbytes, &wrote, &ol);
    if (status == 0) {
        errno = rb_w32_map_errno(GetLastError());
        return -1;
    }

    return wrote;
}

#endif /* ! RUBY_WIN32_H */


struct pread_args
{
    rb_io_t *fptr;
    void *buf;
    size_t nbytes;
    off_t offset;
};

static ssize_t
io_pread_1(struct pread_args *args)
{
    return pread(args->fptr->fd, args->buf, args->nbytes, args->offset);
}

enum {
    READ_BLOCK_SIZE = 1024 * 1024,
};

static inline VALUE
io_pread_all(struct pread_args *args, VALUE buffer)
{
    VALUE tmp = rb_str_buf_new(READ_BLOCK_SIZE);
    rb_str_set_len(tmp, 0);

    if (NIL_P(buffer)) {
        buffer = rb_str_buf_new(0);
        rb_str_set_len(buffer, 0);
    } else {
        StringValue(buffer);
        rb_str_set_len(buffer, 0);
    }

    rb_str_locktmp(buffer);
    args->buf = RSTRING_PTR(tmp);

    for (;;) {
        rb_str_resize(tmp, READ_BLOCK_SIZE);
        args->nbytes = RSTRING_LEN(tmp);
        ssize_t n = (ssize_t)rb_thread_io_blocking_region(NOGVL_FUNC(io_pread_1), args, args->fptr->fd);

        if (n == 0) {
            break;
        } else if (n < 0) {
            rb_str_unlocktmp(buffer);
            rb_str_resize(tmp, 0);
            rb_str_set_len(tmp, 0);
            rb_sys_fail_path(args->fptr->pathv);
        } else {
            args->offset += n;
            rb_str_unlocktmp(buffer);
            rb_str_set_len(tmp, n);
            rb_str_concat(buffer, tmp);
            rb_str_locktmp(buffer);
        }
    }

    rb_str_unlocktmp(buffer);
    rb_str_resize(tmp, 0);
    rb_str_set_len(tmp, 0);

    return buffer;
}

static inline VALUE
io_pread_partial(struct pread_args *args, VALUE buffer)
{
    if (NIL_P(buffer)) {
        buffer = rb_str_buf_new(args->nbytes);
        rb_str_resize(buffer, args->nbytes);
    } else {
        StringValue(buffer);
        rb_str_resize(buffer, args->nbytes);
    }

    rb_str_locktmp(buffer);
    args->buf = RSTRING_PTR(buffer);
    ssize_t n = (ssize_t)rb_thread_io_blocking_region(NOGVL_FUNC(io_pread_1), args, args->fptr->fd);
    rb_str_unlocktmp(buffer);

    if (n < 0) {
        rb_sys_fail_path(args->fptr->pathv);
    }

    rb_str_set_len(buffer, n);
    rb_str_resize(buffer, n);

    return buffer;
}

/*
 * call-seq:
 *  IO#pread(offset [, length [, buffer] ] ) -> string, buffer, or nil
 *
 * 指定位置から読み込むことが出来る。
 *
 * IO#readとの違いは、IO#pos=とIO#readとの間でスレッドスイッチが発生すること
 * による、意図したストリーム位置から#readできないという問題がない。
 *
 * offset:: 読み込むデータのファイル位置をオクテット数で指定する。nilは許さない。
 * length:: 読み込むデータ長をオクテット数で指定する。nilを指定すると、ファイルの最後まで読み込む。
 * buffer:: 読み込み先バッファとして指定する。指定しない場合は、IO#pread内部で勝手に用意する。
 * 戻り値:: 正常に読み込んだ場合、buffer、もしくはStringインスタンスを返す。
 * 例外:: いろいろ。ArgumentErrorだったり、Errno::EXXXだったり、これらにとどまらない。
 */
static VALUE
io_pread(int argc, VALUE argv[], VALUE io)
{
    VALUE offset, length = Qnil, buffer = Qnil;

    rb_scan_args(argc, argv, "12", &offset, &length, &buffer);

    struct pread_args args;

    GetOpenFile(io, args.fptr);

    rb_io_check_char_readable(args.fptr);
    rb_io_check_closed(args.fptr);

    args.offset = NUM2OFFT(offset);

    if (NIL_P(length)) {
        buffer = io_pread_all(&args, buffer);
    } else {
        args.nbytes = NUM2SIZET(length);
        buffer = io_pread_partial(&args, buffer);
    }

    OBJ_TAINT(buffer);

    return buffer;
}


struct pwrite_args
{
    rb_io_t *fptr;
    const void *buf;
    size_t nbytes;
    off_t offset;
};

static ssize_t
io_pwrite_1(struct pwrite_args *args)
{
    return pwrite(args->fptr->fd, args->buf, args->nbytes, args->offset);
}

/*
 * call-seq:
 *  IO#pwrite(offset, buffer) -> wrote size
 *
 * 指定位置への書き込みを行う。
 */
static VALUE
io_pwrite(VALUE io, VALUE offset, VALUE buffer)
{
    rb_secure(4);

    StringValue(buffer);

    struct pwrite_args args;

    args.nbytes = RSTRING_LEN(buffer);
    args.offset = NUM2OFFT(offset);

    GetOpenFile(io, args.fptr);

    rb_io_check_closed(args.fptr);

    rb_str_locktmp(buffer);
    args.buf = RSTRING_PTR(buffer);
    ssize_t n = (ssize_t)rb_thread_io_blocking_region(NOGVL_FUNC(io_pwrite_1), &args, args.fptr->fd);
    rb_str_unlocktmp(buffer);

    if (n < 0) {
        rb_sys_fail_path(args.fptr->pathv);
    }

    return SIZET2NUM(n);
}


void
Init_ioposrw(void)
{
    rb_define_method(rb_cIO, "pread", RUBY_METHOD_FUNC(io_pread), -1);
    rb_define_method(rb_cIO, "pwrite", RUBY_METHOD_FUNC(io_pwrite), 2);
}
