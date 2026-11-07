# CLion は MI (-target-select) で接続するため hookpost-remote は発火しない。
# 書き込みは Run Configuration の Before launch (make flash-call / flash-ans) で行う。
# 手動で書き込みたいときは、ターゲットを一時停止して GDB コンソールで `flash` と入力。
define flash
  monitor reset
  load
  monitor reset
end
document flash
Reset target, load current ELF, and reset again.
end
