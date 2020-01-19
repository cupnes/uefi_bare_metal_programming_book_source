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

= キー入力を取得する
第1章では、UEFI仕様書から機能を調べ、UEFIアプリケーションの作成・実行までの流れを説明しました。この章では、キー入力の取得方法を紹介し、簡単なシェルもどきを作ってみます。

== EFI_SIMPLE_TEXT_INPUT_PROTOCOL
キー入力を取得する関数は"EFI_SIMPLE_TEXT_INPUT_PROTOCOL"の中にあります。EFI_SIMPLE_TEXT_INPUT_PROTOCOLも、前の章でテキスト出力のために使用したEFI_SIMPLE_TEXT_OUTPUT_PROTOCOLと同じく、SystemTableのメンバです(@<img>{efi_system_table_definition_2})。

//image[efi_system_table_definition_2][EFI_SYSTEM_TABLEの定義(一部)(再掲)]{
//}

EFI_SIMPLE_TEXT_INPUT_PROTOCOLの定義は@<list>{efi_simple_text_input_protocol_definition}の通りです。@<list>{efi_simple_text_input_protocol_definition}では使用する関数のみ定義しています。EFI_SIMPLE_TEXT_INPUT_PROTOCOLの全体は、仕様書の"11.3 Simple Text Input Protocol(P.420)"を参照してください。

//list[efi_simple_text_input_protocol_definition][EFI_SIMPLE_TEXT_INPUT_PROTOCOLの定義][c]{
struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
	unsigned long long _buf;
	unsigned long long (*ReadKeyStroke)(
		struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
		struct EFI_INPUT_KEY *Key);
};
//}

キー入力は、EFI_SIMPLE_TEXT_INPUT_PROTOCOLの"ReadKeyStroke"関数で取得できます(仕様書"11.3 Simple Text Input Protocol(P.423)"参照)。ReadKeyStroke関数はノンブロッキングの関数で、関数実行時にキー入力が無ければエラーのステータスを返します。

引数の意味は以下の通りです(第1引数は省略)。

 : struct EFI_INPUT_KEY *Key
    取得した文字を格納するポインタ。

なお、EFI_INPUT_KEY構造体の定義は@<list>{efi_input_key_definition}の通りです。

//list[efi_input_key_definition][EFI_INPUT_KEYの定義][c]{
struct EFI_INPUT_KEY {
	unsigned short ScanCode;
	unsigned short UnicodeChar;
};
//}

また、@<list>{efi_input_key_definition}のメンバの意味は以下の通りです。

 : unsigned short ScanCode
    Unicode範囲外のキー(ESC、上下左右、Fn等)の値を表すスキャンコード。スキャンコードの一覧は仕様書P.410の"Table 88"を参照。本書ではESCキー(ScanCode=0x17)のみ使用する。Unicode範囲内のキー(英数字、Enter)が入力された時、ScanCodeへは0が格納される。
 : unsigned short UnicodeChar
    Unicode範囲内のキー入力時、入力文字に対応するUnicode値が格納される。Unicode範囲外のキー入力時、0が格納される。

== エコーバックプログラムを作ってみる
ReadKeyStroke関数を使用して、取得した文字を画面へ出力する「エコーバック」のサンプルを@<list>{sample2_1_echoback_main}に示します。サンプルのディレクトリは"sample2_1_echoback"です。

#@# sample2.1: キー入力を取得するサンプル
#@#   => ~/bare_metal_uefi/020_echoback/
//listnum[sample2_1_echoback_main][sample2_1_echoback/main.c][c]{
struct EFI_INPUT_KEY {
	unsigned short ScanCode;
	unsigned short UnicodeChar;
};

struct EFI_SYSTEM_TABLE {
	char _buf1[44];
	struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
		unsigned long long _buf;
		unsigned long long (*ReadKeyStroke)(
			struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
			struct EFI_INPUT_KEY *Key);
	} *ConIn;
	unsigned long long _buf2;
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
	struct EFI_INPUT_KEY key;
	unsigned short str[3];
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
	while (1) {
		if (!SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn,
						       &key)) {
			if (key.UnicodeChar != L'\r') {
				str[0] = key.UnicodeChar;
				str[1] = L'\0';
			} else {
				str[0] = L'\r';
				str[1] = L'\n';
				str[2] = L'\0';
			}
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
							  str);
		}
	}
}
//}

@<list>{sample2_1_echoback_main}では、EFI_SIMPLE_TEXT_INPUT_PROTOCOLの定義をEFI_SYSTEM_TABLEに追加しています。

efi_main関数内について、ClearScreen後のwhileの無限ループがエコーバックの処理です。ReadKeyStrokeでキー入力を取得できたら、str配列へヌル文字(L'\0')を付加した文字列として格納し、OutputStringで画面へ表示します。なお、Enterキーの入力時はCR('\r')を取得するため、取得した文字がCRのときはLF('\n')も出力するようにしています。

サンプルを実行すると、入力した文字がそのまま表示されるエコーバック動作を確認できます(@<img>{sample2_1_echoback})。

//image[sample2_1_echoback][エコーバックサンプル実行の様子]{
//}

=== 補足: キー入力を待つ(WaitForKey)
@<list>{sample2_1_echoback_main}では、while()内でReadKeyStroke関数が成功するまで、ReadKeyStroke関数を何度も呼び出しています。しかし、キー入力が得られるまでCPUを休ませてあげた方がCPUに優しいです。

そのためにEFI_SIMPLE_TEXT_INPUT_PROTOCOLには"WaitForKey"というメンバ変数があります(@<list>{wait_for_key_definition}、仕様書"11.3 Simple Text Input Protocol(P.421)")。

//list[wait_for_key_definition][WaitForKeyの定義][c]{
struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
	unsigned long long _buf;
	unsigned long long (*ReadKeyStroke)(
		struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
		struct EFI_INPUT_KEY *Key);
	void *WaitForKey;
};
//}

"void *"は、UEFIでイベントを指す"EFI_EVENT"型の実体で、仕様書上は@<code>{EFI_EVENT WaitForKey}と定義されています。仕様書P.421のWaitForKeyの説明にも記載の通り、WaitForKeyはWaitForEvent関数で使用できます。

WaitForEventは指定したイベントの発生を待つ関数です。SystemTable->BootServices内で定義されています。BootServicesはEFI_BOOT_SERVICESという構造体で、主にブートローダー向けにUEFIが提供している関数(サービス)を持ちます(詳細は次の章で説明します)。WaitForEventの定義は@<list>{wait_for_event_definition}の通りです。

//list[wait_for_event_definition][WaitForEventの定義][c]{
unsigned long long (*WaitForEvent)(
	unsigned long long NumberOfEvents,
	void **Event,
	unsigned long long *Index);
//}

引数の意味は以下の通りです。

 : unsigned long long NumberOfEvents
    第2引数Eventに指定するイベントの数。
 : void **Event
    イベントリスト。
 : unsigned long long *Index
    発生したイベントのイベントリスト内のインデックスを設定する変数のポインタ。

WaitForKeyとWaitForEventを使用して、@<list>{sample_wait_for_key}の様にキー入力を待つことができます。

//listnum[sample_wait_for_key][WaitForKeyとWaitForEventを使用する例][c]{
struct EFI_INPUT_KEY key;
unsigned long long waitidx;

/* キー入力取得まで待機 */
SystemTable->BootServices->WaitForEvent(1,
	&(SystemTable->ConIn->WaitForKey), &waitidx);

/* キー入力取得 */
SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key);
//}

== シェルっぽいものを作ってみる
ここまでで、コンソール画面上での文字の入出力ができるようになりました。OSっぽいものを作る上で、まずはシェルっぽいものを作ってみます。サンプルのディレクトリは"sample2_2_shell"です。

このサンプルでは、これからOSっぽいものを作っていく上で土台となるソースコード構成を用意します。ここでは、関数化を適宜行い、ソースコードを以下の様に分けます。

 : main.c
    エントリポイント(efi_main)を配置。
 : efi.h,efi.c
    UEFI仕様上の定義や、初期化処理を配置。
 : common.h,common.c
    汎用的に使用される定義や関数を配置。
 : shell.h,shell.c
    シェルの処理を配置。

エントリポイントの引数であるSystemTableは、何をするにしても必要になるため、グローバル変数へ格納しておくことにします。その処理を行うのがefi.cの初期化処理(efi_init関数)です(@<list>{sample2_2_shell_efi})@<fn>{efi_init}。efi_initではSetWatchdogTimerという関数を呼び出していますが、これについては後述のコラムで説明します。
//footnote[efi_init][EDK2やgnu-efiといった開発環境やツールチェインでも同様に、SystemTable等をグローバル変数へ格納する枠組みになっています。]

//listnum[sample2_2_shell_efi][sample2_2_shell/efi.c][c]{
#include "efi.h"
#include "common.h"

struct EFI_SYSTEM_TABLE *ST;

void efi_init(struct EFI_SYSTEM_TABLE *SystemTable)
{
	ST = SystemTable;
	ST->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
}
//}

そして、シェルのソースコードは@<list>{sample2_2_shell_shell_c}、エントリポイントのソースコードは@<list>{sample2_2_shell_main_c}の通りです。

#@# sample2.2: シェルもどき
#@#   => ~/bare_metal_uefi/030_shell/ をもっとシンプルに(コマンドライン引数とか不要)
#@#   => puts()追加
#@#   => integerの出力はputi()とか、シンプルに
//listnum[sample2_2_shell_shell_c][sample2_2_shell/shell.c][c]{
#include "common.h"
#include "shell.h"

