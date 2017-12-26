# ioposrw 変更履歴

## 0.4 (平成29年12月26日 火曜日)

  * ``IO#pread``/``IO#pwrite`` を ``IO#readat``/``IO#writeat`` に変更

    Ruby-2.5 で ``IO#pread``/``IO#pwrite`` が組み込まれましたが引数のとり方が異なるため、 ``IO#readat``/``IO#writeat`` として利用できるように変更しました。

  * ファイル終端位置の指定を行えるようになった

    ``readat``/``writeat`` の ``offset`` 引数に nil を与えることで、ファイルの終端位置を示すことが出来るように機能を変更しました。

  * ``IOPositioningReadWrite`` モジュールを追加し、``IO`` クラスへ include するように変更

    実装の実体を ``IOPositioningReadWrite`` モジュールに行い、IO クラスへ include するようにしました。

  * "ioposrw/stringio" ライブラリの追加

    ``IO.ioposrw_enable_stringio_extend`` メソッドは廃止する予定です。

    代わりに ``require "ioposrw/stringio"`` を使って下さい。
