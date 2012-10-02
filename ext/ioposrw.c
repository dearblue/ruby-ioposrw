/* encoding:utf-8 */

/*
 * ioposrw.c -
 *
 * - Author: dearblue <dearblue@users.sourceforge.jp>
 * - License: 2-clause BSD License
 * - Project Page: http://sourceforge.jp/projects/rutsubo/
 */

#include <ruby.h>
#include <ruby/io.h>
#include <ruby/intern.h>


#define NOGVL_FUNC(FUNC) ((rb_blocking_function_t *)(FUNC))

#define FUNCALL(RECV, METHOD, ...)                                  \
    ({                                                              \
        VALUE _params[] = { __VA_ARGS__ };                          \
        rb_funcall3((RECV), (METHOD),                               \
                    sizeof(_params) / sizeof(_params[0]), _params); \
    })                                                              \


#ifndef RUBY_WIN32_H
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/stat.h>
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
io_pread_pread(struct pread_args *args)
{
    return pread(args->fptr->fd, args->buf, args->nbytes, args->offset);
}

static ssize_t
io_pread_try(struct pread_args *args)
{
    return (ssize_t)rb_thread_blocking_region(NOGVL_FUNC(io_pread_pread),
                                              args, RUBY_UBF_IO, 0);
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
    } else {
        StringValue(buffer);
    }
    rb_str_set_len(buffer, 0);

    rb_str_locktmp(buffer);
    args->buf = RSTRING_PTR(tmp);

    for (;;) {
        rb_str_resize(tmp, READ_BLOCK_SIZE);
        args->nbytes = RSTRING_LEN(tmp);
        ssize_t n = (ssize_t)rb_ensure(RUBY_METHOD_FUNC(io_pread_try), (VALUE)args, rb_str_unlocktmp, buffer);

        if (n == 0) {
            break;
        } else if (n < 0) {
            rb_str_resize(tmp, 0);
            rb_str_set_len(tmp, 0);
            rb_sys_fail(NIL_P(args->fptr->pathv) ? NULL : RSTRING_PTR(args->fptr->pathv));
        } else {
            args->offset += n;
            rb_str_set_len(tmp, n);
            rb_str_concat(buffer, tmp);
            rb_str_locktmp(buffer);
        }
    }

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
    } else {
        StringValue(buffer);
    }
    rb_str_resize(buffer, args->nbytes);

    rb_str_locktmp(buffer);
    args->buf = RSTRING_PTR(buffer);
    ssize_t n = (ssize_t)rb_ensure(RUBY_METHOD_FUNC(io_pread_try), (VALUE)args, rb_str_unlocktmp, buffer);

    if (n == 0) {
        return Qnil;
    } else if (n < 0) {
        rb_sys_fail(NIL_P(args->fptr->pathv) ? NULL : RSTRING_PTR(args->fptr->pathv));
    }

    rb_str_set_len(buffer, n);
    rb_str_resize(buffer, n);

    return buffer;
}

/*
 * call-seq:
 * IO#pread(offset [, length [, buffer] ] ) -> string, buffer, or nil
 *
 * 指定位置から読み込むことが出来ます。
 *
 * IO#read との違いは、IO#pos= と IO#read との間でスレッドスイッチが発生することによる、
 * 意図したストリーム位置から #read できないという問題がないことです。
 *
 * [offset]     読み込み位置をバイト値で指定します。
 *
 *              これはストリーム先端からの絶対位置となります。
 *
 *              負の整数を指定した場合、ファイル終端からの相対位置になります。
 *              File#seek に SEEK_END で負の値を指定した場合と同じです。
 *              SEEK_END と 0 を指定する挙動は出来ないことに注意してください。
 *
 * [length (省略可能)]
 *              読み込むデータ長をバイト値で指定します。
 *
 *              省略もしくは nil を指定すると、ファイルの最後まで読み込みます。
 *
 * [buffer (省略可能)]
 *              読み込み先バッファとして指定します。
 *
 *              省略もしくは nil を指定すると、IO#pread 内部で勝手に用意します。
 *
 *              buffer は変更可能 (frozen ではない String) なインスタンスである必要があります。
 *
 * [RETURN]     正常に読み込んだ場合、データが格納された buffer、もしくは String インスタンスを返します。
 *
 *              ファイルサイズを越えた位置を offset に与えた場合、nil を返します。
 *
 * [EXCEPTIONS] いろいろ。
 *
 *              Errno::EXXX だったり、SEGV だったり、これらにとどまりません。
 *
 * 処理中は呼び出しスレッドのみが停止します (GVL を開放します)。
 * その間別スレッドから buffer オブジェクトの変更はできなくなります
 * (厳密に言うと、内部バッファの伸縮を伴う操作が出来なくなるだけです)。
 *
 *
 * === 注意点
 * Windows においては、#pread 後に IO 位置が更新されます (IO#pos= と IO#read した後と同じ位置)。
 * これは Windows 上の制限となります。
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

    {
        struct stat st;
        if (args.offset < 0 && fstat(args.fptr->fd, &st) == 0) {
            args.offset += st.st_size;
        }
        if (args.offset < 0) {
            rb_raise(rb_eArgError, "file offset too small");
        }
    }

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
io_pwrite_pwrite(struct pwrite_args *args)
{
    return pwrite(args->fptr->fd, args->buf, args->nbytes, args->offset);
}

static ssize_t
io_pwrite_try(struct pwrite_args *args)
{
    return (ssize_t)rb_thread_blocking_region(NOGVL_FUNC(io_pwrite_pwrite), args, RUBY_UBF_IO, 0);
}

/*
 * call-seq:
 * IO#pwrite(offset, buffer) -> wrote size
 *
 * 指定位置への書き込みを行います。
 *
 * [RETURN]     書き込んだデータ量がバイト値で返ります。
 *
 * [offset]     書き込み位置をバイト値で指定します。これはストリーム先端からの絶対位置となります。
 *
 * [buffer]     書き込みたいデータが格納されたStringインスタンスを指定します。
 *
 * [EXCEPTIONS] Errno::EXXX や、SEGV など。
 *
 * 処理中は呼び出しスレッドのみが停止します (GVL を開放します)。
 * また、別スレッドから buffer オブジェクトの変更はできなくなります
 * (厳密に言うと、内部バッファの伸縮を伴う操作が出来なくなるだけです)。
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

    {
        struct stat st;
        if (Qfalse && args.offset < 0 && fstat(args.fptr->fd, &st) == 0) {
            args.offset += st.st_size;
        }
        if (args.offset < 0) {
            rb_raise(rb_eArgError, "file offset too small");
        }
    }

    rb_str_locktmp(buffer);
    args.buf = RSTRING_PTR(buffer);
    ssize_t n = (ssize_t)rb_ensure(RUBY_METHOD_FUNC(io_pwrite_try), (VALUE)&args, rb_str_unlocktmp, buffer);

    if (n < 0) {
        rb_sys_fail(NIL_P(args.fptr->pathv) ? NULL : RSTRING_PTR(args.fptr->pathv));
    }

    return SIZET2NUM(n);
}


/* ---- */