#define MAX_COMMAND_LEN	100

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];

	while (TRUE) {
		puts(L"poiOS> ");
		if (gets(com, MAX_COMMAND_LEN) <= 0)
			continue;

		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		else
			puts(L"Command not found.\r\n");
	}
}
//}

//listnum[sample2_2_shell_main_c][sample2_2_shell/main.c][c]{
#include "efi.h"
#include "shell.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
	efi_init(SystemTable);

	shell();
}
//}

@<list>{sample2_2_shell_shell_c}では、"OSっぽいもの"ということで、プロンプトに"poiOS"と付けてみました@<fn>{poios}。
//footnote[poios]["mockOS"とか"OSmodoki"とかも考えていたのですが、Google検索してみると、どちらも既に世の中に存在するようです。]

@<list>{sample2_2_shell_shell_c}で登場した各種の定数や、関数puts・gets・strcmpは、common.hとcommon.cで定義しています。これまで説明したUEFIの機能の呼び出し方を関数化しただけなので、紙面上では特に紹介しません(独特な実装方法をしているわけでもないので)。気になる方は、GitHubのサンプルコードをダウンロードして見てみてください。

また、@<list>{sample2_2_shell_main_c}では、efi_init関数でUEFIの初期化処理を行い、shell関数を実行することでシェルを起動しています。以降、main.cは書き換えません。

そして、サンプル実行の様子は@<img>{sample2_2_shell}の通りです。

//image[sample2_2_shell][シェルもどき実行の様子]{
//}

===[column] 5分のウォッチドッグタイマを解除する
実はUEFIアプリケーション起動時、ウォッチドッグタイマがセットされています。その時間は5分ですので、何もしないでいると、UEFIアプリケーションが起動してから5分後に再起動することになります。ウォッチドッグタイマはSystemTable->BootServices->SetWatchdogTimer関数で解除できます。

SetWatchdogTimer関数の定義は@<list>{set_watchdog_timer_definition}の通りです(仕様書"6.5 Miscellaneous Boot Services(P.201)")。

//list[set_watchdog_timer_definition][SetWatchdogTimerの定義][c]{
unsigned long long (*SetWatchdogTimer)(
	unsigned long long Timeout,
	unsigned long long WatchdogCode,
	unsigned long long DataSize,
	unsigned short *WatchdogData);
//}

また、引数の意味は以下の通りです。

 : unsigned long long Timeout
    ウォッチドッグのタイムアウト時間。0を指定するとウォッチドッグタイマー無効化。無効化の際、その他の引数は0あるいはNULLで良い。
 : unsigned long long WatchdogCode
    タイムアウト時のウォッチドッグコード(イベント番号)を指定。本書では使用しない。
 : unsigned long long DataSize
    WatchdogDataのデータサイズ(バイト指定)。
 : unsigned short *WatchdogData
    オプショナルであり、本書では使用しない。加えて、何なのか良く分かっていないです(ウォッチドッグイベント発生時にログに記録される追加の説明?)。

ウォッチドッグタイマー無効化のコード例は@<list>{disable_wdt}の通りです。

//list[disable_wdt][ウォッチドッグタイマ無効化][c]{
ST->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
//}

= 画面に絵を描く
シェルっぽいものが一応できたので、次はGUIっぽいものを作ってみます。まずは画面へ絵を描く方法を紹介し、アイコン代わりに矩形を描いてみます。サンプルプログラムは"sample3_1_draw_rect"ディレクトリです。

== EFI_GRAPHICS_OUTPUT_PROTOCOL
グラフィック描画には"EFI_GRAPHICS_OUTPUT_PROTOCOL"を使用します(仕様書"11.9 Graphics Output Protocol(P.464)"参照)。ただし、このプロトコルはSystemTableのメンバにはありません。

実はほとんどのプロトコルはSystemTable->BootServicesの中の関数を使用してプロトコルの先頭アドレスを取得する必要があります(仕様書"4.4 EFI Boot Services Table(P.80)"参照)。BootServicesはEFI_BOOT_SERVICESという構造体で、主にブートローダー向けにUEFIが提供している関数(サービス)を持ちます。@<fn>{another_service}
//footnote[another_service]["～_SERVICES"には他に"EFI_RUNTIME_SERVICES"があり、こちらもSystemTable->RuntimeServicesという形でSystemTableから参照できます。]

SystemTableのメンバに無いほとんどのプロトコルは、SystemTable->BootServices->LocateProtocol関数(仕様書"6.3 Protocol Handler Services(P.184)"参照)でプロトコル構造体の先頭アドレスを取得できます。LocateProtocol関数は、プロトコル毎に一意に決められている"GUID"からプロトコルの先頭アドレスを取得する関数です。"GUID"も仕様書に記載されています。例えばEFI_GRAPHICS_OUTPUT_PROTOCOLの場合、@<img>{efi_graphics_output_protocol_guid}のように記載されています。

//image[efi_graphics_output_protocol_guid][EFI_GRAPHICS_OUTPUT_PROTOCOLのGUID]{
//}

LocateProtocolの定義は@<list>{locate_protocol_definition}の通りです。

//list[locate_protocol_definition][LocateProtocolの定義][c]{
unsigned long long (*LocateProtocol)(
	struct EFI_GUID *Protocol,
	void *Registration,
	void **Interface);
//}

引数の意味は以下の通りです。

 : struct EFI_GUID *Protocol
    取得したいプロトコルのGUIDを指定。
 : void *Registration
    オプショナル。必要に応じてレジストレーションキーというものを指定するらしい。本書では使用しない(NULL指定)。
 : void **Interface
    プロトコル構造体の先頭アドレスを格納するポインタを指定。

SystemTableと同じく、EFI_GRAPHICS_OUTPUT_PROTOCOLも、efi_init関数でグローバル変数へ格納することにします。LocateProtocol処理を追加したefi_initは@<list>{sample3_1_draw_rect_efi}の通りです。

//listnum[sample3_1_draw_rect_efi][sample3_1_draw_rect/efi.c][c]{
#include "efi.h"
#include "common.h"

struct EFI_SYSTEM_TABLE *ST;
struct EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;

void efi_init(struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_GUID gop_guid = {0x9042a9de, 0x23dc, 0x4a38, \
				    {0x96, 0xfb, 0x7a, 0xde,	\
				     0xd0, 0x80, 0x51, 0x6a}};

	ST = SystemTable;
	ST->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	ST->BootServices->LocateProtocol(&gop_guid, NULL, (void **)&GOP);
}
//}

これで、EFI_GRAPHICS_OUTPUT_PROTOCOLを取得できました。EFI_GRAPHICS_OUTPUT_PROTOCOLの定義は@<list>{efi_graphics_output_protocol_definition}の通りです(仕様書"11.9.1 Blt Buffer(P.466)")。なお、例によって、定義の内容は本書で使用するメンバのみに限定しています。定義の全体は仕様書を確認してください。

//list[efi_graphics_output_protocol_definition][EFI_GRAPHICS_OUTPUT_PROTOCOLの定義][c]{
struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
	unsigned long long _buf[3];
	struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
		unsigned int MaxMode;
		unsigned int Mode;
		struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
			unsigned int Version;
			unsigned int HorizontalResolution;
			unsigned int VerticalResolution;
			enum EFI_GRAPHICS_PIXEL_FORMAT {
				PixelRedGreenBlueReserved8BitPerColor,
				PixelBlueGreenRedReserved8BitPerColor,
				PixelBitMask,
				PixelBltOnly,
				PixelFormatMax
			} PixelFormat;
		} *Info;
		unsigned long long SizeOfInfo;
		unsigned long long FrameBufferBase;
	} *Mode;
};
//}

画面描画はフレームバッファへピクセルデータを書き込むことで行います。GOP->Mode->FrameBufferBase変数からフレームバッファの先頭アドレスを取得できます。ピクセルフォーマットはGOP->Mode->Info->PixelFormat変数から確認できます。PixelFormatは"enum EFI_GRAPHICS_PIXEL_FORMAT"というenum定数です。このenum定数により、ピクセルフォーマットが何であるかを確認できます(仕様書"11.9.1 Blt Buffer(P.467)")。

EFI_GRAPHICS_PIXEL_FORMATの定義は@<list>{enum_pixel_format}の通りです。筆者が動作確認に使用できる環境(ThinkPad E450とQEMUのOVMF)では全て、ピクセルフォーマットはPixelBlueGreenRedReserved8BitPerColorであったため、本書ではピクセルフォーマットは「BGR+Reserved各8ビット」を想定します。もし異なる場合は、適宜読み替えてください。

//list[enum_pixel_format][EFI_GRAPHICS_PIXEL_FORMATの定義][c]{
enum EFI_GRAPHICS_PIXEL_FORMAT {
	PixelRedGreenBlueReserved8BitPerColor,
	PixelBlueGreenRedReserved8BitPerColor,
	PixelBitMask,
	PixelBltOnly,
	PixelFormatMax
};
//}

なお、GOP->Mode->Info->PixelFormatの値を確認するには画面に数値を出力できる必要があります。次の章のサンプル(sample4_1_get_pointer_stateディレクトリ)で、16進数で数値を出力する関数puthをcommon.cに追加しますので、参考にしてみてください。

加えて、ピクセルフォーマットのバイト列を定義するEFI_GRAPHICS_OUTPUT_BLT_PIXEL構造体も@<list>{struct_efi_graphics_output_blt_pixel}に示します(仕様書"11.9.1 Blt Buffer(P.474)")。

