= Hello UEFI!
この章では、UEFIでベアメタルプログラミングを行う流れとして、環境構築、"Hello UEFI!"を画面出力するプログラムの作成・実行までを説明します。

== ベアメタルプログラミングの流れ
UEFIファームウェアがロード・実行するプログラムを「UEFIアプリケーション」と呼びます。本書のベアメタルプログラミングではUEFIアプリケーションを作成し、PCへ実行させます。流れとしては以下の通りです。

 1. UEFIの仕様に沿ったプログラムを書く
 2. UEFIファームウェアが実行できる形式へクロスコンパイル
 3. UEFIファームウェアが見つけられるように起動ディスクを作成

次から、この流れに沿って、"Hello UEFI!"と画面表示するプログラムを作ってみます。

== UEFIの仕様に沿ったプログラムを書く
=== UEFI仕様書の読み方
UEFIの仕様書は@<href>{http://www.uefi.org/specifications}で公開されています("UEFI Specification"の箇所からPDFをダウンロードできます)。なお、本書ではUEFIバージョン2.3.1の仕様書を参照します@<fn>{for_lenovo}。ただし、本書で扱うような範囲は、実機のUEFIバージョン・参照する仕様書のバージョン共に、多少バージョンが前後しても問題は無いと思います。
//footnote[for_lenovo][私の動作確認している実機環境(Lenovo ThinkPad E450)の都合上です。]

UEFIの仕様書にはC言語のコード片なども含まれており、UEFIファームウェア上で動作するプログラムは基本的にC言語で作ります。例えば、プログラムの実行開始場所であるエントリポイント@<fn>{entrypoint}は、仕様書の"4.1 UEFI Image Entry Point(P.75)"に@<img>{efi_image_entry_point}の記載があります。
//footnote[entrypoint][C言語の一般的なプログラムにおけるmain関数に相当するものです。]

//image[efi_image_entry_point][仕様書のエントリポイントの説明箇所]{
//}

@<img>{efi_image_entry_point}を見ると、引数と戻り値が決められていることが分かります。関数名はコンパイル時のオプションで指定するため何でも良いです。引数や戻り値は独自の型で定義されていますが、これらの型も仕様書内で定義されています。仕様書内を"EFI_STATUS"で検索してみると、"2.3.1 Data Types(P.23)"がヒットします(@<img>{uefi_data_types_uintn_efi_status})。

//image[uefi_data_types_uintn_efi_status]["2.3.1 Data Types"の"Table 6. Common UEFI Data Types"(抜粋)]{
//}

@<img>{uefi_data_types_uintn_efi_status}には仕様書内で良く使われる型の定義が書かれています。ここを見ると、"EFI_STATUS"はステータスコードを示し、"UINTN"という型であること、また"UINTN"は32ビットCPUではunsignedの4バイト(unsigned int)、64ビットCPUではunsignedの8バイト(unsigned long long)であることが分かります。

なお、"IN"・"OUT"・"OPTIONAL"・"EFIAPI"等は引数や関数の説明の為の記載です。EDK2やgnu-efiといった既存の開発環境やツールチェイン@<fn>{about_edk2_gnuefi}では空文字列として定義されています。
//footnote[about_edk2_gnuefi][本書はベアメタルプログラミングのため、どちらも使用しませんが、これらのソースコードはUEFIの機能を呼び出す実装方法の参考になります。]

"EFI_SYSTEM_TABLE"は、"2.3.1 Data Types"の"Table 6."に説明がありません。これは構造体をtypedefしたもので、構造体については章を設けて説明されています。"EFI_SYSTEM_TABLE"で仕様書内を検索してみると、"4.3 EFI System Table(P.78)"がヒットし、こちらもC言語のコードで定義が記載されています(@<img>{efi_system_table_definition})。

//image[efi_system_table_definition][EFI_SYSTEM_TABLEの定義(一部)]{
//}

@<img>{efi_system_table_definition}には様々なメンバが並んでいます。UEFIの仕様を見ていくコツとして、「プロトコル」という概念があります。UEFIでは「プロトコル」という単位で機能を分けており、"〜_PROTOCOL"という名前の構造体がいくつもあります。"〜_PROTOCOL"という名前の構造体は関数ポインタをメンバに持っており、その関数ポインタからUEFIファームウェアの機能を呼び出せます。

@<img>{efi_system_table_definition}では、"EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL"が画面へ文字を出力するためのプロトコルです。では、"EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL"がどのようなメンバを持っているのかというと、これまで同様、仕様書を検索してみます。すると、"11.4 Simple Text Output Protocol(P.424)"に構造体の定義が書かれていることが分かります(@<img>{efi_simple_text_output_protocol_definition})。

//image[efi_simple_text_output_protocol_definition][EFI_SIMPLE_TEXT_OUTPUT_PROTOCOLの定義]{
//}

@<img>{efi_simple_text_output_protocol_definition}の"Protocol Interface Structure"をみると、"OutputString"というメンバがあり、これを使うと画面へ文字を表示できそうだと分かります。@<img>{efi_simple_text_output_protocol_definition}の"Parameters"に構造体の各メンバの説明があり、"OutputString"の説明には定義の説明を行っているページへのリンクがあります(下線で示されている"OutputString()"の箇所)。OutputStringは仕様書で@<img>{efi_text_string_definition}の様に定義されています。

//image[efi_text_string_definition][OutputStringの定義]{
//}

引数の意味は以下の通りです。

 : IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL自体のポインタ。
 : IN CHAR16 *String
    画面へ表示する文字列のポインタ。UEFIでは文字はUnicode(2バイト)。

戻り値はEFI_STATUS(unsigned long long)型で、実行結果のステータスが格納されています。成功時に0、エラー/ワーニング時に0以外の値が格納されます。本書では「0であるか否か」でしか使用しませんが、ステータス値の詳細は仕様書の"Appendix D Status Codes(P.1873)"と、各プロトコルの説明箇所の"Status Codes Returned"を参照してください。

なお、プロトコル内の関数はどれも「第1引数にプロトコル自体のポインタをとる」、「EFI_STATUS型のステータスコードを返す」であるため、以降、この2つの説明は省略します。

また、UEFIのファームウェアによっては起動時に画面にメッセージが出力されます。ここでは画面をクリアしてからOutputStringで画面へ文字列を表示することにします。

画面をクリアするにはEFI_SIMPLE_TEXT_OUTPUT_PROTOCOLのClearScreenを使用します(仕様書"11.4 Simple Text Output Protocol(P.437)")。@<img>{efi_simple_text_output_protocol_definition}では見切れていますがClearScreenも"Parameters"に説明があり、定義を説明しているページへのリンクがあります。ClearScreenの定義は@<img>{efi_text_clear_screen_definition}の通りです。引数はプロトコル自体を指す"This"のみです。

//image[efi_text_clear_screen_definition][ClearScreenの定義]{
//}

なお、以降の章ではプロトコル構造体とメンバである関数の定義は、サンプルコードをベースに説明します。

=== ソースコードを書く
エントリポイントの仕様も、画面へ文字列を出力するための関数の呼び出し方も分かったのでソースコードを書いてみます。サンプルソースコードのディレクトリは"sample1_1_hello_uefi"です。

なお、本書では開発環境はDebian GNU/Linuxを想定しています。ただし、これは筆者の作業環境がそうであるというだけで、クロスコンパイルとUSBフラッシュメモリのフォーマットができれば何でもよいです(詳細は後述します)。もちろん、エディタも何でも良いです。

"Hello UEFI!"を出力するソースコードを@<list>{sample1_1_hello_uefi_main}に示します。

#@# sample1.1: "Hello UEFI!"をコンソール出力するサンプル
#@#   => bare_metal_uefi/010_conout/
//listnum[sample1_1_hello_uefi_main][sample1_1_hello_uefi/main.c][c]{
struct EFI_SYSTEM_TABLE {
	char _buf[60];
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
		unsigned long long _buf;
		unsigned long long (*OutputString)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
			unsigned short *String);
		unsigned long long _buf2[4];
		unsigned long long (*ClearScreen)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
	} *ConOut;
};

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
					  L"Hello UEFI!\n");
	while (1);
}
//}

