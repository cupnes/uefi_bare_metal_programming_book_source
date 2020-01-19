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