//list[struct_efi_graphics_output_blt_pixel][EFI_GRAPHICS_OUTPUT_BLT_PIXELの定義][c]{
struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL {
	unsigned char Blue;
	unsigned char Green;
	unsigned char Red;
	unsigned char Reserved;
};
//}

以上で、画面に矩形を描く為のUEFIの定義の追加は終了です。

== 矩形を描くサンプルを実装
フレームバッファの先頭アドレスもピクセルフォーマットも分かったので、後はピクセルフォーマットに従ったバイト列をフレームバッファの領域へ書き込むだけです。まずは1ピクセルを指定の座標に描く関数"draw_pixel"を作成します(@<list>{draw_pixel})。グラフィックス関係の処理は"graphics.c"というソースファイルを作成し、そこへ追加します。

//listnum[draw_pixel][sample3_1_draw_rect/graphics.c(draw_pixel関数)][c]{
void draw_pixel(unsigned int x, unsigned int y,
		struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color)
{
	unsigned int hr = GOP->Mode->Info->HorizontalResolution;
	struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL *base =
		(struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)GOP->Mode->FrameBufferBase;
	struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL *p = base + (hr * y) + x;

	p->Blue = color.Blue;
	p->Green = color.Green;
	p->Red = color.Red;
	p->Reserved = color.Reserved;
}
//}

@<list>{draw_pixel}では、以下を行っています。

 1. @<code>{GOP->Mode->Info->HorizontalResolution}で水平解像度を取得
 2. 水平解像度の値と与えられたX、Y座標値から1ピクセルを描くアドレスを計算
 3. 2.へcolor変数の値を書き込み

なお、ここで取得している水平解像度はUEFIファームウェアがデフォルトで認識している画面モードの解像度で、筆者のThinkPad E450の場合640ピクセルでした。画面モードはEFI_GRAPHICS_OUTPUT_PROTOCOLのSetMode関数で変更できますので興味があれば試してみてください(仕様書"11.9.1 Blt Buffer(P.473)")。SetMode関数ではモード番号を指定しますが、この番号の最大値は同じくEFI_GRAPHICS_OUTPUT_PROTOCOLのMode->MaxModeから確認できます(仕様書"11.9.1 Blt Buffer(P.466)")。

次に、draw_pixelを使用して、draw_rectを@<list>{draw_rect}のように実装できます。

//listnum[draw_rect][sample3_1_draw_rect/graphics.c(draw_rect関数)][c]{
void draw_rect(struct RECT r, struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL c)
{
	unsigned int i;

	for (i = r.x; i < (r.x + r.w); i++)
		draw_pixel(i, r.y, c);
	for (i = r.x; i < (r.x + r.w); i++)
		draw_pixel(i, r.y + r.h - 1, c);

	for (i = r.y; i < (r.y + r.h); i++)
		draw_pixel(r.x, i, c);
	for (i = r.y; i < (r.y + r.h); i++)
		draw_pixel(r.x + r.w - 1, i, c);
}
//}

なお、RECT構造体は@<list>{rect_definition}のようにgraphics.hで定義します。

//list[rect_definition][RECT構造体の定義][c]{
struct RECT {
	unsigned int x, y;
	unsigned int w, h;
};
//}

以上を使用して、画面に矩形を描く"rect"コマンドを追加してみます。追加後のshell.cは@<list>{sample3_1_draw_rect_shell_c}の通りです。

#@# sample3.1: 画面描画サンプル
#@#   => 画面に矩形を描くサンプル
#@#   => draw_rect()を追加しておく
#@#   => BootServicesを定義するついでに、ここからWDTのセットを行う
//listnum[sample3_1_draw_rect_shell_c][sample3_1_draw_rect/shell.c][c]{
#include "common.h"
#include "graphics.h"
#include "shell.h"

#define MAX_COMMAND_LEN	100

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};	/* 追加 */

	while (TRUE) {
		puts(L"poiOS> ");
		if (gets(com, MAX_COMMAND_LEN) <= 0)
			continue;

		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		else if (!strcmp(L"rect", com))	/* 追加 */
			draw_rect(r, white);	/* 追加 */
		else
			puts(L"Command not found.\r\n");
	}
}
//}

実機で実行した様子は@<img>{sample3_1_draw_rect}の通りです。

//image[sample3_1_draw_rect][rectコマンド実行の様子]{
//}

== GUIモードを追加する
GUIのモードへ切り替える"gui"コマンドを追加してみます。サンプルのディレクトリは"sample3_2_add_gui_mode"です。

GUIモードとしては、まずはアイコンに見立てた矩形を1つ配置するだけです。GUIモードの実装は、新たにgui.cを作成し、そこへ記述することにします。そして、シェルからは、guiコマンドが実行されたときに、gui.cに実装されているgui関数を呼び出すこととします。

gui.cの実装を@<list>{sample3_2_add_gui_mode_gui_c}に示します。

//listnum[sample3_2_add_gui_mode_gui_c][sample3_2_add_gui_mode/gui.c][c]{
#include "efi.h"
#include "common.h"
#include "graphics.h"
#include "gui.h"

void gui(void)
{
	struct RECT r = {10, 10, 20, 20};

	ST->ConOut->ClearScreen(ST->ConOut);

	/* ファイルアイコンに見立てた矩形を描画 */
	draw_rect(r, white);

	while (TRUE);
}
//}

gui関数を呼び出すguiコマンドを追加したshell.cを@<list>{sample3_2_add_gui_mode_shell_c}に示します。

#@# sample3.2: sample2.2.をos_modoki(あるいはmock-os)へリネームし、guiコマンド追加
#@#   => guiコマンド実行時の内容はsample3.1
#@#   => GUIはこの段階からソースコード分けたほうが良いかも
#@#   => 出力した文字に重なって描かれる事を確認し、画面クリアを追加する
#@#     => ファイル操作でフォントを実装せずに済ますことへの伏線
//listnum[sample3_2_add_gui_mode_shell_c][sample3_2_add_gui_mode/shell.c][c]{
#include "common.h"
#include "graphics.h"
#include "shell.h"
#include "gui.h"	/* 追加 */

#define MAX_COMMAND_LEN	100

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};

	while (TRUE) {
		puts(L"poiOS> ");
		if (gets(com, MAX_COMMAND_LEN) <= 0)
			continue;

		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		else if (!strcmp(L"rect", com))
			draw_rect(r, white);
		else if (!strcmp(L"gui", com))	/* 追加 */
			gui();	/* 追加 */
		else
			puts(L"Command not found.\r\n");
	}
}
//}

@<img>{sample3_2_add_gui_mode_1}のようにguiコマンドを実行すると、アイコンに見立てた矩形をひとつだけ描いた状態の画面(@<img>{sample3_2_add_gui_mode_2})へ遷移します。

//image[sample3_2_add_gui_mode_1][guiコマンド実行の様子]{
//}
//image[sample3_2_add_gui_mode_2][アイコン一つ表示するだけのGUIモード]{
//}

= マウス入力を取得する
次はマウスを使ってみます。この章では、マウス入力を取得し、マウスの動きに合わせてマウスカーソルを描画してみます。

== EFI_SIMPLE_POINTER_PROTOCOL
マウスの入力値を取得するにはEFI_SIMPLE_POINTER_PROTOCOLを使用します(仕様書"11.5 Simple Pointer Protocol(P.439)")。本書で使用する箇所の定義は@<list>{struct_efi_simple_pointer_protocol}の通りです。

//list[struct_efi_simple_pointer_protocol][EFI_SIMPLE_POINTER_PROTOCOLの定義]{
struct EFI_SIMPLE_POINTER_PROTOCOL {
	unsigned long long (*Reset)(
		struct EFI_SIMPLE_POINTER_PROTOCOL *This,
		unsigned char ExtendedVerification);
	unsigned long long (*GetState)(
		struct EFI_SIMPLE_POINTER_PROTOCOL *This,
		struct EFI_SIMPLE_POINTER_STATE *State);
	void *WaitForInput;
};
//}

EFI_SIMPLE_POINTER_PROTOCOLでは、Reset関数でポインティングデバイス(マウス)をリセットし、GetState関数で状態を取得します。

Reset関数の引数の意味は以下の通りです。

 : unsigned char ExtendedVerification
    拡張の検査(ExtendedVerification)を行うか否かのフラグ。TRUEが設定されていると拡張検査が行われる。どのような拡張検査が行われるかはファームウェア依存。本書では特にそのような検査は不要であるためFALSEを指定。

また、GetState関数の引数については以下の通りです。

  : struct EFI_SIMPLE_POINTER_STATE *State
     ポインティングデバイスの状態を指定されたポインタ変数へ格納。

なお、状態を表すEFI_SIMPLE_POINTER_STATE構造体の定義は@<list>{efi_simple_pointer_state_definition}の通りです。

//list[efi_simple_pointer_state_definition][EFI_SIMPLE_POINTER_STATEの定義][c]{
struct EFI_SIMPLE_POINTER_STATE {
	int RelativeMovementX;	/* X軸方向の相対移動量 */
	int RelativeMovementY;	/* Y軸方向の相対移動量 */
	int RelativeMovementZ;	/* Z軸方向の相対移動量 */
	unsigned char LeftButton;	/* 左ボタン押下状態(TRUE=1,FALSE=0) */
	unsigned char RightButton;	/* 右ボタン押下状態(TRUE=1,FALSE=0) */
};
//}

