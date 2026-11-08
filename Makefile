GDB_SRV = /Applications/SEGGER/JLink/JLinkGDBServerCLExe
RTT_VIEWER = /Applications/SEGGER/JLink/JLinkRTTViewer.app/Contents/MacOS/JLinkRTTViewerExe
ARM_GDB = /opt/homebrew/bin/arm-none-eabi-gdb
ELF = build/Debug/Drill_black.elf

# 常駐GDBサーバ経由で書き込む。引数: GDBサーバのポート
# 残りBPを消し、reset+haltで書込み・照合し、reset後は停止状態のまま切断する
# (CLion接続後に Resume すれば起動直後からデバッグできる)
define gdb_flash
	$(ARM_GDB) -nx -batch \
		-ex "target extended-remote localhost:$(1)" \
		-ex "monitor clrbp" -ex "monitor reset" -ex "monitor halt" \
		-ex "load" -ex "compare-sections" \
		-ex "monitor reset" -ex "monitor memU32 0xE000EDFC = 0x01000000" \
		-ex "disconnect" $(ELF)
endef

.PHONY:  call
call:
	$(RTT_VIEWER) --connection localport 19031 --autoconnect &
	$(GDB_SRV) \
  	-device STM32F411CE \
 	-if SWD \
  	-speed 4000 \
  	-select usb=260109114 \
  	-port 2331 \
  	-swoport 2332 \
  	-telnetport 2333 \
  	-rtttelnetport 19031 \
  	-nogui

.PHONY: ans
ans:
	$(RTT_VIEWER) --connection localport 19041 --autoconnect &
	$(GDB_SRV)  \
  	-device STM32F411CE \
 	-if SWD \
  	-speed 4000 \
  	-select usb=852006438 \
  	-port 2341 \
  	-swoport 2342 \
  	-telnetport 2343 \
  	-rtttelnetport 19041 \
  	-nogui

.PHONY: flash-call
flash-call:
	$(call gdb_flash,2331)

.PHONY: flash-ans
flash-ans:
	$(call gdb_flash,2341)

.PHONY: clean
clean:
	-pkill -9 -i jlink
