// encoding: utf-8

/*
 * ioposrw.c -
 *
 * Author : dearblue <dearblue@users.sourceforge.jp>
 * License : 2-clause BSD License
 * Project Page : http://sourceforge.jp/projects/rutsubo/
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


/* ---- */

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

    if (RSTRING_LEN(buffer) == 0) {
        return Qnil;
    } else {
        return buffer;
    }
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

    if (n == 0) {
        return Qnil;
    } else if (n < 0) {
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
 * - offset : 読み込み位置をバイト値で指定する。
 *
 *   これはストリーム先端からの絶対位置となる。
 * - length (省略可能) : 読み込むデータ長をバイト値で指定する。
 *
 *   nil を指定すると、ファイルの最後まで読み込む。
 * - buffer (省略可能) : 読み込み先バッファとして指定する。
 *
 *   指定しない場合は、IO#pread内部で勝手に用意する。
 *
 *   buffer は変更可能 (frozen ではない String) なインスタンスである必要がある。
 * - 戻り値 :
 *
 *   正常に読み込んだ場合、データが格納されたbuffer、もしくはStringインスタンスを返す。
 *
 *   ファイルサイズを越えた位置を offset に与えた場合、nil を返す。
 * - 例外 : いろいろ。
 *
 *   Errno::EXXX だったり、SEGV だったり、これらにとどまらない。
 */
static VALUE
io_pread(int argc, VALUE argv[], VALUE io)
{
    VALUE offset, length = Qnil, buffer = Qnil;

    rb_scan_args(argc, argv, "12", &offset, &length, &buffer);

    struct pread_args args;

    args.offset = NUM2OFFT(offset);

    GetOpenFile(io, args.fptr);

    rb_io_check_char_readable(args.fptr);
    rb_io_check_closed(args.fptr);

    if (NIL_P(length)) {
        buffer = io_pread_all(&args, buffer);
    } else {
        args.nbytes = NUM2SIZET(length);
        if (args.nbytes > 0) {
            buffer = io_pread_partial(&args, buffer);
        } else {
            if (NIL_P(buffer)) {
                buffer = rb_str_new(0, 0);
            } else {
                rb_str_set_len(buffer, 0);
            }
        }
    }

    if (!NIL_P(buffer)) {
        OBJ_TAINT(buffer);
    }

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
 *
 * - 戻り値 : 書き込んだデータ量がバイト値で返る。
 * - 引数 offset : 書き込み位置をバイト値で指定する。これはストリーム先端からの絶対位置となる。
 * - 引数 buffer : 書き込みたいデータが格納されたStringインスタンスを指定する。
 * - 例外 : Errno::EXXX や、SEGV など
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


/* ---- */

static VALUE cStringIO;
static ID IDstring, IDis_closed_read, IDis_closed_write;


static inline VALUE
strio_getstr(VALUE io)
{
    return rb_funcall3(io, IDstring, 0, 0);
}

static inline VALUE
strio_getread(VALUE io)
{
    if (RTEST(rb_funcall3(io, IDis_closed_read, 0, 0))) {
	rb_raise(rb_eIOError, "not opened for reading");
    }

    return strio_getstr(io);
}

static inline VALUE
strio_getwrite(VALUE io)
{
    if (RTEST(rb_funcall3(io, IDis_closed_write, 0, 0))) {
	rb_raise(rb_eIOError, "not opened for writing");
    }

    return strio_getstr(io);
}


/*
 * call-seq:
 *  StringIO#pread(offset, length = nil, buffer = nil) -> nil, buffer or string
 *
 * IO#pread と同じ。
 *
 * ただし、例外の搬出は挙動が異なる。このあたりは将来的に (200年後くらいには) 改善されるだろう。
 */
static VALUE
strio_pread(int argc, VALUE argv[], VALUE io)
{
    VALUE offset, length = Qnil, buffer = Qnil;

    rb_scan_args(argc, argv, "12", &offset, &length, &buffer);

    VALUE str = strio_getread(io);

    VALUE tmp = Qnil;
    if (NIL_P(buffer)) {
        tmp = buffer = rb_str_buf_new(0);
    }

    ssize_t off = NUM2SSIZET(offset);
    if (off < 0) { NUM2SIZET(offset); } // 例外の発生は任せた
    ssize_t lenmax = RSTRING_LEN(str);
    ssize_t len = NIL_P(length) ? lenmax : NUM2SSIZET(length);
    if (len < 0) { NUM2SIZET(SSIZET2NUM(len)); } // 例外発生!

    if (len == 0) {
        if (NIL_P(buffer)) {
            return rb_str_new(0, 0);
        } else {
            rb_str_modify(buffer);
            rb_str_set_len(buffer, 0);
            return buffer;
        }
    }

    if (off >= lenmax) { return Qnil; }

    if (off + len < 0) {
        rb_raise(rb_eRangeError, "offset and length are too large");
    }

    if (off + len > lenmax) { len = lenmax - off; }

    StringValue(buffer);
    rb_str_modify(buffer);
    rb_str_resize(buffer, len);
    rb_str_set_len(buffer, len);

    memcpy(RSTRING_PTR(buffer), RSTRING_PTR(str) + off, len);

    OBJ_INFECT(buffer, str);

    return buffer;
}

/*
 * call-seq:
 *  StringIO#pwrite(offset, buffer) -> integer
 *
 * IO#pwrite と同じ。
 *
 * ただし、例外の搬出は挙動が異なる。このあたりは将来的に (200年後くらいには) 改善されるだろう。
 */
static VALUE
strio_pwrite(VALUE io, VALUE offset, VALUE buffer)
{
    rb_secure(4);

    VALUE str = strio_getwrite(io);

    ssize_t off = NUM2SSIZET(offset);
    if (off < 0) { NUM2SIZET(offset); } // 例外発生!
    StringValue(buffer);
    ssize_t len = RSTRING_LEN(buffer);
    if (len < 0) { NUM2SSIZET(SIZET2NUM(len)); } // 例外発生!
    if (off + len < 0) {
        rb_raise(rb_eRangeError,
                 "offset and buffer size are too large");
    }

    if (off + len > RSTRING_LEN(str)) {
        ssize_t p = RSTRING_LEN(str);
        rb_str_modify(buffer);
        rb_str_resize(str, off + len);
        rb_str_set_len(str, off + len);
        if (off > p) {
            memset(RSTRING_PTR(str) + p, 0, off - p);
        }
    } else {
        rb_str_modify(str);
    }

    memcpy(RSTRING_PTR(str) + off, RSTRING_PTR(buffer), len);

    return SSIZET2NUM(len);
}


/*
 * call-seq:
 *  IO.ioposrw_enable_stringio_extend -> nil
 *
 * StringIO にも同様の StringIO#pread / StringIO#pwrite を提供する。
 *
 * このメソッドは、stringio が require されていなければ勝手に読み込む。
 *
 * 有効にしたら無効に出来ないことに注意。
 *
 * <tt>$SAFE < 2</tt> でなければ SecurityError 例外が発生する。
 */
static VALUE
io_s_enable_stringio_extend(VALUE obj)
{
    static int oyobi_de_nai = Qfalse;

    if (!oyobi_de_nai) {
        rb_secure(2);

        rb_require("stringio");
        cStringIO = rb_const_get(rb_cObject, rb_intern("StringIO"));
        rb_define_method(cStringIO, "pread", RUBY_METHOD_FUNC(strio_pread), -1);
        rb_define_method(cStringIO, "pwrite", RUBY_METHOD_FUNC(strio_pwrite), 2);

        IDstring          = rb_intern_const("string");
        IDis_closed_read  = rb_intern_const("closed_read?");
        IDis_closed_write = rb_intern_const("closed_write?");

        oyobi_de_nai = Qtrue;
    }

    return Qnil;
}


/*
 * Document-class: IO
 *
 * この拡張ライブラリは IO#pread / IO#pwrite を提供する。
 *
 * <tt>require "ioposrw"</tt> によって利用できるようになる。
 */

/*
 * Document-class: StringIO
 *
 * StringIO#pread / StringIO#pwrite を提供する。
 *
 * StringIO#pread / StringIO#pwrite は <tt>require "ioposrw"</tt> しただけでは有効になっていない。
 *
 * *IO.ioposrw_enable_stringio_extend* を呼ぶ必要がある。このメソッドは、<tt>require "stringio"</tt> されていなければ勝手に読み込む。
 */

// rdoc 用に認識させるために使う
#define RDOC(...)

void
Init_ioposrw(void)
{
    RDOC(cIO = rb_define_class("IO", rb_cObject));
    RDOC(cStringIO = rb_define_class("StringIO", rb_cObject));
    rb_define_method(rb_cIO, "pread", RUBY_METHOD_FUNC(io_pread), -1);
    rb_define_method(rb_cIO, "pwrite", RUBY_METHOD_FUNC(io_pwrite), 2);
    rb_define_singleton_method(rb_cIO, "ioposrw_enable_stringio_extend", RUBY_METHOD_FUNC(io_s_enable_stringio_extend), 0);
}
