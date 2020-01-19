= poiOSの機能拡張例
ここまでOSっぽいものを作る事を目指してUEFIファームウェアの機能の呼び出し方を説明してきました。題材として作ってきたpoiOSはまだまだ不完全なものなので、ぜひご自身で機能拡張を行ってみてください(あるいはフルスクラッチで作ってみてください)。

ここでは機能拡張の例を紹介します。この章で説明する全ての拡張を行ったサンプルを"sample_poios"のディレクトリに格納しています。

== 画像を表示してみる
フレームバッファのアドレスとピクセルフォーマットが分かっているので、フレームバッファへ書き込むことで画像を表示することもできます。

=== blt関数を追加
まず、指定されたバッファに格納されているピクセルデータをフレームバッファへ書き込む関数"blt(BLock Transfer)"をgraphics.cへ追加します(@<list>{sample_poios_graphics_c_blt})。

//listnum[sample_poios_graphics_c_blt][sample_poios/graphics.c(blt関数)][c]{
void blt(unsigned char img[], unsigned int img_width, unsigned int img_height)
{
	unsigned char *fb;
	unsigned int i, j, k, vr, hr, ofs = 0;

	fb = (unsigned char *)GOP->Mode->FrameBufferBase;
	vr = GOP->Mode->Info->VerticalResolution;
	hr = GOP->Mode->Info->HorizontalResolution;

	for (i = 0; i < vr; i++) {
		if (i >= img_height)
			break;
		for (j = 0; j < hr; j++) {
			if (j >= img_width) {
				fb += (hr - img_width) * 4;
				break;
			}
			for (k = 0; k < 4; k++)
				*fb++ = img[ofs++];
		}
	}
}
//}

@<list>{sample_poios_graphics_c_blt}は単に引数imgのバイト列をピクセルデータとしてフレームバッファへ書き込んでいるだけです。なお、画像は常に原点(0,0)から描画されます。

=== シェルへ画像閲覧コマンド追加(view)
次に、blt関数を使用して画像ファイルを画面へ描画するコマンド"view"をshell.cへ追加します(@<list>{sample_poios_shell_c})。なお、「画像ファイル」はUEFIに対応したピクセルフォーマットの生バイナリとします。

//listnum[sample_poios_shell_c][sample_poios/shell.c][c]{
/* ・・・省略・・・ */
#define MAX_COMMAND_LEN	100

#define MAX_IMG_BUF	4194304	/* 4MB */	/* 追加 */

unsigned char img_buf[MAX_IMG_BUF];	/* 追加 */
/* ・・・省略・・・ */
void view(unsigned short *img_name)
{
	unsigned long long buf_size = MAX_IMG_BUF;
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned int vr = GOP->Mode->Info->VerticalResolution;
	unsigned int hr = GOP->Mode->Info->HorizontalResolution;

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"error: SFSP->OpenVolume");

	status = root->Open(root, &file, img_name, EFI_FILE_MODE_READ,
			    EFI_FILE_READ_ONLY);
	assert(status, L"error: root->Open");

	status = file->Read(file, &buf_size, (void *)img_buf);
	if (check_warn_error(status, L"warning:file->Read"))
		blt(img_buf, hr, vr);

	while (getc() != SC_ESC);

	status = file->Close(file);
	status = root->Close(root);
}

void shell(void)
{
	/* ・・・省略・・・ */

	while (!is_exit) {
		/* ・・・省略・・・ */
		else if (!strcmp(L"view", com)) {	/* 追加 */
			view(L"img");	/* 追加 */
			ST->ConOut->ClearScreen(ST->ConOut);	/* 追加 */
		} else
			puts(L"Command not found.\r\n");
	}
}
//}

@<list>{sample_poios_shell_c}について、viewコマンド実行時、"img"というファイル名を指定してview関数を実行します。view関数は引数で指定されたファイル名のファイルを開き、読み出したバイナリデータをバッファ"img_buf"へ格納しblt関数へ渡します。その後、ESCキー押下まで待機し、ESCキーが押下されるとview関数を抜けます。cat関数やedit関数もそうですが、shellの各コマンドの関数は「画面を散らかしたまま返す」が仕様なので、view関数も画面をクリアせずに呼び出し元へ戻ります。そのため、shell関数側でview関数実行後、ClearScreen関数を呼び出しています。

