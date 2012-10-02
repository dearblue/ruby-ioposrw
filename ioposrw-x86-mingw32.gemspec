require "fileutils"

FileUtils.mkpath "lib"
system "cd ext && make && copy /y ioposrw.so ..\\lib\\" or exit $?.exitstatus

Gem::Specification.new do |spec|
    spec.author = "dearblue"
    spec.email = "dearblue@users.sourceforge.jp"
    spec.homepage = "http://sourceforge.jp/projects/rutsubo/"

    spec.name = "ioposrw"
    spec.version = "0.3.2"
    spec.summary = "Append IO#pread/pwrite methods. These provide similar functionality to the POSIX pread/pwrite."
    spec.license = "2-clause BSD License"
    spec.platform = "x86-mingw32"
    spec.description = <<-EOS
Append IO#pread/pwrite methods. These provide similar functionality to the POSIX pread/pwrite.
This library works on windows and unix. But tested on windows and FreeBSD only.

This binary package is for windows only.
    EOS

    spec.files = %w(
        README.txt
        ext/extconf.rb
        ext/ioposrw.c
        lib/ioposrw.so
        testset/test1.rb
    )

    spec.rdoc_options = "-e UTF-8 -m README.txt"
    spec.extra_rdoc_files = %w(README.txt ext/ioposrw.c)
    spec.has_rdoc = false
    spec.required_ruby_version = ">= 1.9.3"
end