@<list>{sample1_1_hello_uefi_main}について、EFI_SYSTEM_TABLEの構造体定義箇所では使用するメンバのみ定義し、その他はアドレスが合うようにバッファを入れています。

また、"efi_main"という名前で定義しているエントリポイントについて、OutputString関数の前に、ClearScreen関数で画面クリアを行っています。そして、末尾には、whileの無限ループを設置しています(戻り先が無いため)。

なお、"EFI_STATUS"等のUEFI固有の型は全て"unsigned long long"等へ書き下しています。これは単純に個人の趣味です。移植性が下がるため、UEFIの仕様書通りの書き方をしたい場合は適宜typedefを追加してください。

以降は、@<list>{sample1_1_hello_uefi_main}が"main.c"というファイルで保存されているものとして説明します。

== UEFIファームウェアが実行できる形式へクロスコンパイル
ソースコードを用意できたので、コンパイルを行います。UEFIファームウェアが認識する実行ファイル形式はPE32+という形式です@<fn>{pe32plus}。Linuxの実行ファイル形式はELFなので、PE32+へクロスコンパイルする必要があります。
//footnote[pe32plus][主にWindowsで使用される実行ファイル形式です。]

クロスコンパイラを用意するために、gcc-mingw-w64-x86-64パッケージをインストールします。

