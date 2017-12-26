ver = RUBY_VERSION.slice(/\d+\.\d+/)
soname = File.basename(__FILE__, ".rb") << ".so"
lib = File.join(File.dirname(__FILE__), ver, soname)
if File.file?(lib)
  require_relative File.join(ver, soname)
else
  require_relative soname
end

require_relative "ioposrw/version"

module IOPositioningReadWrite
  module IOClass
    #
    # call-seq:
    #  IO.ioposrw_enable_stringio_extend -> nil
    #
    # This is a feature that is FUTURE OBSOLUTE.
    #
    # Please use <tt>require "ioposrw/stringio"</tt> insted.
    #
    def ioposrw_enable_stringio_extend
      warn <<-EOM
(warning) #{caller(1, 1)[0]}: IO.ioposrw_enable_stringio_extend is FUTURE OBSOLUTE. please use ``require \"ioposrw/stringio\"'' insted.
      EOM
      require_relative "ioposrw/stringio"
      nil
    end
  end
end

class IO
  extend IOPositioningReadWrite::IOClass
  include IOPositioningReadWrite::IO
end
