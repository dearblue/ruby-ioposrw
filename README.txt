#encoding:utf-8

= ioposrw

- Author : dearblue <dearblue@users.sourceforge.jp>
- Project Page : http://sourceforge.jp/projects/rutsubo/
- License : 2-clause BSD License (二条項 BSD ライセンス)

位置指定IO読み書きメソッド追加拡張ライブラリ


== これは何?

ioposrwはIOインスタンスに位置指定読み書き機能を追加する拡張ライブラリで、それらは IO#pread / IO#pwrite によって提供される。

これらのメソッドは、元々ある IO#read / IO#write によって更新されるファイル位置ポインタに影響されることがない。

マルチスレッド動作中に同じファイルインスタンスの別の領域を読み書きしても、想定通りの動作が期待できる。

追加の機能として、StringIO#pread / StringIO#pwrite も実装してある。


== どう使うのか?

ライブラリの読み込み:

	require "ioposrw"

ファイルの読み込み:

	File.open("sample.txt") do |file|
	  buf = ""
	  file.pread(100, 200, buf) # sample.txt の先頭 200 バイト位置から
	                            # 100 バイトを buf に読み込む
	end

ファイルの書き込み:

	File.open("sample.txt", "w") do |file|
	  buf = "ごにょごにょ"
	  file.pwrite(buf, 200) # sample.txt の先頭 200 バイト位置に buf を書き込む
	end


== 内部の実装方法

POSIX システムコールの pread / pwrite を中心に実装した。

Windows 環境では OVERRAPPED 構造体を与えた ReadFile / WriteFile をそれぞれ用いて pread / pwrite 関数を構築し、あとは同じである。


== ライセンスについて

二条項BSDライセンスの下で取り扱うことができるものとする。


\[以上\]
