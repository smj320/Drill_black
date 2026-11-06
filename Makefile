RTT_VIEWER = /Applications/SEGGER/JLink_V978/JLinkRTTViewer.app/Contents/MacOS/JLinkRTTViewerExe

.PHONY:  vcall
vcall:
	$(RTT_VIEWER) -device STM32F411CE -if SWD -speed 4000 -usb 852006438 -a &

.PHONY: vans
vans:
	$(RTT_VIEWER) -device STM32F411CE -if SWD -speed 4000 -usb 260109114 -a &