static VALUE cStringIO;
static ID IDstring, IDis_closed_read, IDis_closed_write;


static inline VALUE
strio_getstr(VALUE io)
{
    return FUNCALL(io, IDstring);
}

static inline VALUE
strio_getread(VALUE io)
{
    if (RTEST(FUNCALL(io, IDis_closed_read))) {
	rb_raise(rb_eIOError, "not opened for reading");
    }

    return strio_getstr(io);
}

static inline VALUE
strio_getwrite(VALUE io)
{
    if (RTEST(FUNCALL(io, IDis_closed_write))) {
	rb_raise(rb_eIOError, "not opened for writing");
    }

    return strio_getstr(io);
}


/*
 * call-seq:
 * StringIO#pread(offset [, length [, buffer] ] ) -> nil, buffer or string
 *
 * IO#pread と同じ。
 *
 * ただし、例外の搬出は挙動が異なります。このあたりは将来的に (200年後くらいには) 改善される見込みです。
 *
 * 処理中は Ruby のすべてのスレッドが停止します (GVL を開放しないため)。
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

    ssize_t lenmax = RSTRING_LEN(str);
    ssize_t len = NIL_P(length) ? lenmax : NUM2SSIZET(length);

    ssize_t off = NUM2SSIZET(offset);
    if (off < 0) {
        off += lenmax;
    }

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
        rb_raise(rb_eRangeError, "offset and length are too small or too large");
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
 * StringIO#pwrite(offset, buffer) -> integer
 *
 * IO#pwrite と同じ。
 *
 * ただし、例外の搬出は挙動が異なります。このあたりは将来的に (200年後くらいには) 改善される見込みです。
 *
 * 処理中は Ruby のすべてのスレッドが停止します (GVL を開放しないため)。
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
 * IO.ioposrw_enable_stringio_extend -> nil
 *
 * StringIO にも同様の StringIO#pread / StringIO#pwrite を提供します。
 *
 * このメソッドは、stringio が require されていなければ勝手に読み込みます。
 *
 * 有効にしたら無効に出来ないことに注意してください (現段階の実装ではセーフレベルをまともに扱っていないため、セキュリティリスクがあります)。
 *
 * <tt>$SAFE < 2</tt> でなければ SecurityError 例外が発生します。
 */
static VALUE
io_s_enable_stringio_extend(VALUE obj)
{
    static int oyobi_de_nai = Qtrue;

    if (oyobi_de_nai) {
        rb_secure(2);

        rb_require("stringio");
        cStringIO = rb_const_get(rb_cObject, rb_intern("StringIO"));
        rb_define_method(cStringIO, "pread", RUBY_METHOD_FUNC(strio_pread), -1);
        rb_define_method(cStringIO, "pwrite", RUBY_METHOD_FUNC(strio_pwrite), 2);

        IDstring          = rb_intern_const("string");
        IDis_closed_read  = rb_intern_const("closed_read?");
        IDis_closed_write = rb_intern_const("closed_write?");

        oyobi_de_nai = Qfalse;
    }

    return Qnil;
}


/*
 * Document-class: IO
 *
 * <b>現段階の実装状況においてセーフレベルをまともに扱っていないため、セキュリティリスクがあります。</b>
 *
 * この拡張ライブラリは IO#pread / IO#pwrite を提供します。
 *
 * <tt>require "ioposrw"</tt> によって利用できるようになります。
 */

/*
 * Document-class: StringIO
 *
 * <b>現段階の実装状況においてセーフレベルをまともに扱っていないため、セキュリティリスクがあります。</b>
 *
 * StringIO#pread / StringIO#pwrite を提供します。
 *
 * StringIO#pread / StringIO#pwrite は <tt>require "ioposrw"</tt> しただけでは有効になりません。
 *
 * *IO.ioposrw_enable_stringio_extend* を呼ぶ必要があります。
 * このメソッドは、<tt>require "stringio"</tt> されていなければ勝手に読み込みます。
 *
 *  require "ioposrw" # この段階では StringIO#pread / StringIO#pwrite は利用できません。
 *  IO.ioposrw_enable_stringio_extend # これで StringIO#pread / StringIO#pwrite が利用できるようになります。
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
