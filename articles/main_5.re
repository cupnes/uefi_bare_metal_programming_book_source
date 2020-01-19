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
