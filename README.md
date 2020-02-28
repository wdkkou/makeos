# 概要

os 自作入門を Ubuntu の環境で出来る限り作る

# 開発環境

- os Ubuntu16.04 (18.04 も一応試した)
- エミュレータ　 qemu-system-i386

# 実行方法

git clone https://github.com/wdkkou/makeos.git  
cd makeos  
make

# 作成した os の使い方

- window のバツボタンをクリックするとその window を閉じる
- capslock で大文字にできる
- shft+f2 でコンソールを表示する (複数起動可能)
- コンソールに実行可能なコマンドを打つことでアプリなどが起動

# 実行可能なコマンド

- mem
  - os のメモリ情報を表示
- clear
  - コンソールに表示されている文字を消して最初の状態にする
- ls
  - os 内のアプリやファイルを表示する
- exit
  - console を閉じるコマンド
- start
  - 新しいコンソールを開いてそのコンソールでアプリを実行する
  - アプリを起動したまま別のアプリを実行可能(一部出来ない...?)  
    例　`start コマンド`
- ncst
  - 新しいコンソールを開かずにアプリを実行する
  - アプリを起動したまま別のアプリを実行可能(一部出来ない...?)  
    例　`ncst コマンド`
- star
  - 黒い画面に黄色い点が表示
- stars
  - 黒い画面に複数の黄色い点が表示
- line
  - 色んな色の線が表示
- walk
  - 黄色い＊を j(下), k(上), l(右), h(左) で動かすことが可能
- noodle
  - 時間を図ることが可能
- color
  - 複数の色が表示された window の表示
- color2
  - color より細かい複数の色が表示された window の表示
- sosu
  - 1000 以下の素数を表示
- sosu2
  - 10000 以下の素数を表示
- win
  - window の表示
- win2
  - hello,world の文字と黄色い下線がある window の表示
- win3
  - hello,world の文字と水色の下線がある window の表示
- cat
  - この os 内に含まれるテキストファイルを現在の文字コードで表示  
    例　`cat ファイルの名前`
- iroha
  - 文字コードが sjis ならば, イロハニホヘトと表示
  - それ以外の文字コードならば, 文字化け
- langmode
  - 文字コードの切り替えが可能
  - 0 English ASCII
  - 1 sjis
  - 2 euc  
    例　`langmode 0`
- chklang
  - 現在の文字コードが分かる
- notrec
- 矩形の形で穴が開いている window が表示
- bball
  - 線で書かれたボールの絵が表示
- invader
  - インベーダーゲームが起動
  - j(下), k(上), l(右), h(左) で機体が移動
  - space で発射
