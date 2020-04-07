The following are the commands you need to run for all of our parts.

# Part 1
- make
- ./part1

# Part 2
- make
- ./part2 1000100000000

# Part 3
- make
- ./part3 /dev/input/by-id/YourDevice

# Part5
- make run_prepare
- ./part5 /dev/input/by-id/YourDevice
- make cleanup

Please make sure to rmmod everyone else's drivers before you begin.
The first command builds the binary, then insmods everything necessary, it will also run the part but it passes a hardcoded keyboard so it will most probably fail the last task (Which is fine and expected)
The second command actually runs the part, you just need to modify the argument to your keyboard.
The third one is necessary to rmmod all drivers, this is essential to not mess up the next part.

# Part6
- make run_prepare
- ./part5 /dev/input/by-id/YourDevice
- make cleanup

Please make sure to rmmod everyone else's drivers before you begin.
The first command builds the binary, then insmods everything necessary, it will also run the part but it passes a hardcoded keyboard so it will most probably fail the last task (Which is fine and expected)
The second command actually runs the part, you just need to modify the argument to your keyboard.
The third one is necessary to rmmod all drivers, this is essential to not mess up the next students.
