#encoding:utf-8

= ioposrw

- Author : dearblue <dearblue@users.sourceforge.jp>
- Project Page : http://sourceforge.jp/projects/rutsubo/
- License : 2-clause BSD License (二条項 BSD ライセンス)

位置指定IO読み書きメソッド追加拡張ライブラリ


== これは何?

ioposrw は IO インスタンスに位置指定読み書き機能を追加する拡張ライブラリで、それらは IO#pread / IO#pwrite によって提供されます。

これらのメソッドは、元々ある IO#read / IO#write によって更新されるファイル位置ポインタに影響されません。

マルチスレッド動作中に同じファイルインスタンスの別の領域を読み書きしても、想定通りの動作が期待できます。

追加の機能として、StringIO#pread / StringIO#pwrite も実装してあります (実際に利用する場合は、<t>require "ioposrw"</t> した後に <t>IO.ioposrw_enable_stringio_extend</t> を呼び出す必要があります)。


== どう使うのか?

ライブラリの読み込み:

	require "ioposrw"

ファイルの読み込み:

	File.open("sample.txt") do |file|
	  buf = ""
	  file.pread(200, 100, buf) # sample.txt の先頭 200 バイト位置から
	                            # 100 バイトを buf に読み込む
	end

ファイルの書き込み:

	File.open("sample.txt", "w") do |file|
	  buf = "ごにょごにょ"
	  file.pwrite(200, buf) # sample.txt の先頭 200 バイト位置に buf を書き込む
	end


== 注意事項について

- <b>現段階の実装状況においてセーフレベルをまともに扱っていないことによる、セキュリティリスクが考えられます。</b>

- Windows 上では、IO#pread / IO#pwrite を呼び出すとファイルポインタ (IO#pos) が指定位置の次に変更されます (このことは丁度 IO#pos= と IO#read を呼び出した後の状態と考えてください)。

  これは Windows 自身に伴う仕様となります。


== 内部の実装方法

POSIX システムコールの pread / pwrite を中心に実装しました。

Windows 環境では OVERRAPPED 構造体を与えた ReadFile / WriteFile をそれぞれ用いて pread / pwrite 関数を構築し、あとは同じです。


== ライセンスについて

二条項BSDライセンスの下で取り扱うことができます。


\[以上\]
