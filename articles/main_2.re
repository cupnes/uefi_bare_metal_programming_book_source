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