====[column] BGRA32ビットへの画像変換方法
BGRA32ビット(各8ビット)の生の画像データ(ヘッダ等が無いバイナリ)への変換は、ImageMagickのconvertコマンドで行えます。コマンド例は以下の通りです。

//cmd{
$ @<b>{convert hoge.png -depth 8 yux.bgra}
//}

なお、現状のviewコマンドは画像のスクロールやリサイズは行うことができません。そのため、筆者は、自身のPCのUEFIファームウェアがデフォルトで認識している解像度(Width=640px)に合わせるように画像をリサイズしています。コマンド例は以下の通りです。

//cmd{
$ @<b>{convert hoge.png -resize 640x -depth 8 yux.bgra}
//}

=== GUIモードへ画像ファイル表示機能追加
最後に、gui.cを拡張し、画像ファイルをクリックするとview関数を使用して画面へ画像を描画するようにします(@<list>{sample_poios_gui_c_gui_icon_loop})。なお、poiOSにおいては「"i"で始まるファイル名のファイル」を画像ファイルとすることにします。

//listnum[sample_poios_gui_c_gui_icon_loop][sample_poios/gui.c(gui関数内)][c]{
/* ファイルアイコン処理ループ */
for (idx = 0; idx < file_num; idx++) {
	if (is_in_rect(px, py, file_list[idx].rect)) {
		/* ・・・省略・・・ */
		if (prev_lb && !s.LeftButton) {
			if (file_list[idx].name[0] != L'i')	/* 追加 */
				cat_gui(file_list[idx].name);
			else	/* 追加 */
				view(file_list[idx].name);	/* 追加 */
			file_num = ls_gui();
		}
		/* ・・・省略・・・ */
	}
}
//}

@<list>{sample_poios_gui_c_gui_icon_loop}に示す通り、gui.cの変更箇所はgui関数内の"ファイルアイコン処理ループ"内だけです。左クリック時にcat_gui関数を呼び出していたところを、ファイル名が"i"で始まる場合にはview関数を呼び出すようにしています。

===[column] blt関数はUEFIの仕様に存在します
実は仕様上、EFI_GRAPHICS_OUTPUT_PROTOCOL内にBltという関数が存在します(仕様書"11.9.1 Blt Buffer(P.474)")。UEFIのBlt関数はバッファのデータをフレームバッファへ書き込む(EfiBltBufferToVideo)だけでなく、フレームバッファのデータをバッファへ格納する(EfiBltVideoToBltBuffer)、フレームバッファのある矩形領域内のデータを別の座標へコピーする(EfiBltVideoToVideo)等ができます。

ただし、筆者が動作確認に使用しているLenovo ThinkPad E450(UEFIバージョン2.3.1)ではこの機能を使うことができず@<fn>{dont_work_blt}、今回は自前で実装しました。なお、QEMU(OVMF、UEFIバージョン2.3.1)では動作しました。
//footnote[dont_work_blt][ステータスはSuccessを返すが、画面には何も描画されていないという状況でした。]

== GUIモードとシェルの終了機能を追加する
=== シェル終了機能追加
exitコマンドで終了できるようにしてみます。

変更箇所はshell.cのshell関数内だけです(@<list>{sample_poios_shell_c_shell_exit})。

//listnum[sample_poios_shell_c_shell_exit][sample_poios/shell.c(shell関数へexit機能追加)][c]{
void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};
	unsigned char is_exit = FALSE;	/* 追加 */

	while (!is_exit) {	/* 変更 */
		/* ・・・省略・・・ */
		} else if (!strcmp(L"exit", com))	/* 追加 */
			is_exit = TRUE;	/* 追加 */
		else
			puts(L"Command not found.\r\n");
	}
}
//}

=== GUIモード終了機能追加
右上に[X]ボタンを配置し、このボタンをクリックすることでGUIモードを終了できるようにしてみます。

