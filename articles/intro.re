= はじめに
本書をお手にとっていただき、ありがとうございます。

本書では「UEFI」で「ベアメタルプログラミング」を行います。

ベアメタルプログラミングは、OS無しでハードウェアを直接制御するプログラムを書くプログラミング方法です。

「ハードウェアを直接制御」といっても、信号線の一本一本を制御するようなプログラムを書くわけではありません。たいていのハードウェアには「ファームウェア」というソフトウェアがROMに書き込まれています。そのため、ファームウェアと適切にやり取りすることでハードウェアを制御できます。

PCの代表的なファームウェアは「BIOS」です。近年は、実装が規格化された「UEFI」に置き換わっています。そのため、PCでは、電源を入れるとBIOSあるいはUEFIのファームウェアが動作し、起動ディスクからOSをブートします。

本書では近年の流れに沿って、UEFIでベアメタルプログラミングを行います。そのために、本書では主に「UEFIのファームウェアの機能をどうやって呼び出すのか」について説明していきます。

== poiOSについて
本書ではUEFIの機能を呼び出すことで、OSっぽいものを作ってみます。本書では"poiOS"と名づけており、構造は@<img>{poios_arch}の通りです。

//image[poios_arch][poiOSの構造]{
//}

== 本書に関する情報の公開場所
本書のPDFデータ版、サンプルのソースコードやコンパイル済バイナリ等、本書に関する情報は以下のページ(あるいはそこから辿れるリンク先)にまとめています。

 * @<href>{http://yuma.ohgami.jp}

また、上記のページにも書いていますが、サンプルコードは以下のGitHubのリポジトリで公開しています。サンプルコードは各節毎にディレクトリを分けており、各節の冒頭で該当するサンプルコードのディレクトリ名を説明します。文中ではコードの一部分しか引用しないこともありますので、コード全体を見たいときは下記URL先のものを参照してください。

 * @<href>{https://github.com/cupnes/c92_uefi_bare_metal_programming_samples}
