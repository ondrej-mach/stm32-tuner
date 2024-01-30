openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg &

gdb bin/main.elf \
-ex 'target remote localhost:3333'

kill $(jobs -p)
