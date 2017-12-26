unless File.read("README.md", 4096) =~ /^\s*\*\s*version:{1,2}\s*(.+)/i
  raise "バージョン情報が README.md に見つかりません"
end

ver = $1
verfile = "lib/ioposrw/version.rb"
LIB << verfile

file verfile => "README.md" do |*args|
  File.binwrite args[0].name, <<-VERSION_FILE
module IOPositioningReadWrite
  VERSION = "#{ver}"
end
  VERSION_FILE
end


GEMSTUB = Gem::Specification.new do |s|
  s.name = "ioposrw"
  s.version = ver
  s.summary = "Append IO#readat/writeat methods. These provide similar functionality to the POSIX pread/pwrite."
  s.description = <<EOS
Append IO#readat/writeat methods. These provide similar functionality to the POSIX pread/pwrite.
This library works on windows and unix. But tested on windows and FreeBSD only.
EOS
  s.license = "BSD-2-Clause License"
  s.author = "dearblue"
  s.email = "dearblue@users.noreply.github.com"
  s.homepage = "https://github.com/dearblue/ruby-ioposrw"

  s.add_development_dependency "rake"
  s.add_development_dependency "test-unit"
end

DOC << "QUICKREF.md"