@<list>{efi_simple_pointer_state_definition}に関し、各メンバの意味はコメントの通りです。なお、"RelativeMovementZ"は普通のマウスを使う限り常に0でした(特殊なマウスを持っていないので、筆者が試す限り0以外見たことがありません)。

EFI_SIMPLE_POINTER_PROTOCOL(@<list>{struct_efi_simple_pointer_protocol})の"WaitForInput"は、マウス入力があるまで待機するためのイベントです。EFI_SIMPLE_TEXT_INPUT_PROTOCOLのWaitForKeyと同様に、SystemTable->BootServices->WaitForEvent関数へ渡して使うことができます。

=== マウスの状態を見てみる(pstatコマンド)
EFI_SIMPLE_POINTER_PROTOCOLを使うことでマウスの状態を取得できることが分かったので、試しにマウスの状態をコンソール上へダンプしてみます。ここではpstatというコマンドとしてマウス状態ダンプの機能を追加してみます。サンプルのディレクトリは"sample4_1_get_pointer_state"です。

まず、LocateProtocol関数でEFI_SIMPLE_POINTER_PROTOCOLの構造体の先頭アドレスを取得します。そのため、efi.cのefi_init関数へLocateProtocolの処理を追加し、グローバル変数としてアクセスできるようにします(@<list>{sample4_1_get_pointer_state_efi})。

//listnum[sample4_1_get_pointer_state_efi][sample4_1_get_pointer_state/efi.c][c]{
#include "efi.h"
#include "common.h"

struct EFI_SYSTEM_TABLE *ST;
struct EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
struct EFI_SIMPLE_POINTER_PROTOCOL *SPP;	/* 追加 */

void efi_init(struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_GUID gop_guid = {0x9042a9de, 0x23dc, 0x4a38, \
				    {0x96, 0xfb, 0x7a, 0xde,	\
				     0xd0, 0x80, 0x51, 0x6a}};
	/* 追加(ここから) */
	struct EFI_GUID spp_guid = {0x31878c87, 0xb75, 0x11d5, \
				    {0x9a, 0x4f, 0x0, 0x90,    \
				     0x27, 0x3f, 0xc1, 0x4d}};
     	/* 追加(ここまで) */

	ST = SystemTable;
	ST->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	ST->BootServices->LocateProtocol(&gop_guid, NULL, (void **)&GOP);
	/* 追加 */
	ST->BootServices->LocateProtocol(&spp_guid, NULL, (void **)&SPP);
}
//}

pstatコマンドを追加したshell.cは@<list>{sample4_1_get_pointer_state_shell_c}の通りです。

#@# sample4.1: マウス入力値取得サンプル(ポーリング版)
#@#   => 090_pointer_get_state
//listnum[sample4_1_get_pointer_state_shell_c][sample4_1_get_pointer_state/shell.c][c]{
#include "efi.h"	/* 追加 */
#include "common.h"
#include "graphics.h"
#include "shell.h"
#include "gui.h"

#define MAX_COMMAND_LEN	100

/* 追加(ここから) */
void pstat(void)
{
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	unsigned long long waitidx;

	SPP->Reset(SPP, FALSE);

	while (1) {
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput),
					       &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			puth(s.RelativeMovementX, 8);
			puts(L" ");
			puth(s.RelativeMovementY, 8);
			puts(L" ");
			puth(s.RelativeMovementZ, 8);
			puts(L" ");
			puth(s.LeftButton, 1);
			puts(L" ");
			puth(s.RightButton, 1);
			puts(L"\r\n");
		}
	}
}
/* 追加(ここまで) */

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};

	while (TRUE) {
		puts(L"poiOS> ");
		if (gets(com, MAX_COMMAND_LEN) <= 0)
			continue;

		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		/* ・・・省略・・・ */
		else if (!strcmp(L"pstat", com))	/* 追加 */
			pstat();	/* 追加 */
		else
			puts(L"Command not found.\r\n");
	}
}
//}

@<list>{sample4_1_get_pointer_state_shell_c}に関して、pstat関数ではputh関数を使用して画面へ数値を表示しています。puth関数はこのサンプルからcommon.cへ追加した関数で、引数で指定した数値を16進数で表示します。第1引数が表示する数値で、第2引数が表示する際の桁数です。

#@# sample4.2: マウス入力値取得サンプル(イベント版)
#@#   => 無し
#@#   => というか、SimplePointerProtocolのWaitForInputはBS->WaitForEvent()用なので、ブロックで待つことしかできない
#@#   => イベントハンドラを使用するやり方はAbsolutePointerProtocolを使ってやるようだが、まだよくわかっていない
#@#   => ひとまず、マウスの処理は、GUIの周期的なループ処理の中でやることにしよう

最後に@<list>{sample4_1_get_pointer_state_shell_c}のサンプルの実行の様子を@<img>{sample4_1_get_pointer_state}に示します。

//image[sample4_1_get_pointer_state][pstatコマンド実行の様子]{
//}

== マウスカーソルを追加する
グラフィックを描画する方法が分かり、マウスの入力値の取得もできたので、アイコンもどきの矩形を表示するだけだったGUIモードへマウスカーソルを表示する機能を追加してみます。サンプルのディレクトリは"sample4_2_add_cursor"です。

実装の仕方としては、ここではとにかく簡単にするため、「マウスカーソルは1ドット」で作ってみます。マウスカーソルの機能を追加したgui.cを@<list>{sample4_2_add_cursor_gui_c}に示します。

#@# sample4.2: マウスカーソル機能追加
#@#   => sample3.2.のguiへマウスカーソルの機能を追加する
#@#   => マウスカーソルオーバーで矩形の色を変え、クリックでまた色を変える
#@#   => 簡単のため、マウスカーソルは1px
#@#   => FrameBufferを読みだすとReservedには0x00が入っている。書き込む時、Reservedは0xffでないと表示されないため、0xffへ書き換える
//listnum[sample4_2_add_cursor_gui_c][sample4_2_add_cursor/gui.c][c]{
#include "efi.h"
#include "common.h"
#include "graphics.h"
#include "shell.h"
#include "gui.h"

/* 追加(ここから) */
struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL cursor_tmp = {0, 0, 0, 0};
int cursor_old_x;
int cursor_old_y;

void draw_cursor(int x, int y)
{
	draw_pixel(x, y, white);
}

void save_cursor_area(int x, int y)
{
	cursor_tmp = get_pixel(x, y);
	cursor_tmp.Reserved = 0xff;
}

void load_cursor_area(int x, int y)
{
	draw_pixel(x, y, cursor_tmp);
}

void put_cursor(int x, int y)
{
	if (cursor_tmp.Reserved)
		load_cursor_area(cursor_old_x, cursor_old_y);

	save_cursor_area(x, y);

	draw_cursor(x, y);

	cursor_old_x = x;
	cursor_old_y = y;
}
/* 追加(ここまで) */

void gui(void)
{
	struct RECT r = {10, 10, 20, 20};
	/* 追加・変更(ここから) */
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	int px = 0, py = 0;
	unsigned long long waitidx;
	unsigned char is_highlight = FALSE;

	ST->ConOut->ClearScreen(ST->ConOut);
	SPP->Reset(SPP, FALSE);

	/* ファイルアイコンに見立てた矩形を描画 */
	draw_rect(r, white);

	while (TRUE) {
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput), &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			/* マウスカーソル座標更新 */
			px += s.RelativeMovementX >> 13;
			if (px < 0)
				px = 0;
			else if (GOP->Mode->Info->HorizontalResolution <=
				 (unsigned int)px)
				px = GOP->Mode->Info->HorizontalResolution - 1;
			py += s.RelativeMovementY >> 13;
			if (py < 0)
				py = 0;
			else if (GOP->Mode->Info->VerticalResolution <=
				 (unsigned int)py)
				py = GOP->Mode->Info->VerticalResolution - 1;

			/* マウスカーソル描画 */
			put_cursor(px, py);

			/* ファイルアイコン処理 */
			if (is_in_rect(px, py, r)) {
				if (is_highlight) {
					draw_rect(r, yellow);
					is_highlight = TRUE;
				}
			} else {
				if (is_highlight) {
					draw_rect(r, white);
					is_highlight = FALSE;
				}
			}
		}
	}
	/* 追加・変更(ここまで) */
}
//}

gui関数へ追加した処理の流れとしては、ClearScreen関数からdraw_rect関数までで、画面とマウスの初期化を行い、アイコン(矩形)の描画を行っています。@<code>{while(TRUE)}内の定常処理では以下の流れで処理を行っています。

 1. WaitForEventで待機し、GetState関数でマウス入力値を取得
 2. マウスカーソル座標更新
 3. マウスカーソル描画(put_cursor関数)
 4. ファイルアイコン処理

なお、put_cursor関数は引数で指定した座標へマウスカーソルを移動させる関数です。put_cursorでは以下の流れで処理を行います。

 1. 退避していたピクセルデータを復帰(load_cursor_area)
 2. 描画するカーソル位置のピクセルデータを退避(save_cursor_area)
 3. カーソル描画(draw_cursor)
 4. カーソル位置を退避(cursor_old_x,cursor_old_y)

