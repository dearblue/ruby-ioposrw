Gem::Specification.new do |spec|
    spec.name = "ioposrw"
    spec.version = "0.1"
    spec.summary = "Append IO#pread/pwrite methods. These provide similar functionality to the POSIX pread/pwrite."
    spec.license = "2-clause BSD License"
    spec.author = "dearblue"
    spec.email = "dearblue@users.sourceforge.jp"
    spec.files = %w(
        README.txt
        ext/extconf.rb
        ext/ioposrw.c
    )

    spec.has_rdoc = false
    spec.required_ruby_version = ">= 1.9.3"
    spec.extensions << "ext/extconf.rb"
end
