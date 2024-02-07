openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg &

gdb bin/main.elf \
-ex 'set pagination off' \
-ex 'target remote localhost:3333' \
-ex 'load' \
-ex 'break recurse' \
-ex 'continue'
# -ex 'x/1024hu tunerBuffer' \
# -ex 'quit'

killall openocd
