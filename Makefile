GDB_SRV = /Applications/SEGGER/JLink/JLinkGDBServerCLExe
RTT_VIEWER = /Applications/SEGGER/JLink/JLinkRTTViewer.app/Contents/MacOS/JLinkRTTViewerExe
JLINK_EXE = /Applications/SEGGER/JLink/JLinkExe
FLASH_OPTS = -device STM32F411CE -if SWD -speed 4000 -autoconnect 1 -CommanderScript tools/flash.jlink

.PHONY:  call
call:
	$(RTT_VIEWER) --connection localport 19031 --autoconnect &
	$(GDB_SRV) \
  	-device STM32F411CE \
 	-if SWD \
  	-speed 4000 \
  	-select USB=852006438 \
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
  	-select USB=260109114 \
  	-port 2341 \
  	-swoport 2342 \
  	-telnetport 2343 \
  	-rtttelnetport 19041 \
  	-nogui

.PHONY: flash-call
flash-call:
	$(JLINK_EXE) -SelectEmuBySN 852006438 $(FLASH_OPTS)

.PHONY: flash-ans
flash-ans:
	$(JLINK_EXE) -SelectEmuBySN 260109114 $(FLASH_OPTS)

.PHONY: clean
clean:
	-pkill -9 -i jlink
