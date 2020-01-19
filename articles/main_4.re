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
