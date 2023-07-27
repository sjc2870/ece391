dir=student-distrib


echo "run gdb bootimg now in other window "
qemu-system-i386 -hda $dir/mp3.img -m 256 -gdb tcp:127.0.0.1:1234 -S -name mp3 -curses
