require "fileutils"

FileUtils.mkpath "lib"
system "cd ext && make && copy /y ioposrw.so ..\\lib\\" or exit $?.exitstatus

Gem::Specification.new do |spec|
    spec.name = "ioposrw"
    spec.version = "0.1"
    spec.summary = "Append IO#pread/pwrite methods. These provide similar functionality to the POSIX pread/pwrite."
    spec.license = "2-clause BSD License"
    spec.author = "dearblue"
    spec.email = "dearblue@users.sourceforge.jp"
    spec.platform = "mingw32"
    spec.files = %w(
        README.txt
        lib/ioposrw.so
    )

    spec.has_rdoc = false
    spec.required_ruby_version = ">= 1.9.3"
end
