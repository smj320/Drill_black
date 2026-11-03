# UVCメモ

## JLINKの接続
給電を行う。jlinkコマンドから行う。二台あるときは
JLinkExe -usb <シリアル番号>
Education;260109114
PlusCompact:852006438
power on perm

## Embedded gdb server
gdb /usr/local/bin/JLinkGDBServerCL
call
-device STM32F411CE -if SWD -speed 4000 -select usb=260109114 -port 2331  -rtttelnetport 19021
gdbはポートを連続して3つ使うので、別ポートにする場合は大きく開けたほうがよい
ans
-device STM32F411CE -if SWD -speed 4000 -select usb=852006438 -port 2341  -rtttelnetport 19022

## クラスの登録

クラスは/USB_HOST/App/usb_host.cで登録している。
CDCの場合、登録するべきUSBH_CDC_CLASSはusbh_cdc.hで
```c
extern USBH_ClassTypeDef  CDC_Class;
#define USBH_CDC_CLASS    &CDC_Class
```
と定義されている。USVHの場合は、
HAL_source/Middlewares/ST/STM32_USB_Host_Library/Class/VIDEO/Inc/usbh_video.h
で
```c++
extern USBH_ClassTypeDef  VIDEO_Class;
#define USBH_VIDEO_CLASS  &VIDEO_Class
```
と定義されているので、これを登録すればよかったはず。




iliasamさんの STM32_HOST_UVC_Camera をベースにされているのですね！実績のある非常に強力なコードですので、これをBlack Pill（STM32F411）へ移植して「毎分2枚」の仕様に落とし込むための具体的な修正ポイントが見えてきます。
このサンプルは、標準のSTM32USBホストライブラリを拡張してUVCクラスを追加した構成になっています。Discovery（F407）からBlack Pill（F411）へ移植する際、絶対に書き換えるべき「3つのポイント」は以下となります。

1. クロック設定（HSEとPLL）の書き換え

iliasamさんのコードは、Discoveryボード標準の 8MHz晶発振器（HSE） を前提にクロック（SYSCLK=168MHz、USB=48MHz）が計算されています。
一般的なBlack Pill（WeAct製など）は 25MHzの晶発振器 が載っているため、main.c の SystemClock_Config を以下のように修正し、USB用の48MHzを正確に作り出す必要があります。
• 修正例（F411 / 25MHz HSE から 96MHz駆動＋48MHz USBを作る場合）
• HSE_VALUE： 25000000 (25MHz)
• PLL_M： 25  （まず1MHzに落とす）
• PLL_N： 192 （192MHzまで引き上げる）
• PLL_P： 2   （SYSCLK = 96MHz、F411の最大値は100MHzですが、48MHzを綺麗に作るため96MHzにします）
• PLL_Q： 4   （USBCLK = 192 / 4 = 48MHz 🎯）

2. VBUSパワー制御コードの無効化

サンプルの usb_bsp.c や usbh_usr.c の中（特に USBH_HW_Init や USB_OTG_BSP_DriveVBUS 周辺）には、Discoveryボード上のパワーマシンスイッチIC（STMPS2141など）を叩いてカメラに5Vを給電するコード（例：GPIO_SetBits(GPIOC, GPIO_Pin_0) など）が含まれています。
• 対策： Black PillにはこのスイッチICも制御ピンもないため、VBUSを駆動（DriveVBUS）している中身のGPIO操作コードをすべてコメントアウトしてください。
• 物理的な給電： 代わりに、前述の通りUSBカメラのVBUSへは外部から5Vを直接給電してください。

3. メモリ（SRAM）割り当ての縮小（uvc_host.h など）

iliasamさんのコードは、リアルタイムに画面（LCD等）に描画するため、かなり大きめのリングバッファやダブルバッファをSRAM上に静的確保している可能性があります。F407（192KB）からF411（128KB）へ移すと、容量オーバーでビルドエラーになるケースがあります。
• 対策： 今回は「毎分2枚（30秒に1枚）」の超低頻度です。ストリーミングを常時回して画面を更新する必要はありません。
• uvc_host.h やバッファ定義箇所を見つけ、同時パケット受信用のバッファサイズ（UVC_IN_EP_SIZE や PREV_BUF_SIZE など）を、カメラの解像度（QVGAやVGA）のJPEGが1枚収まる最小限のサイズに絞ってください。

💡 毎分2枚で回すためのアプリケーション側の制御イメージ

無事にBlack Pillで列挙（Enumeration）が成功し、usbh_usr.c 内のステートが HOST_USER_CLASS_ACTIVE になったら、以下の手順でJPEGデータを1枚分だけぶっこ抜きます。
c
// mainループ、またはタイマー割り込みでの処理イメージ
while (1) {
// 1. 30秒（または任意の時間）待機
HAL_Delay(30000);

    // 2. UVCのストリーミングを開始（iliasamさんのAPIを叩く）
    // 例: UVC_StartStreaming(&USB_OTG_Core, &USB_Host);

    // 3. フラグやマーカーを確認し、JPEGが1枚バッファに溜まるのを待つ
    // iliasamさんのコード内の「jpeg_buffer」や「frame_ready」のようなフラグを監視
    while(!frame_is_complete) {
        USBH_Process(&USB_OTG_Core, &USB_Host); // USBタスクを回す
    }

    // 4. ストリーミングを即座に停止（メモリや負荷を解放するため）
    // 例: UVC_StopStreaming(&USB_OTG_Core, &USB_Host);

    // 5. バッファにあるJPEGバイト列（FF D8 ... FF D9）を、
    //    UARTやSPIなどの「bytestream」に一気に流し込む！
    // HAL_UART_Transmit(&huart1, jpeg_buffer, jpeg_size, 1000);

    frame_is_complete = 0; // フラグをクリアして次の30秒へ
}