Gem::Specification.new do |spec|
    spec.author = "dearblue"
    spec.email = "dearblue@users.sourceforge.jp"
    spec.homepage = "http://sourceforge.jp/projects/rutsubo/"

    spec.name = "ioposrw"
    spec.version = "0.3"
    spec.summary = "Append IO#pread/pwrite methods. These provide similar functionality to the POSIX pread/pwrite."
    spec.license = "2-clause BSD License"
    spec.description = <<-EOS
Append IO#pread/pwrite methods. These provide similar functionality to the POSIX pread/pwrite.
This library works on windows and unix. But tested on windows and FreeBSD only.
    EOS

    spec.files = %w(
        README.txt
        ext/extconf.rb
        ext/ioposrw.c
        testset/test1.rb
    )

    spec.rdoc_options = %w(-e UTF-8 -m README.txt)
    spec.extra_rdoc_files = %w(README.txt ext/ioposrw.c)
    spec.has_rdoc = false
    spec.required_ruby_version = ">= 1.9.3"
    spec.extensions << "ext/extconf.rb"
end
