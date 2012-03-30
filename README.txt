# encoding:UTF-8

= ioposrw

- Author : dearblue <dearblue@users.sourceforge.jp>
- Project Page : http://sourceforge.jp/projects/rutsubo/

位置指定IO読み書きメソッド追加拡張ライブラリ


== これは何?

ioposrwは、IOインスタンスに位置指定読み書き機能を追加する拡張ライブラリで、それらはIO#pread、IO#pwriteによって提供される。

これらのメソッドは、元々あるIO#read、IO#writeによって更新されるファイル位置ポインタに影響されることがない。

マルチスレッド動作中に同じファイルインスタンスの別の領域を読み書きしても、想定通りの動作が期待できる。

POSIX環境であればシステムコールのpread/pwriteが、Windows環境であればOVERRAPPED構造体を与えたReadFile/WriteFileをそれぞれ用いて実装してある。


== どう使うのか?

ライブラリの読み込み:

	require "ioposrw"

ファイルの読み込み:

	File.open("sample.txt") do |file|
	  buf = ""
	  file.pread(100, 200, buf) # sample.txt の先頭 200 bytes 位置から 100 bytes を buf に読み込む
	end

ファイルの書き込み:

	File.open("sample.txt", "w") do |file|
	  buf = "ごにょごにょ"
	  file.pwrite(buf, 200) # sample.txt の先頭 200 bytes 位置に buf を書き込む
	end


== ライセンスについて

二条項BSDライセンスの下で取り扱うことができるものとする。


== やっつけリファレンス

=== IO#pread(offset [, size [, buffer] ] ) # => buffer or a String

概要::
	指定位置から読み込む。IO#readとは異なり、読み込み量の指定を必要とする。
戻り値::
	読み込まれたデータが格納されたStringインスタンスが返る。
	引数bufferを与えた場合は、bufferが返る。
引数 offset::
	読み込み位置を bytes で指定する。これはストリーム先端からの絶対位置となる。
	0以下の値を与えた場合、例外『ほにゃらら』が発生する。
引数 size (省略可能)::
	読み込み量を bytes で指定する。
	省略した場合、offset位置からファイルの最後までを読み込むことを意味する。
引数 buffer (省略可能)::
	任意で読み込み先となるStringインスタンスを指定する。
	省略した場合、IO#pread内部でStringインスタンスが生成される。
	bufferは変更可能(frozenではない)なインスタンスである必要がある。
例外::
	Errno::EXXXが発生するかもしれない。
	bufferの再確保時にout of memoryが発生するかもしれない。
	SEGVが発生するかもしれない。ここ、笑えないところ。

=== IO#pwrite(offset, buffer) # => a Integer

概要::
	指定位置にデータを書き込む。
戻り値::
	書き込んだデータ量がbyte単位で返る。
引数 offset::
	書き込み位置を byte 単位で指定する。これはストリーム先端からの絶対位置となる。
	0以下の値を与えた場合、例外『ほにゃらら』が発生する。
引数 buffer::
	書き込みたいデータが格納されたStringインスタンスを指定する。
例外::
	Errno::EXXX: IO障害発生や、システムコールエラーなどの場合に発生する。
	ArgumentError: 引数がおかしい場合に発生する。