//cmd{
$ @<b>{sudo apt install gcc-mingw-w64-x86-64}
//}

インストール後、以下のコマンドでクロスコンパイルできます。

//cmd{
$ @<b>{x86_64-w64-mingw32-gcc -Wall -Wextra -e efi_main -nostdinc -nostdlib \\}
	@<b>{-fno-builtin -Wl,--subsystem,10 -o main.efi main.c}
//}

"-e"オプションがエントリポイントを指定するオプションです。ここで"efi_main"を指定しているため、efi_main関数がエントリポイントとして扱われます。また、"--subsystem,10"では、作成する実行ファイルがUEFIアプリケーションであることを指定しています。生成される"main.efi"がUEFI向けPE32+実行ファイルです。

なお、PE32+でUEFIアプリケーション向けにコンパイルが行えれば、他の方法でも構いません。例えば、Windows版のx86_64-w64-mingw32-gccは@<href>{https://sourceforge.net/projects/mingw-w64/}からダウンロードできるようです@<fn>{win_mingw_gcc}。
//footnote[win_mingw_gcc][参考:"Windows(64ビット環境)でvimprocをコンパイルしてみよう":http://qiita.com/akase244/items/ce5e2e18ad5883e98a77]

===[column] Makefileで自動化
@<href>{https://github.com/cupnes/c92_uefi_bare_metal_programming_samples}で公開しているサンプルプログラムにはMakefileも入っています。

サンプルのディレクトリへ移動し、@<code>{make}を実行することでコンパイルできます。

//cmd{
$ @<b>{cd c92_uefi_bare_metal_programming_samples/<各サンプルのディレクトリ>}
$ make
//}

コンパイルが完了すると、fs/EFI/BOOT/BOOTX64.EFI にefi実行ファイルが生成されます。

== UEFIファームウェアが見つけられるように起動ディスクを作成
UEFIアプリケーションの実行ファイルを生成できたので、このファイルをUEFIファームウェアが見つけられるようにストレージへ配置し、UEFIアプリケーションの起動ディスクを作成します。ストレージとしてはUSBフラッシュメモリが簡単です。本書ではUSBフラッシュメモリを想定して説明します。

UEFIはFATファイルシステムを認識できます。そのため、まずはUSBフラッシュメモリをFAT32あたりでフォーマットします。

単にFAT32でフォーマットできれば何でもよいです。操作例としては以下の通りです(USBフラッシュメモリは/dev/sdbとして認識されているものとします)。
//cmd{
$ @<b>{sudo fdisk /dev/sdb}
Welcome to fdisk (util-linux 2.25.2).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.


Command (m for help): @<b>{d}    <= 既存のパーティションを削除
Selected partition 1
Partition 1 has been deleted.

Command (m for help): @<b>{o}

Created a new DOS disklabel with disk identifier 0xde746309.

Command (m for help): @<b>{n}
Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): @<b>{p}
Partition number (1-4, default 1): @<b>{1}
First sector (2048-15228927, default 2048):
Last sector, +sectors or +size{K,M,G,T,P} (2048-15228927, default 15228927):

Created a new partition 1 of type 'Linux' and of size 7.3 GiB.

Command (m for help): @<b>{t}
Selected partition 1
Hex code (type L to list all codes): @<b>{b}
If you have created or modified any DOS 6.x partitions, please see the fdisk \\
documentation for additional information.
Changed type of partition 'Linux' to 'W95 FAT32'.

Command (m for help): @<b>{w}
The partition table has been altered.
Calling ioctl() to re-read partition table.
Syncing disks.
$ @<b>{sudo mkfs.vfat -F 32 /dev/sdb1}
mkfs.fat 3.0.27 (2014-11-12)
$
//}

フォーマット完了後、main.efiを"BOOTX64.EFI"へリネームし、USBフラッシュメモリ内に"EFI/BOOT/BOOTX64.EFI"というパスで配置してください。

操作例は以下の通りです。

//cmd{
$ @<b>{sudo mount /dev/sdb1 /mnt}
$ @<b>{sudo mkdir -p /mnt/EFI/BOOT}
$ @<b>{sudo cp main.efi /mnt/EFI/BOOT/BOOTX64.EFI}
$ @<b>{sudo umount /mnt}
//}

作成したUSBフラッシュメモリからブートすると、"Hello UEFI!"と画面へ出力されます(@<img>{sample1_1_hello_uefi})。

//image[sample1_1_hello_uefi]["Hello UEFI!"と画面出力される様子]{
//}

シャットダウンの機能は無いので、終了させる際は電源ボタンで終了させてください。

===[column] QEMUで実行する
QEMU上で実行することも可能です。QEMUにはUEFIのファームウェアが含まれていないので、ovmf(Open Virtual Machine Firmware)パッケージもインストールしてください。

//cmd{
$ @<b>{sudo apt install qemu-system-x86}
$ @<b>{sudo apt install ovmf}
//}

QEMUには、指定したディレクトリをハードディスクと見なして実行してくれる機能があります。例えば、fsというディレクトリを作成し、fs/EFI/BOOT/BOOTX64.EFI にefiファイルを配置した上で、"-hda fat:fs"というオプションを指定してQEMUを実行すると、QEMUがfsディレクトリ以下をFATフォーマットされたハードディスクと見なして実行してくれます。

//cmd{
$ @<b>{qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -hda fat:fs}
//}

なお、@<href>{https://github.com/cupnes/c92_uefi_bare_metal_programming_samples}で公開しているサンプルのMakefileにはQEMUで実行するルールも記載済みです。@<code>{make run}で実行できます。

//cmd{
$ @<b>{cd c92_uefi_bare_metal_programming_samples/<各サンプルのディレクトリ>}
$ make run
//}

ただし、OVMFでは実装されていないのか、動作しない機能もあります。本書の内容だと、マウス入力を取得する"EFI_SIMPLE_POINTER_PROTOCOL"が動作しませんでした。

===[column] 日本語表示
Unicodeとしては日本語も扱えます。ただし、UEFIファームウェアによってサポートされている文字はまちまちな様です。表紙の写真の通り、筆者のLenovo製ThinkPad E450のUEFIファームウェア(バージョン2.3.1)では、ひらがなや漢字等は一部の文字が表示できませんでした。

なお、QEMUのOVMFで試してみると、日本語は一切表示できないようです(@<img>{echo_title_qemu})。

//image[echo_title_qemu][QEMU(OVMF)での日本語表示]{
//}
