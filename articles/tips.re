= TIPS
この章では、TIPS集として参考になりそうな情報をまとめておきます。

== タイマーイベント
タイマーイベントを使って指定した時間待機する事を試してみます。使用する関数はSystemTable->BootServicesの中のCreateEventとSetTimerとWaitForEventの3つです。

CreateEvent関数は名前の通りイベント(EFI_EVENT)を生成する関数です。EFI_SIMPLE_TEXT_INPUT_PROTOCOLやEFI_SIMPLE_POINTER_PROTOCOLではWaitForKeyやWaitForInputといったイベントが用意されていましたが、通常は作る必要があります。定義は@<list>{create_event_definition}の通りです(仕様書"6.1 Event, Timer, and Task Priority Services(P.112)")。

//listnum[create_event_definition][CreateEvent関数の定義][c]{
unsigned long long (*CreateEvent)(
	unsigned int Type,
	unsigned long long NotifyTpl,
	void (*NotifyFunction)(void *Event, void *Context),
	void *NotifyContext,
	void *Event);
//}

また、引数の意味は以下の通りです。

 : unsigned int Type
    生成するイベントのタイプとモード、属性値。タイマーを扱う際は"EVT_TIMER(=0x80000000)"を指定する。その他のパラメータは仕様書P.112参照。
 : unsigned long long NotifyTpl
    イベント通知の際のタスク優先度レベル。おそらく
 : void (*NotifyFunction)(void *Event, void *Context)
 : void *NotifyContext
 : void *Event

SetTimer関数の定義は@<list>{set_timer_definition}の通りです。

== メモリアロケート
== マルチコア制御

== SNP(Simple Network Protocol)

== gnu-efiのビルド・実行方法
