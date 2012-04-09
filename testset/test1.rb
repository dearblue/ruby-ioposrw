#!/usr/bin/env ruby
#vim: set fileencoding:utf-8

require "test/unit"
require "ioposrw"
require "tempfile"

class TC_ioposrw < Test::Unit::TestCase
    attr_reader :file

    SOURCE = ((0...256).to_a * 257).shuffle!.pack("C*")

    def setup
        @file = Tempfile.open("")
        file.binmode
        file << SOURCE
        file.flush
    end

    def try_pread(file)
        [
            [0, 0, false, "", "空文字列になるべき"],
            [SOURCE.length, 0, false, "", "空文字列になるべき"],
            [SOURCE.length, nil, false, nil, "不適切な EOF"],
            [file.size, nil, false, nil, "不適切な EOF"],
            [file.size, 99999999, false, nil, "不適切な EOF"],
            [0, nil, false, SOURCE, "先頭からの全読み込みが不一致"],
            [0, SOURCE.length, false, SOURCE, "先頭からの全読み込みが不一致"],
            [SOURCE.length / 2, SOURCE.length / 2, false,
             SOURCE.slice(SOURCE.length / 2, SOURCE.length / 2),
             "途中位置からの固定長分の読み込みが不一致"],

            [0, 0, true, "", "空文字列になるべき"],
            [SOURCE.length, 0, true, "", "空文字列になるべき"],
            [SOURCE.length, nil, true, nil, "不適切な EOF"],
            [file.size, nil, true, nil, "不適切な EOF"],
            [file.size, 99999999, true, nil, "不適切な EOF"],
            [0, nil, true, SOURCE, "先頭からの全読み込みが不一致"],
            [0, SOURCE.length, true, SOURCE, "先頭からの全読み込みが不一致"],
            [SOURCE.length / 2, SOURCE.length / 2, true,
             SOURCE.slice(SOURCE.length / 2, SOURCE.length / 2),
             "途中位置からの固定長分の読み込みが不一致"],
        ].each do |pos, size, buf, ref, mesg|
            if buf
                buf = 200.times.reduce("") { |a, *| a << rand(256).chr(Encoding::BINARY) }
            else
                buf = nil
            end
            assert(file.pread(pos, size, buf) == ref,
                   "%s (%s)" % [mesg, pos: pos, size: size, buf_len: (buf ? buf.length : nil)])
        end

        256.times do
            size = rand(file.size * 2)
            pos = file.pos = rand(file.size)
            assert(file.read(size) == (buf = file.pread(pos, size)),
                   "IO#pos + IO#read との不一致 (pos=#{pos}, size=#{size})")
            buf1 = "".force_encoding(Encoding::BINARY)
            file.pos = pos
            assert(file.read(size, buf1) == (buf = file.pread(pos, size, buf1)),
                   "IO#pos + IO#read with buffer との不一致 (pos=#{pos}, size=#{size})")
            assert(buf == SOURCE.byteslice(pos, size),
                   "SOURCE.byteslice との不一致 (pos=#{pos}, size=#{size})")
        end
    end

    def try_pwrite(file)
        test = proc do |pos, buf|
            assert(file.pwrite(pos, buf) == buf.bytesize, "書き込み量の不一致")
            if SOURCE.length < pos
                SOURCE << "\0" * (pos - SOURCE.length)
            end
            SOURCE[pos, buf.bytesize] = buf
            assert(file.size == SOURCE.length, "ファイルサイズが不正 (file.size=#{file.size})")
            buf = file.pread(0)
            assert(buf.length == SOURCE.length, "ファイルデータ量の不一致")
            assert(buf == SOURCE, "書き込みデータの不一致")
        end
        [
            [0, "HERGFDGFHMFDNGF"],
            [10, "fsdghjljlkjhtgrfegvhjmg"],
            [SOURCE.length + 100, "guiuydrtfv nm,j\;lkjchxdgxehtrjyukli,mnbvgdshbjt\n\r\r\n\0liu;79pfy7kt7lofvol"],
        ].each(&test)

        256.times do
            pos = rand(SOURCE.length + 16384)
            buf = (0...256).to_a.shuffle!.pack("C*")
            test.(pos, buf)
        end
    end

    def test_io_pread
        try_pread(file)
    end

    def test_io_pwrite
        try_pwrite(file)
    end

    def test_stringio_enable
        th = Thread.fork do
            $SAFE = 2
            IO.ioposrw_enable_stringio_extend
        end
        assert_raise(SecurityError) { th.join }
        assert_nil(IO.ioposrw_enable_stringio_extend)
        assert($".select{|x| x=~/stringio\.so$/}.length == 1, 'require "stringio" されない')
        assert(StringIO.instance_methods.select{|x| x==:pread}.length == 1, "StringIO#pread が未定義")
        assert(StringIO.instance_methods.select{|x| x==:pwrite}.length == 1, "StringIO#pwrite が未定義")
        @@str = StringIO.new(SOURCE.dup)
        assert_not_nil(@@str)
    end

    def test_stringio_pread
        assert(@@str)
        try_pread(@@str)
    end

    def test_stringio_pwrite
        assert(@@str)
        try_pwrite(@@str)
    end
end
