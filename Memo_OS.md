# Free RTOS 系メモ


### SysTicの変更

cubemxでコード生成するときに警告画出るので、
FreeRTOS側でタイマーをSysTickからTM5に変更する。
SysTickはOSが利用するので、HALが使うタイマをSysTickから逃がす意図。

### cmakeの修正
```text
file(GLOB_RECURSE SOURCES "Core/*.*" "Drivers/*.*" "Middlewares/*.*")
に
CONFIGURE_DEPENDS を追加する。
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "Core/*.*" "Drivers/*.*" "Middlewares/*.*")
```

### リンクエラー対策

FreeRTOSをいれた場合、Cmakefileにリンカスクリプトを明示しないとエラーになる
```aiignore
# project settings
project(CMX869B C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# 以下を追加
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F303xx_FLASH.ld")
```

### PLL起動エラー対策

OpenOCDのreset-initがPLL(HSI x16)をSYSCLKにしているので、
そのままだとSystemClock_Config()のPLL再設定がHAL_ERRORになる。HSIに戻しておく
```aiignore
HAL_RCC_DeInit();
```

### 外部割込関数を書く位置

```aiignore
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) // 設定したピン番号に合わせて変更
    {
        // 割り込み発生時の処理（フラグ操作やLEDのトグルなど）
    }
}
```

### タイマの使い分け
RTOSなしの場合、HAL_delay()関数はSysTickを使う。RTOSがあるときは
OSがSysTickを使い、HAL_delay()はCubeMXのSysでTimebase Source
をTIM6とかに割り当てる。周期タスクはTM2とかを割り当てる。

## デバッグ環境

### JLINKの接続
給電を行う。jlinkコマンドから行う。二台あるときは
JLinkExe -usb <シリアル番号>
Education;260109114
PlusCompact:852006438
power on perm

### Embedded gdb serverのパス
gdb /usr/local/bin/JLinkGDBServerCL
call
-device STM32F411CE -if SWD -speed 4000  -select usb=260109114 -port 2341 -swoport 2342 -telnetport 2343 -rtttelnetport 19122
ans
-device STM32F411CE -if SWD -speed 4000 -select usb=852006438 -port 2351 -swoport 2352 -telnetport 2353  -rtttelnetport 19132

RTTの出力を見るには
call
nc localhost 19122
ans
nc localhost 19132

### RTOSを入れたときのデバッガがとまらない問題

OpenOCD標準の target/stm32f3x.cfg は、書き込みに使うRAMの作業領域をデフォルトで 16KB にしています。
F303K8のSRAMは 12KB しかありません。
•
書き込み用のバッファはイメージが大きいほど大きく確保されます。FreeRTOSを入れてイメージが約19KBに増えたため、
バッファが実在しないRAMの範囲まではみ出し、error writing to flash at address 0x08000000
で失敗していました。FreeRTOSを入れる前は、イメージが小さかったので表に出なかったのだと思います。
修正した内容
st_nucleo_f3.cfg、st_nucleo_f3_ans.cfg、st_nucleo_f3_call.cfg の3つで、
source [find target/stm32f3x.cfg] の前に次の行を追加しました。
set WORKAREASIZE 0x2000
このあと書き込みは Verified OK になり、ボードには現在のビルドが入っています。

## UART関係

### printfをUART2に接続

```c++
/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    // ここに周期割込で実行したい処理を書く
    // 例: フラグを立てる、GPIOを反転させるなど
  }
}
/* USER CODE END 4 */
```

### SEGGER_RTT.h

SEGGER_RTT.hがSEGGER_RTT_ConfigDefault.hを読んで、
EGGER_RTT_ConfigDefault.hがEGGER_RTT_Config.hを呼んでいる。