save_cursor_area関数内に関して、get_pixel関数はgraphics.cへ新たに追加した関数で、フレームバッファからピクセルデータを取得します。ピクセルデータはEFI_GRAPHICS_OUTPUT_BLT_PIXEL構造体の形式です。フレームバッファから読み出した際のReservedメンバの値は0なのですが、フレームバッファへ書き込むときに0を指定していると何も表示されないので、save_cursor_area関数内でReservedは0xffで上書きしています。これを利用して、put_cursor関数内では、退避済みのピクセルデータがあるか否かをReservedメンバの値で確認しています。

gui関数内の@<code>{while(TRUE)}内の処理に戻り、"2. マウスカーソル座標更新"のマウスの移動量計算は、pstatコマンドで確認した結果からおおよそ算出したものです。下位12ビットが全て0であり、12ビットシフトするだけでは移動量としては大きすぎたため、13ビット右シフトしています。もしpstatコマンドで確認した結果が異なるようであれば、適宜修正してください。

"4. ファイルアイコン処理"のアイコン更新処理では、現在のマウスカーソル座標がアイコン(矩形)の範囲内であればアイコンをハイライト色で上書きし、そうでなければ元の色で上書きしています。

#@# 今回新たにget_pixel関数とis_in_rect関数が登場しました。それぞれgraphics.cとcommon.cへ追加されています。別段、特殊な実装方法をしているわけでもないので、興味があれば見てみてください。get_pixel関数は、引数で指定された座標のピクセル値をフレームバッファから取得し、is_in_rect関数は関数名の通り、第1・第2引数で指定した座標が第3引数で指定した矩形内であるか否かを返すだけです。

サンプルを実行し、シェル上でguiコマンドを実行することでGUIモードへ遷移すると、@<img>{sample4_2_add_cursor_1}の画面になります。アイコン(矩形)の右側にある点がマウスカーソルです@<fn>{like_a_dust}。マウスカーソルがアイコンの上にのると、枠が黄色にハイライト表示されます(@<img>{sample4_2_add_cursor_2})@<fn>{mierunoka}。
//footnote[like_a_dust][ゴミのように見えますがマウスカーソルなのです。。。]
//footnote[mierunoka][白黒の印刷なので印刷上は分からないかも知れませんが。。]

//image[sample4_2_add_cursor_1][マウスカーソルが表示される様子]{
//}
//image[sample4_2_add_cursor_2][マウスカーソルがアイコンにのるとハイライト表示]{
//}

= ファイル読み書き
マウスカーソルを用意できたので、何かクリックしてみたいです。UEFIのファームウェアはFATファイルシステム上のファイルを操作するプロトコルを持っていますので、それを使ってファイル操作機能を追加してみます。

== EFI_SIMPLE_FILE_SYSTEM_PROTOCOLとEFI_FILE_PROTOCOL
UEFIではFATファイルシステムを扱えます。ファイルシステムを操作するためのプロトコルが"EFI_SIMPLE_FILE_SYSTEM_PROTOCOL"と"EFI_FILE_PROTOCOL"です(仕様書"12.4 Simple File System Protocol(P.494)"と"12.5 EFI File Protocol(P.497)")。

EFI_SIMPLE_FILE_SYSTEM_PROTOCOLの定義は@<list>{efi_simple_file_system_protocol_definition}の通りです。

//list[efi_simple_file_system_protocol_definition][EFI_SIMPLE_FILE_SYSTEM_PROTOCOLの定義][c]{
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
	unsigned long long Revision;
	unsigned long long (*OpenVolume)(
		struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
		struct EFI_FILE_PROTOCOL **Root);
};
//}

@<list>{efi_simple_file_system_protocol_definition}は定義の全体で、関数はOpenVolumeのみを持っています。OpenVolumeは名前の通りボリュームを開く関数です。"ボリューム"はディスクのパーティションに当たるもので、Windows OSで言うところの"ドライブ"に相当するものです。OpenVolume関数の引数の意味は以下の通りです。

 : struct EFI_FILE_PROTOCOL **Root
    ボリュームを開いて得られた最上位階層のディレクトリ(ルートディレクトリ)。

UEFIではファイル/ディレクトリの各エントリをEFI_FILE_PROTOCOLという構造体で扱います。OpenVolumeで得られるルートディレクトリエントリもEFI_FILE_PROTOCOLです。EFI_FILE_PROTOCOLの定義を@<list>{efi_file_protocol_definition}に示します(本書で使用するもののみ定義しています)。

//list[efi_file_protocol_definition][EFI_FILE_PROTOCOLの定義][c]{
struct EFI_FILE_PROTOCOL {
	unsigned long long _buf;
	unsigned long long (*Open)(struct EFI_FILE_PROTOCOL *This,
				   struct EFI_FILE_PROTOCOL **NewHandle,
				   unsigned short *FileName,
				   unsigned long long OpenMode,
				   unsigned long long Attributes);
	unsigned long long (*Close)(struct EFI_FILE_PROTOCOL *This);
	unsigned long long _buf2;
	unsigned long long (*Read)(struct EFI_FILE_PROTOCOL *This,
				   unsigned long long *BufferSize,
				   void *Buffer);
	unsigned long long (*Write)(struct EFI_FILE_PROTOCOL *This,
				    unsigned long long *BufferSize,
				    void *Buffer);
	unsigned long long _buf3[4];
	unsigned long long (*Flush)(struct EFI_FILE_PROTOCOL *This);
};
//}

@<list>{efi_file_protocol_definition}の各関数の使い方は、以降の節で説明します。

== ルートディレクトリ直下のファイル/ディレクトリを一覧表示(ls)
ルートディレクトリ直下のファイル/ディレクトリを一覧表示する"ls"コマンドを作成してみます。サンプルのディレクトリは"sample5_1_ls"です。

まずは、EFI_SIMPLE_FILE_SYSTEM_PROTOCOLを使えるように、efi.cのefi_init関数へLocateProtocol関数の処理を追加します(@<list>{sample5_1_ls_efi_c})。

//listnum[sample5_1_ls_efi_c][sample5_1_ls/efi.c][c]{
/* ・・・省略・・・ */
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;	/* 追加 */

void efi_init(struct EFI_SYSTEM_TABLE *SystemTable)
{
	/* ・・・省略・・・ */
	/* 追加(ここから) */
	struct EFI_GUID sfsp_guid = {0x0964e5b22, 0x6459, 0x11d2, \
				     {0x8e, 0x39, 0x00, 0xa0,	  \
				      0xc9, 0x69, 0x72, 0x3b}};
      	/* 追加(ここまで) */
	/* ・・・省略・・・ */
	/* 追加 */
	ST->BootServices->LocateProtocol(&sfsp_guid, NULL, (void **)&SFSP);
}
//}

これで、グローバル変数"SFSP"を通してEFI_SIMPLE_FILE_SYSTEM_PROTOCOLを使用できるようになりました。次は、SFSP->OpenVolume関数を呼び出すことでルートディレクトリを開きます。コード例は@<list>{sample_open_volume}の通りです。

//listnum[sample_open_volume][OpenVolume関数の使用例][c]{
struct EFI_FILE_PROTOCOL *root;
SFSP->OpenVolume(SFSP, &root);
//}

EFI_FILE_PROTOCOLがディレクトリである場合、Read関数を呼び出す事で、ディレクトリ内に存在するファイル/ディレクトリ名を取得できます。EFI_FILE_PROTOCOLのRead関数の定義は@<list>{read_definition}の通りです。

//list[read_definition][EFI_FILE_PROTOCOLのRead関数の定義][c]{
unsigned long long (*Read)(struct EFI_FILE_PROTOCOL *This,
			   unsigned long long *BufferSize,
			   void *Buffer);
//}

引数の意味は以下の通りです。

 : unsigned long long *BufferSize
    第3引数"Buffer"のサイズ(バイト指定)を格納した変数のポインタを指定。Read関数実行後、このポインタで参照された変数にはreadしたデータサイズが格納される。EFI_FILE_PROTOCOLがディレクトリの場合、全てのファイル/ディレクトリ名を取得し終えると、0が格納される。
 : void *Buffer
    readしたデータを格納するバッファの先頭アドレス。EFI_FILE_PROTOCOLがディレクトリの場合、1回のreadにつき1つのファイル/ディレクトリ名を格納する。

ファイル/ディレクトリに対する一通りの処理を終えたら、EFI_FILE_PROTOCOLのClose関数を実行します。Close関数の定義は@<list>{close_definition}の通りです。

//list[close_definition][EFI_FILE_PROTOCOLのClose関数の定義][c]{
unsigned long long (*Close)(struct EFI_FILE_PROTOCOL *This);
//}

以上を踏まえて、起動ディスク直下のファイル/ディレクトリを一覧表示するコマンドを追加してみます。

まずは、ファイルの付帯情報を管理する構造体の配列"struct FILE file_list[]"を作成します。管理する付帯情報としては、今はまだ「ファイル名」だけです。行数は少ないのですが、file.hとfile.cというファイル名で@<list>{sample_5_1_ls_file_h}と@<list>{sample_5_1_ls_file_c}のソースコードを作成します。

//listnum[sample_5_1_ls_file_h][sample_5_1_ls/file.h][c]{
#ifndef _FILE_H_
#define _FILE_H_

#include "graphics.h"

#define MAX_FILE_NAME_LEN	4
#define MAX_FILE_NUM	10
#define MAX_FILE_BUF	1024

struct FILE {
	unsigned short name[MAX_FILE_NAME_LEN];
};

extern struct FILE file_list[MAX_FILE_NUM];

#endif
//}

//listnum[sample_5_1_ls_file_c][sample_5_1_ls/file.c][c]{
#include "file.h"

