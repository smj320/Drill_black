.PHONY:  debug_call
debug_call:
	JLinkGDBServerCL -device STM32F411CE \
          -if SWD \
          -speed 4000 \
          -select usb=260109114 \
          -port 2341 \
          -swoport 2342 \
          -telnetport 2343 \
          -rtttelnetport 19022

.PHONY: debug_ans
debug_ans:
	JLinkGDBServerCL -device STM32F411CE \
          -if SWD \
          -speed 4000 \
          -select usb=852006438 \
          -port 2351 \
          -swoport 2352 \
          -telnetport 2353 \
          -rtttelnetport 19032