変更するソースコードはgui.cのみです。まず、[X]ボタンを配置するput_exit_button関数と、マウスカーソルに対して再描画やクリック判定を行うupdate_exit_button関数を追加します(@<list>{sample_poios_gui_c_put_update_exit_button})。

//listnum[sample_poios_gui_c_put_update_exit_button][sample_poios/gui.c(put_exit_buttonとupdate_exit_button)][c]{
/* ・・・省略・・・ */
#define EXIT_BUTTON_WIDTH	20
#define EXIT_BUTTON_HEIGHT	20
/* ・・・省略・・・ */
struct FILE rect_exit_button;
/* ・・・省略・・・ */
void put_exit_button(void)
{
	unsigned int hr = GOP->Mode->Info->HorizontalResolution;
	unsigned int x;

	rect_exit_button.rect.x = hr - EXIT_BUTTON_WIDTH;
	rect_exit_button.rect.y = 0;
	rect_exit_button.rect.w = EXIT_BUTTON_WIDTH;
	rect_exit_button.rect.h = EXIT_BUTTON_HEIGHT;
	rect_exit_button.is_highlight = FALSE;
	draw_rect(rect_exit_button.rect, white);

	/* EXITボタン内の各ピクセルを走査(バッテンを描く) */
	for (x = 3; x < rect_exit_button.rect.w - 3; x++) {
		draw_pixel(x + rect_exit_button.rect.x, x, white);
		draw_pixel(x + rect_exit_button.rect.x,
			   rect_exit_button.rect.w - x, white);
	}
}

unsigned char update_exit_button(int px, int py, unsigned char is_clicked)
{
	unsigned char is_exit = FALSE;

	if (is_in_rect(px, py, rect_exit_button.rect)) {
		if (!rect_exit_button.is_highlight) {
			draw_rect(rect_exit_button.rect, yellow);
			rect_exit_button.is_highlight = TRUE;
		}
		if (is_clicked)
			is_exit = TRUE;
	} else {
		if (rect_exit_button.is_highlight) {
			draw_rect(rect_exit_button.rect, white);
			rect_exit_button.is_highlight = FALSE;
		}
	}

	return is_exit;
}
//}

@<list>{sample_poios_gui_c_put_update_exit_button}について、put_exit_button関数では[X]ボタンを画面に描画すると同時に、グローバル変数"struct FILE rect_exit_button"へボタンの座標・大きさ・ハイライト状態を格納しています。また、update_exit_button関数は、ハイライト状態を更新するのに加え、ボタンクリック判定結果をTRUE/FALSEで返します。

次に、これらの関数を使用するようにgui関数へ機能追加を行います(@<list>{sample_poios_gui_c_gui_exit})。

//listnum[sample_poios_gui_c_gui_exit][sample_poios/gui.c(gui関数へ終了機能追加)][c]{
void gui(void)
{
	/* ・・・省略・・・ */
	unsigned char is_exit = FALSE;	/* 追加 */

	SPP->Reset(SPP, FALSE);
	file_num = ls_gui();
	put_exit_button();	/* 追加 */

	while (!is_exit) {	/* 変更 */
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput), &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			/* ・・・省略・・・ */

			/* ファイルアイコン処理ループ */
			for (idx = 0; idx < file_num; idx++) {
				if (is_in_rect(px, py, file_list[idx].rect)) {
					/* ・・・省略・・・ */
					if (prev_lb && !s.LeftButton) {
						/* ・・・省略・・・ */
						file_num = ls_gui();
						put_exit_button();  /* 追加 */
					}
					if (prev_rb && !s.RightButton) {
						edit(file_list[idx].name);
						file_num = ls_gui();
						put_exit_button();  /* 追加 */
						executed_rb = TRUE;
					}
				} else {
					/* ・・・省略・・・ */
				}
			}

			/* ファイル新規作成・編集 */
			if ((prev_rb && !s.RightButton) && !executed_rb) {
				/* アイコン外を右クリックした場合 */
				dialogue_get_filename(file_num);
				edit(file_list[file_num].name);
				ST->ConOut->ClearScreen(ST->ConOut);
				file_num = ls_gui();
				put_exit_button();	/* 追加 */
			}

			/* 追加(ここから) */
			/* 終了ボタン更新 */
			is_exit = update_exit_button(px, py,
						     prev_lb && !s.LeftButton);
			/* 追加(ここまで) */

			/* ・・・省略・・・ */
		}
	}
}
//}