struct FILE file_list[MAX_FILE_NUM];
//}

そして、"ls"コマンドを追加したshell.cの内容は@<list>{sample5_1_ls_shell_c}の通りです。

#@# sample5.1: ファイルリスト表示サンプル
#@#   => 061_fs_ls_root
//listnum[sample5_1_ls_shell_c][sample5_1_ls/shell.c][c]{
/* ・・・省略・・・ */

/* 追加(ここから) */
int ls(void)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	unsigned long long buf_size;
	unsigned char file_buf[MAX_FILE_BUF];
	struct EFI_FILE_INFO *file_info;
	int idx = 0;
	int file_num;

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	while (1) {
		buf_size = MAX_FILE_BUF;
		status = root->Read(root, &buf_size, (void *)file_buf);
		assert(status, L"root->Read");
		if (!buf_size) break;

		file_info = (struct EFI_FILE_INFO *)file_buf;
		strncpy(file_list[idx].name, file_info->FileName,
			MAX_FILE_NAME_LEN - 1);
		file_list[idx].name[MAX_FILE_NAME_LEN - 1] = L'\0';
		puts(file_list[idx].name);
		puts(L" ");

		idx++;
	}
	puts(L"\r\n");
	file_num = idx;

	root->Close(root);

	return file_num;
}
/* 追加(ここまで) */

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};

	while (TRUE) {
		puts(L"poiOS> ");
		if (gets(com, MAX_COMMAND_LEN) <= 0)
			continue;

		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		/* ・・・省略・・・ */
		else if (!strcmp(L"ls", com))	/* 追加 */
			ls();	/* 追加 */
		else
			puts(L"Command not found.\r\n");
	}
}
//}

@<list>{sample5_1_ls_shell_c}では、ls関数実行の都度OpenVolume関数でルートディレクトリを開いています。これは、Readで一通りファイル/ディレクトリエントリを取得しきってしまうと、再度取得するには一度Closeし、再度OpenVolumeする必要があるためです。取得したファイル/ディレクトリエントリをキャッシュしておくという考え方もありますが、ここでは新鮮な結果を返すために(また、簡単のために)、都度OpenVolumeし、Readしています。

また、@<list>{sample5_1_ls_shell_c}のls関数ではassert関数を呼び出しています。assert関数は引数で渡されたステータス値をチェックし、ステータス値が成功(=0)以外であれば、同じく引数で指定されたメッセージを出力して"@<code>{while(1);}"で固めます。なお、内部的にはステータスチェックとメッセージ表示はcheck_warn_errorという関数に分かれていて、assert関数はcheck_warn_errorの戻り値に応じて"@<code>{while(1);}"で固めるだけです。詳しくはcommon.cのソースコードを見てみてください。

サンプルを実行する際はUSBフラッシュメモリ直下にファイルをいくつか配置してみてください。例えば、"abc"と"hlo"の2つのファイルを置いてみると、@<img>{sample5_1_ls}の様に表示されます。

//image[sample5_1_ls][ファイル/ディレクトリ一覧を表示している様子]{
//}

#@# ===[column] 指定したボリューム番号のボリュームを開くには
#@# UEFIのファームウェアによっては、OpenVolumeで開かれるボリュームが起動ディスクではない場合があるようです。例えば、GDP WINでOpenVolumeを試した際に、起動ディスクとは別のボリュームを開いていました。

#@# UEFIシェル@<fn>{uefi_shell}には、"fs0:"のようにボリューム番号指定でボリュームを開くコマンドがあります。そのため、ボリューム番号を指定してボリュームを開く事ができることはわかっているのですが、UEFIファームウェアをどのように叩けばそれが行えるのかが、まだわかっていません。
#@# //footnote[uefi_shell][@<href>{https://github.com/tianocore/edk2/tree/master/EdkShellBinPkg/FullShell/X64}]

#@# 書かれている内容をまだ試せてはいないのですが、情報としてはStack Overflowの以下の質問ページが参考になりそうです。

#@#  * uefi - Can I write on my local filesystem using EFI
#@#  ** @<href>{https://stackoverflow.com/questions/32324109/can-i-write-on-my-local-filesystem-using-efi}

#@# なお、そもそもUEFIを試せる実機がThinkPad E450とGPD WINの2つしか無いため、「デフォルトのOpenVolumeが起動ディスクとは別のボリュームを開く」という挙動がUEFIファームウェアの実装として珍しいものなのか、一般的なものなのかもわかってないです。(GPD WINのようなケースはレアなのかもしれません。)

== GUIモードでファイル/ディレクトリ一覧を表示する
これまでGUIモードではファイルのアイコンに見立てた矩形が表示されるだけでした。ここでは、矩形内にファイル名を配置してみます。サンプルプログラムは"sample5_2_gui_ls"のディレクトリです。

処理の流れとしては、ファイル名のリストを表示した後、各ファイル名を矩形で囲みます。なお、簡単のためにファイル名は3文字とします。

まず、ファイルの付帯情報として矩形の位置・大きさとハイライト状態を追加します。変更箇所はfile.hで、@<list>{sample5_2_gui_ls_file_h}の通りです。

//listnum[sample5_2_gui_ls_file_h][sample5_2_gui_ls/file.h][c]{
/* ・・・省略・・・ */
struct FILE {
	struct RECT rect;	/* 追加 */
	unsigned char is_highlight;	/* 追加 */
	unsigned short name[MAX_FILE_NAME_LEN];
};
/* ・・・省略・・・ */
//}

以上を踏まえてgui.cへ機能を追加すると@<list>{sample5_2_gui_ls_gui_c}の通りです。

//listnum[sample5_2_gui_ls_gui_c][sample5_2_gui_ls/gui.c][c]{
#include "efi.h"
#include "common.h"
#include "file.h"
#include "graphics.h"
#include "shell.h"
#include "gui.h"

#define WIDTH_PER_CH	8	/* 追加 */
#define HEIGHT_PER_CH	20	/* 追加 */

/* ・・・省略・・・ */

/* 追加(ここから) */
int ls_gui(void)
{
	int file_num;
	struct RECT t;
	int idx;

	ST->ConOut->ClearScreen(ST->ConOut);

	file_num = ls();

	t.x = 0;
	t.y = 0;
	t.w = (MAX_FILE_NAME_LEN - 1) * WIDTH_PER_CH;
	t.h = HEIGHT_PER_CH;
	for (idx = 0; idx < file_num; idx++) {
		file_list[idx].rect.x = t.x;
		file_list[idx].rect.y = t.y;
		file_list[idx].rect.w = t.w;
		file_list[idx].rect.h = t.h;
		draw_rect(file_list[idx].rect, white);
		t.x += file_list[idx].rect.w + WIDTH_PER_CH;

		file_list[idx].is_highlight = FALSE;
	}

	return file_num;
}
/* 追加(ここまで) */

void gui(void)
{
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	int px = 0, py = 0;
	unsigned long long waitidx;
	int file_num;	/* 追加 */
	int idx;	/* 追加 */

	SPP->Reset(SPP, FALSE);
	file_num = ls_gui();	/* 追加 */

	while (TRUE) {
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput), &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			/* マウスカーソル座標更新 */
			/* ・・・省略・・・ */

			/* マウスカーソル描画 */
			put_cursor(px, py);

			/* 追加(ここから) */
			/* ファイルアイコン処理ループ */
			for (idx = 0; idx < file_num; idx++) {
				if (is_in_rect(px, py, file_list[idx].rect)) {
					if (!file_list[idx].is_highlight) {
						draw_rect(file_list[idx].rect,
							  yellow);
						file_list[idx].is_highlight = TRUE;
					}
				} else {
					if (file_list[idx].is_highlight) {
						draw_rect(file_list[idx].rect,
							  white);
						file_list[idx].is_highlight =
							FALSE;
					}
				}
			}
			/* 追加(ここまで) */
		}
	}
}
//}

@<list>{sample5_2_gui_ls_gui_c}で追加したls_gui関数は、shell.cのls関数をラップした関数です。画面クリアした後、ls関数を実行してファイル/ディレクトリのリストを画面に表示し、その後、矩形を描きます。また、ls_gui関数は、描画する矩形の位置と大きさ、ハイライト状態をfile_list配列のメンバへ設定します。

@<list>{sample5_2_gui_ls_gui_c}のgui関数へ追加した"ファイルアイコン処理ループ"では、マウス操作に応じた各ファイルの処理を記述しています。ここでは、マウスカーソルがアイコンの上に乗ったらアイコンの枠をハイライト色へ変更する事を行っています。

サンプルの実行結果は@<img>{sample5_2_gui_ls_1}の通りです。

//image[sample5_2_gui_ls_1][ファイルリスト対応版GUIモード実行の様子]{
//}

== ファイルを読んでみる(cat)
ファイルのリストを取得できたので、次はファイルの中身を読んでみます。まずはシェルコマンドとして"cat(もどき)"を作ってみます。サンプルのディレクトリは"sample5_3_cat"です。

なお、ここではUEFIファームウェアをどのように呼び出せばファイルを読めるのかが分かればよいので、簡単のため、catコマンドが読み出すファイル名は固定とします。
#@# //footnote[because_com_args_impl_is_mendo][シェルへコマンドライン引数の実装が面倒なだけです。。。]

ファイルリスト取得の際にはディレクトリのEFI_FILE_PROTOCOLに対してRead関数を呼び出すことでファイル名を取得していました。ファイルのEFI_FILE_PROTOCOLに対してRead関数を呼び出すとファイルの内容を読むことができます(仕様書"12.5 EFI File Protocol(P.504)")。

