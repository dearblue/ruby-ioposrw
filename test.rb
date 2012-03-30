#!/usr/bin/env ruby
#vim: set fileencoding:utf-8

system("cd ext && make") or exit($?)

require_relative "ext/ioposrw.so"

File.open("ext/ioposrw.c", "rb") do |file|
    file.pread(0, 1000)
    p file.pos
    file.pos = 0
    p file.read == file.pread(0)
    file.pos = 5600
    p file.read == file.pread(5600)
    file.pos = file.size - 9
    p file.read(9) == file.pread(file.size - 9, 9)
    file.pos = file.size - 9
    p file.read(16) == file.pread(file.size - 9, 16)
    file.pos = 1
    p file.read(9) == file.pread(1, 9)


    file.pos = 100
    src = file.read(100)
    th = []
    100.times do
        th << Thread.new do
            begin
                while true
                    file.pread(100, 100) == src or raise("!")
                end
            rescue Object
                Thread.main.raise $!
            end
        end
    end

    sleep 30
end
