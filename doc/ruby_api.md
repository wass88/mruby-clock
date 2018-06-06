# mRuby
- `a / b`は小数を返すので要注意 `a.div b`を使いましょう

以下の拡張が有効化されています
- mruby-numeric-ext - Numeric class extension
- mruby-string-ext - String class extension
- mruby-array-ext - Array class extension
- mruby-math - standard Math module
- mruby-random - Random class

# Led
## Led.color r, g, b
- 色は0 ~ 7
## Led.font f
- フォントの数字 //TODO
## Led.flash
- (flushじゃね?) 画面に反映する
## Led.clear c
- メインバッファを色(c, c, c)で塗りつぶす
## Led.clear_buf c, b
- バッファbを色(c, c, c)で塗りつぶす
- バッファは0, 1, 2の3つ。0はメインバッファ
- バッファiはy座標に32*iの場所に存在する。
    + Ex. バッファ1の(10, 10)は(10, 42)
## Led.copy_buf t, f
- バッファfをバッファtにコピーする
## Led.copy tx, ty, fx, fy, w, h
- (fx, fy)から幅w高さhを(tx,ty)にコピーする
## Led.set x, y
- (x, y)をLed.colorで設定した色にする
## Led.line x0, y0, x1, y1
- (x0, y0)から(x1, y1)へ直線を引く
## Led.rect x0, y0, x1, y1
- (x0, y0)から(x1, y1)を対角線とする矩形を書く
## Led.fill_rect x0, y0, x1, y1
- (x0, y0)から(x1, y1)を対角線とする矩形を塗りつぶす
## Led.circle x, y, r
- (x, y)を中心として半径rの円を書く
## Led.fill_circle x, y, r
- (x, y)を中心として半径rの円を塗りつぶす
## Led.char x, y, c
- 一文字cを書く
## Led.text x, y, text
- textを(x,y)に描画する
## Led.scroll t, y, text
- y座標yにtextを流す。tに時間により増える変数を設定する。
## Led.show t, y, text
- y座標yにtextを流す。textが短い時は流れない。

# Time
## Time.update
- 現在時間を更新する。
## Time.now
- 現在時間をミリ秒で得る。
## Time.str format
- strftimeのフォーマットを指定して文字列で時間を得る。
## Time.num id
- [年, 月, 週, 時, 分, 秒][id]を得る。

# Task
## Task.loop do ループの本体 end
- ループの本体を繰り返す
## Task.cmd
- cmdによって書き込まれた文字列を得る。
- http://時計/cmd?q=クエリ で書き込める。
## Task.updated?
- 最後にTask.updated?を実行してからcmdが更新されたかを返す。
## Task.sleep
- 0.1秒スリープする