では、ファイルのEFI_FILE_PROTOCOLはどうやって取得すればよいのかというと、ディレクトリのEFI_FILE_PROTOCOLに対してファイル名を指定してOpen関数を呼び出すことで取得できます(仕様書"12.5 EFI File Protocol(P.499)")。Open関数の定義は@<list>{open_definition}の通りです。

//list[open_definition][EFI_FILE_PROTOCOLのOpen関数の定義][c]{
unsigned long long (*Open)(struct EFI_FILE_PROTOCOL *This,
			   struct EFI_FILE_PROTOCOL **NewHandle,
			   unsigned short *FileName,
			   unsigned long long OpenMode,
			   unsigned long long Attributes);
//}

引数の意味は以下の通りです。

 : struct EFI_FILE_PROTOCOL **NewHandle
    新しく開いたEFI_FILE_PROTOCOLを格納するポインタへのアドレス。
 : unsigned short *FileName
    ファイル名。
 : unsigned long long OpenMode
    ファイルを開くモード(後述)。
 : unsigned long long Attributes
    属性ビット。ファイル作成時にファイルの属性ビットへ設定する値を指定。本書では使用しない。

OpenModeへ指定できる定数の定義は@<list>{open_mode_definitions}の通りです。

//list[open_mode_definitions][Open関数のOpenModeへ指定できる定数][c]{
#define EFI_FILE_MODE_READ	0x0000000000000001
#define EFI_FILE_MODE_WRITE	0x0000000000000002
#define EFI_FILE_MODE_CREATE	0x8000000000000000
//}

なお、OpenModeへ指定できる組み合わせは以下のいずれかです。

 * READ
 * READ | WRITE
 * READ | WRITE | CREATE

以上を踏まえてまとめると、"abc"というファイル名のファイルを読む際の処理の流れは以下の通りです。

 1. EFI_SIMPLE_FILE_SYSTEM_PROTOCOLのOpenVolume関数でボリュームを開く(ルートディレクトリのEFI_FILE_PROTOCOLを取得)
 2. 1.のEFI_FILE_PROTOCOLのOpen関数をファイル名に"abc"を指定して実行("abc"ファイルのEFI_FILE_PROTOCOLを取得)
 3. 2.のEFI_FILE_PROTOCOLのRead関数を呼び出す(ファイルの内容を取得)

catコマンドの実装は@<list>{sample5_3_cat_shell_c}の通りです。処理の流れとしては上記の1.～3.の通りで、最後にClose処理を行っています。

#@# sample5.3: ファイル読み出しサンプル
#@#   => 060_fs_cat_hello
//listnum[sample5_3_cat_shell_c][sample5_3_cat/shell.c]{
/* ・・・省略・・・ */

/* 追加(ここから) */
void cat(unsigned short *file_name)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, file_name, EFI_FILE_MODE_READ, 0);
	assert(status, L"root->Open");

	status = file->Read(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Read");

	puts(file_buf);

	file->Close(file);
	root->Close(root);
}
/* 追加(ここまで) */

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};

	while (TRUE) {
		puts(L"poiOS> ");
		if (gets(com, MAX_COMMAND_LEN) <= 0)
			continue;

		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		/* ・・・省略・・・ */
		else if (!strcmp(L"cat", com))	/* 追加 */
			cat(L"abc");	/* 追加 */
		else
			puts(L"Command not found.\r\n");
	}
}
//}

サンプルの実行の様子を@<img>{sample5_3_cat}に示します。なお、UEFIファームウェアの機能で表示できるUnicodeファイルは以下のnkfコマンドで作成可能です。

//cmd{
$ @<b>{nkf -w16L0 orig.txt > unicode.txt}
//}

//image[sample5_3_cat][catコマンド実行の様子]{
//}

== GUIモードへテキストファイル閲覧機能追加
ファイルの内容を取得する方法が分かったので、GUIモードを拡張してみます。ここでは以下の仕様とします。サンプルのディレクトリは"sample5_4_gui_cat"です。

 1. ファイルのアイコン(矩形)内を左クリックすると閲覧モード開始
 2. 閲覧モードでは、「(1)画面クリア」、「(2)ファイル内容をUnicodeで表示」を行う
 3. ESCキー押下で閲覧モード終了

主に追加・変更するソースコードはgui.cです。gui.cのソースコードを@<list>{sample5_4_gui_cat_gui_c}に示します。

#@# sample5.4: sample5.1.1へファイル読み出し機能追加
#@#   => 画面下部を表示領域とし、そこに読み出したテキストを出力する
#@#   => 実装の簡単のために、テキストは1行のみでもOK
#@#   => catへgui_mode追加
#@#      1. コンソール(text)クリア
#@#      2. ファイルの内容出力
#@#      3. 何らかのキー入力待ち
#@#      4. cat終了
#@#      5. catからgui()へ戻ると、画面再描画
//listnum[sample5_4_gui_cat_gui_c][sample5_4_gui_cat/gui.c][c]{
/* ・・・省略・・・ */

/* 追加(ここから) */
void cat_gui(unsigned short *file_name)
{
	ST->ConOut->ClearScreen(ST->ConOut);

	cat(file_name);

	while (getc() != SC_ESC);
}
/* 追加(ここまで) */

void gui(void)
{
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	int px = 0, py = 0;
	unsigned long long waitidx;
	int file_num;
	int idx;
	unsigned char prev_lb = FALSE;	/* 追加 */

	SPP->Reset(SPP, FALSE);
	file_num = ls_gui();

	while (TRUE) {
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput), &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			/* マウスカーソル座標更新 */
			/* ・・・省略・・・ */

			/* マウスカーソル描画 */
			put_cursor(px, py);

			/* ファイルアイコン処理ループ */
			for (idx = 0; idx < file_num; idx++) {
				if (is_in_rect(px, py, file_list[idx].rect)) {
					if (!file_list[idx].is_highlight) {
						draw_rect(file_list[idx].rect,
							  yellow);
						file_list[idx].is_highlight = TRUE;
					}
					/* 追加(ここから) */
					if (prev_lb && !s.LeftButton) {
						cat_gui(file_list[idx].name);
						file_num = ls_gui();
					}
					/* 追加(ここまで) */
				} else {
					if (file_list[idx].is_highlight) {
						draw_rect(file_list[idx].rect,
							  white);
						file_list[idx].is_highlight =
							FALSE;
					}
				}
			}

			/* 追加(ここから) */
			/* マウスの左ボタンの前回の状態を更新 */
			prev_lb = s.LeftButton;
			/* 追加(ここまで) */
		}
	}
}
//}

@<list>{sample5_4_gui_cat_gui_c}に関して、gui関数へはマウスクリック時の処理を追加しています。マウスのボタンから指を離した瞬間をマウスクリックとするために、マウスボタンの前回状態をprev_lbという変数へ格納しています。そして、ファイルクリック時にファイル名を指定してcat_gui関数を呼び出す処理を、"ファイルアイコン処理ループ"内へ追加しています。cat_gui関数はcat関数をラップしている関数で、上述の閲覧モードを実現しています。

サンプルを実行し、guiコマンドでguiモードを起動すると@<img>{sample5_4_gui_cat_1}のように表示されます。そして、ファイルをクリックすると@<img>{sample5_4_gui_cat_3}のようにファイルの内容が画面に表示されます。

//image[sample5_4_gui_cat_1][GUIモードでファイル一覧が表示される様子]{
//}
//image[sample5_4_gui_cat_3][ファイルの内容が表示される様子]{
//}

===[column] getcでスキャンコードも返せるよう修正(オガム文字の領域を使う)
@<list>{sample5_4_gui_cat_gui_c}のcat_gui関数では、ESCキーで閲覧モードを終了できるように、getc関数が"SC_ESC(ESCキーのスキャンコード)"を返すまで待機しています。実はこのサンプルコード時点で、common.cのgetc関数をスキャンコード「も」返すことができるように修正しています(@<list>{sample_5_4_gui_cat_common_c_getc})。

//listnum[sample_5_4_gui_cat_common_c_getc][sample_5_4_gui_cat/common_c(getc関数)][c]{
unsigned short getc(void)
{
	struct EFI_INPUT_KEY key;
	unsigned long long waitidx;

	ST->BootServices->WaitForEvent(1, &(ST->ConIn->WaitForKey),
				       &waitidx);
	while (ST->ConIn->ReadKeyStroke(ST->ConIn, &key));

	/* 変更 */
	return (key.UnicodeChar) ? key.UnicodeChar
		: (key.ScanCode + SC_OFS);
}
//}

@<list>{sample_5_4_gui_cat_common_c_getc}では、key.UnicodeCharが0(押下されたキーがUnicode範囲外)である時、key.ScanCodeに下駄(SC_OFS)を履かせた値を返しています。これは、スキャンコードの値の範囲(0x00〜0x17)が2バイトUnicodeの値の範囲(0x0000〜0xffff)と被っているためで、Unicodeの使わなそうな範囲をスキャンコードとして使っています。

ここでは0x1680〜0x1697をスキャンコードとしています。Unicodeのこの範囲は「オガム文字」という文字を扱う範囲らしく、「アイルランドを中心に発見された古アイルランド語の碑文に記されている文字で、中世初期に碑文用に用いられたといわれています。」とのことです(@<fn>{ogham})。
//footnote[ogham][@<href>{http://www.asahi-net.or.jp/~ax2s-kmtn/ref/unicode/european.html}]