@<list>{sample_poios_gui_c_gui_exit}では、最初の画面描画時とcatコマンド実行時等の画面再描画時にput_exit_button関数を実行するようにしています。また、マウス処理のループの最後に"終了ボタン更新"の処理を追加し、update_exit_buttonの戻り値に応じてgui関数のメインループを抜けるようにしています。

== マウスを少し大きくする
1pxのマウスカーソルは、使えなくは無いのですが、あまりにも小さいので、少し大きく4x4pxにしてみます。

変更箇所はgui.cのみです(@<list>{sample_poios_gui_c_cursor})。

//listnum[sample_poios_gui_c_cursor][sample_poios/gui.c(マウスカーソルを大きくする)][c]{
#define CURSOR_WIDTH	4	/* 追加 */
#define CURSOR_HEIGHT	4	/* 追加 */
/* ・・・省略・・・ */
/* 追加(ここから) */
struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL cursor_tmp[CURSOR_HEIGHT][CURSOR_WIDTH] =
{
	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
};
int cursor_old_x;
int cursor_old_y;
/* 追加(ここまで) */
/* ・・・省略・・・ */
void draw_cursor(int x, int y)
{
	/* 変更(ここから) */
	int i, j;
	for (i = 0; i < CURSOR_HEIGHT; i++)
		for (j = 0; j < CURSOR_WIDTH; j++)
			if ((i * j) < CURSOR_WIDTH)
				draw_pixel(x + j, y + i, white);
	/* 変更(ここまで) */
}

void save_cursor_area(int x, int y)
{
	/* 変更(ここから) */
	int i, j;
	for (i = 0; i < CURSOR_HEIGHT; i++) {
		for (j = 0; j < CURSOR_WIDTH; j++) {
			if ((i * j) < CURSOR_WIDTH) {
				cursor_tmp[i][j] = get_pixel(x + j, y + i);
				cursor_tmp[i][j].Reserved = 0xff;
			}
		}
	}
	/* 変更(ここまで) */
}

void load_cursor_area(int x, int y)
{
	/* 変更(ここから) */
	int i, j;
	for (i = 0; i < CURSOR_HEIGHT; i++)
		for (j = 0; j < CURSOR_WIDTH; j++)
			if ((i * j) < CURSOR_WIDTH)
				draw_pixel(x + j, y + i, cursor_tmp[i][j]);
	/* 変更(ここまで) */
}

void put_cursor(int x, int y)
{
	if (cursor_tmp[0][0].Reserved)	/* 変更 */
		load_cursor_area(cursor_old_x, cursor_old_y);

	save_cursor_area(x, y);

	draw_cursor(x, y);

	cursor_old_x = x;
	cursor_old_y = y;
}
/* ・・・省略・・・ */
//}

@<list>{sample_poios_gui_c_cursor}については、これまでの関数の枠組みは変わらず、各関数の処理の規模が少し大きくなっただけです。なお、マウスカーソルの形は、"(x * y) < CURSOR_WIDTH"が真となる座標(x,y)にのみドットを置く事で、"┏"を描いています。

== 機能拡張版poiOS実行の様子
機能拡張後のpoiOSを画面写真で紹介します(@<img>{sample_poios_1}、@<img>{sample_poios_2}、@<img>{sample_poios_3}、@<img>{sample_poios_4})。

//image[sample_poios_1][GUIモードの状態]{
//}
//image[sample_poios_2][画像ファイルをクリックすると、画像を表示できます]{
//}
//image[sample_poios_3][[X\]ボタンクリックでGUIモード終了し、シェルモードへ戻ります]{
//}
//image[sample_poios_4][exitコマンド実行でシェルモードとpoiOS終了し、ブートメニュー画面へ]{
//}
