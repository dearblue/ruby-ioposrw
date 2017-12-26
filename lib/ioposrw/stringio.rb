require_relative "../ioposrw"
require "stringio"

class StringIO
  include IOPositioningReadWrite::StringIO
end