そのため、SC_OFSとSC_ESCはcommon.hで@<list>{sample5_4_gui_cat_common_h_sc_ofs_esc}の様に定義しています。

//list[sample5_4_gui_cat_common_h_sc_ofs_esc][SC_OFSとSC_ESCの定義][c]{
#define SC_OFS	0x1680
#define SC_ESC	(SC_OFS + 0x0017)
//}

== ファイルへ書き込んでみる(edit)
ファイルを読むことができたので、次はファイルへの書き込みをしてみます。ここでは「画面上に記述した内容でファイルを上書きする」コマンドとしてeditコマンドを追加します。サンプルのディレクトリは"sample5_5_edit"です。

EFI_FILE_PROTOCOLのWrite関数でファイルへの書き込みが行えます。Write関数の定義は@<list>{write_definition}の通りです。

//list[write_definition][EFI_FILE_PROTOCOLのWrite関数の定義][c]{
unsigned long long (*Write)(struct EFI_FILE_PROTOCOL *This,
			    unsigned long long *BufferSize,
			    void *Buffer);
//}

引数の意味は以下の通りです。

 : unsigned long long *BufferSize
    第3引数"Buffer"のサイズ(バイト指定)を格納した変数のポインタを指定する。Write関数実行後、このポインタで参照された変数には書き込んだデータサイズが格納される。
 : void *Buffer
    書き込むデータを格納したバッファ。

なお、Write関数を実行しても、ただちにディスクへ反映されるわけではありません。キャッシュされているWrite結果をディスクへ反映させるにはFlush関数を実行する必要があります。Flush関数の定義は@<list>{flush_definition}の通りです。

//list[flush_definition][EFI_FILE_PROTOCOLのFlush関数の定義][c]{
unsigned long long (*Flush)(struct EFI_FILE_PROTOCOL *This);
//}

以上を踏まえてeditコマンドを追加します。変更後のshell.cを@<list>{sample5_5_edit_shell_c}に示します。

//listnum[sample5_5_edit_shell_c][sample5_5_edit/shell.c][c]{
/* ・・・省略・・・ */

/* 追加(ここから) */
void edit(unsigned short *file_name)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];
	int i = 0;
	unsigned short ch;

	ST->ConOut->ClearScreen(ST->ConOut);

	while (TRUE) {
		ch = getc();

		if (ch == SC_ESC)
			break;

		putc(ch);
		file_buf[i++] = ch;

		if (ch == L'\r') {
			putc(L'\n');
			file_buf[i++] = L'\n';
		}
	}
	file_buf[i] = L'\0';

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, file_name,
			    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
	assert(status, L"root->Open");

	status = file->Write(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Write");

	file->Flush(file);

	file->Close(file);
	root->Close(root);
}
/* 追加(ここまで) */

void shell(void)
{
	/* ・・・省略・・・ */
	while (TRUE) {
		/* ・・・省略・・・ */
		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		/* ・・・省略・・・ */
		else if (!strcmp(L"edit", com))	/* 追加 */
			edit(L"abc");	/* 追加 */
		else
			puts(L"Command not found.\r\n");
	}
}
//}

@<list>{sample5_5_edit_shell_c}のedit関数について、"@<code>{while (TRUE)}"のコードブロックが上書き用のデータを作成している処理で、ESCキーでループを抜けます。その後、ファイルへの書き込み処理を行います。また、shell関数内に関して、editコマンドもcatコマンドと同様に対象のファイル名は固定で、"abc"というファイル名とします。

サンプルの実行結果は@<img>{sample5_5_edit_1}、@<img>{sample5_5_edit_2}、@<img>{sample5_5_edit_3}の通りです。

//image[sample5_5_edit_1][editコマンドを実行する様子]{
//}
//image[sample5_5_edit_2][上書き用データを作成している様子]{
//}
//image[sample5_5_edit_3][上書きが反映されていることを確認する様子]{
//}

== GUIモードへテキストファイル上書き機能追加
shell.cへ追加したedit関数を使うように以下の仕様でGUIモードを拡張します。サンプルのディレクトリは"sample5_6_gui_edit"です。

 1. ファイルアイコン、あるいは何もない箇所を右クリックで上書きモード(edit関数)開始
 2. ESCキー押下で上書きモード終了

なお、何もない箇所を右クリックした場合はファイルを新規に作成するようにしてみます。ファイルを新規に作成するには、Open関数実行時に第3引数のOpenModeへEFI_FILE_MODE_CREATEを追加で指定するだけです。

主な変更箇所はgui.cとshell.cです。まずgui.cを@<list>{sample5_6_gui_edit_gui_c}に示します。

//listnum[sample5_6_gui_edit_gui_c][sample5_6_gui_edit/gui.c][c]{
/* ・・・省略・・・ */

void gui(void)
{
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	int px = 0, py = 0;
	unsigned long long waitidx;
	int file_num;
	int idx;
	unsigned char prev_lb = FALSE;
	unsigned char prev_rb = FALSE, executed_rb;	/* 追加 */

	SPP->Reset(SPP, FALSE);
	file_num = ls_gui();

	while (TRUE) {
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput), &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			/* マウスカーソル座標更新 */
			/* ・・・省略・・・ */

			/* マウスカーソル描画 */
			put_cursor(px, py);

			/* 右クリックの実行済フラグをクリア */	/* 追加 */
			executed_rb = FALSE;	/* 追加 */

			/* ファイルアイコン処理ループ */
			for (idx = 0; idx < file_num; idx++) {
				if (is_in_rect(px, py, file_list[idx].rect)) {
					/* ・・・省略・・・ */
					if (prev_lb && !s.LeftButton) {
						cat_gui(file_list[idx].name);
						file_num = ls_gui();
					}
					/* 追加(ここから) */
					if (prev_rb && !s.RightButton) {
						edit(file_list[idx].name);
						file_num = ls_gui();
						executed_rb = TRUE;
					}
					/* 追加(ここまで) */
				} else {
					/* ・・・省略・・・ */
				}
			}

			/* 追加(ここから) */
			/* ファイル新規作成・編集 */
			if ((prev_rb && !s.RightButton) && !executed_rb) {
				/* アイコン外を右クリックした場合 */
				dialogue_get_filename(file_num);
				edit(file_list[file_num].name);
				ST->ConOut->ClearScreen(ST->ConOut);
				file_num = ls_gui();
			}
			/* 追加(ここまで) */

			/* マウスの左右ボタンの前回の状態を更新 */
			prev_lb = s.LeftButton;
			prev_rb = s.RightButton;	/* 追加 */
		}
	}
}
//}

@<list>{sample5_6_gui_edit_gui_c}では右クリック時の処理を追加しています。"ファイルアイコン処理ループ"内に追加しているのがファイルを右クリックした場合の処理で、"ファイル新規作成・編集"のコードブロックがアイコン外を右クリックした場合の処理です。"ファイルアイコン処理ループ"内で右クリックを処理済みであることを示すために"executed_rb"変数を用意しています。

"ファイル新規作成・編集"時のdialogue_get_filename関数はshell.cへ追加しています。shell.cへは他に、edit関数実行時のOpenへEFI_FILE_MODE_CREATEを追加しています。変更後のshell.cを@<list>{sample5_6_gui_edit_shell_c}に示します。

//listnum[sample5_6_gui_edit_shell_c][sample5_6_gui_edit/shell.c][c]{
/* ・・・省略・・・ */

/* 追加(ここから) */
void dialogue_get_filename(int idx)
{
	int i;

	ST->ConOut->ClearScreen(ST->ConOut);

	puts(L"New File Name: ");
	for (i = 0; i < MAX_FILE_NAME_LEN; i++) {
		file_list[idx].name[i] = getc();
		if (file_list[idx].name[i] != L'\r')
			putc(file_list[idx].name[i]);
		else
			break;
	}
	file_list[idx].name[i] = L'\0';
}
/* 追加(ここまで) */

/* ・・・省略・・・ */

void edit(unsigned short *file_name)
{
	/* ・・・省略・・・ */
	status = root->Open(root, &file, file_name,
			    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
			    EFI_FILE_MODE_CREATE, 0);	/* 追加 */
	assert(status, L"root->Open");
	/* ・・・省略・・・ */
}

/* ・・・省略・・・ */
//}

サンプルの実行結果は@<img>{sample5_6_gui_edit_1}、@<img>{sample5_6_gui_edit_2}、@<img>{sample5_6_gui_edit_3}の通りです。ここではファイル新規作成を試しています。

//image[sample5_6_gui_edit_1][アイコン外を右クリック]{
//}
//image[sample5_6_gui_edit_2][ファイル名入力画面]{
//}
//image[sample5_6_gui_edit_3][ファイル内容入力]{
//}

#@# ===== ここまでがサンプルの最低限ライン =====
#@# ファイル作成、書き込みは個体サンプルのみの紹介で、OSもどきへの追加実装は省いても良い

#@# sample5.5: ファイル書き込みサンプル
#@#   => 062_fs_write

#@# sample5.6: ファイル書き込み機能追加
#@#   => 既存のファイル右クリックで追記モード
#@#   => 画面下部で文字列を入力することで追記
#@#   => 再度右クリックで終了

#@# sample5.7: ファイル作成サンプル
#@#   => 063_fs_create

#@# sample5.8: ファイル作成機能追加
#@#   => 右クリックで新規ファイル作成
#@#   => 画面下部の結果通知領域でファイル名を聞かれるので、ファイル名を入力する
